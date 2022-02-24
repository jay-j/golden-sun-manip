// input man-in-the-middle; passes keyboard and records them. And records battle state, etc.
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#include <zmq.h> // for IPC stuff

#include "golden_sun.h"
#include "golden_sun_utils.h"
#include "loop_timer.h"

#define STATE_PASSTHRU 0
#define STATE_BATTLE_INIT 1
#define STATE_BATTLE_CMD 2
#define STATE_BATTLE_WATCH 3
#define STATE_BATTLE_END 4

#define ACTIONS_MAX 128

///////////////////////////////////////////////////////////////////////////////////
void key_tap(Display* display, int keycode_sym){
  unsigned int keycode = XKeysymToKeycode(display, keycode_sym);
  XTestFakeKeyEvent(display, keycode, True, 0);
  XTestFakeKeyEvent(display, keycode, False, 0);
  XFlush(display);
}

void key_press(Display* display, int keycode_sym){
  unsigned int keycode = XKeysymToKeycode(display, keycode_sym);
  XTestFakeKeyEvent(display, keycode, True, 0);
}

void key_release(Display* display, int keycode_sym){
  unsigned int keycode = XKeysymToKeycode(display, keycode_sym);
  XTestFakeKeyEvent(display, keycode, False, 0);
}

////////////////////////////////////////////////////////////////////////////////////

#define TELEOP_COMMAND_MSG_SIZE 11
typedef struct {
  int8_t dpad_up;    // w; up 
  int8_t dpad_down;  // s; down
  int8_t dpad_left;  // a; left
  int8_t dpad_right; // d; right
  int8_t button_a;   // a
  int8_t button_b;   // b
  int8_t select;     // select
  int8_t start;      // start
  int8_t button_left;  // shoulder left
  int8_t button_right; // shoulder right
  int8_t battle_init; 
} Teleop_Command;

void joystick(void* socket, Teleop_Command* teleop_command){
  int rc = zmq_recv(socket, teleop_command, TELEOP_COMMAND_MSG_SIZE, ZMQ_DONTWAIT);

  // only print debug data if a message was received
  if (rc != -1){
    uint8_t* ptr = (uint8_t*) teleop_command;
    uint8_t change = 0;
    for (size_t i=0; i<TELEOP_COMMAND_MSG_SIZE; ++i){
      change |= *ptr;
      ++ptr;
    }

    if (change > 0){
      printf("received teleop command: ");
      uint8_t* ptr = (uint8_t*) teleop_command;
      for (size_t i=0; i<11; ++i){
        printf("%u  ", *ptr);
        ++ptr;
      }
      printf("\n");
    }
    
  }
  else{
    // zero the teleop command structure; zero means no commands this cycle!
    void* result = memset(teleop_command, 0, TELEOP_COMMAND_MSG_SIZE);
    assert(result != NULL);
  }
}


void passthru(Display* display, Teleop_Command* teleop_command){
  KeySym bmap[] = {XK_w, XK_s, XK_a, XK_d, XK_k, XK_j, XK_n, XK_m, XK_q, XK_e};
  KeySym* key = bmap;
  uint8_t* cmd = (uint8_t*) teleop_command;

  for (size_t i=0; i<10; ++i){
    if (*cmd == 1){
      key_press(display, *key);
    }
    if (*cmd == 2){
      key_release(display, *key);
    }
    ++key;
    ++cmd;
  }

  XFlush(display);
}


////////////////////////////////////////////////////////////////////////////////////


uint8_t* action_tracking(pid_t pid, uint8_t* wram_ptr, uint8_t* action_list, uint8_t* action_current, Teleop_Command* teleop){
  // TODO how to handle limits
  if (teleop->dpad_right == 1){
    *action_current += 1;
  }
  if (teleop->dpad_left == 1){
    *action_current -= 1; 
  }

  if (teleop->button_a == 1){
    ++action_current;
    assert(action_current - action_list < ACTIONS_MAX);
  }

  // TODO need to move extra; backtracking goes from one character to the previous (push it 3 back, not 1)
  // need to detect what the selection state is; character ready vs some detail
  if (teleop->button_b == 1){
    if (action_current > action_list){
      if (get_battle_menu_character(pid, wram_ptr) == 0){
        action_current -= 3;
      }
      else{
        --action_current;
      }
    }
  }

  // print the action list up to this point
  uint8_t* p = action_list;
  printf("Current Action List: ");
  while (p <= action_current){
    printf("%u  ", *p);
    ++p;
  }
  printf("\n");

  return action_current;
}


////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){

  // find the game process, find where in heap the start of wram is 
  pid_t pid = find_pid();
  uint8_t* wram_ptr = find_wram(pid);

  // clock the state machine
  struct LoopTimeManagement tm;
  tm.desired_loop_time = 30*1e3; // 30Hz, why not?
  tm.program_start_time = get_time_us();
  tm.last_loop_start_time = tm.program_start_time;

  // setup stuff for key pushing
  Display* display = XOpenDisplay(NULL);
  assert(display != NULL);

  // start up zmq server 
  // passes commands 1:1
  void* zmq_context = zmq_ctx_new();
  void* server_socket = zmq_socket(zmq_context, ZMQ_PULL);
  int zmq_opt = 1;
  zmq_setsockopt(server_socket, ZMQ_CONFLATE, &zmq_opt, sizeof zmq_opt);
  int zmq_status = zmq_bind(server_socket, "tcp://*:5555");
  assert(zmq_status == 0);
  Teleop_Command teleop_command;

  // make the golden sun data structures
  Unit allies_raw[4];
  Unit enemies_raw[5];
  ExportAlly allies_send[4];
  ExportEnemy enemies_send[5];

  printf("Two second delay starting...\n");
  usleep(2000*1e3);

  // action tracking
  uint8_t action_list[ACTIONS_MAX];
  memset(action_list, 0, ACTIONS_MAX);
  uint8_t* action_current = action_list;
  
  // special button to initialize a battle mode. initialize when the menu is up (so first action is fight)
  // record game state
  // also directly send battle commands.. but accumulate values for them. 
  //  e.g. simulate state to know advanced to what action, what subaction, what target.
  //  don't allow the user to perform the side hops from below the top row in dual menu options? 
  //  don't allow flipping around the end - since length of a state is not tracked properly
  //  what about buff spells? they target the user first. just record how many shifts for the targeting part.
  // record net battle commands
  // automatically exit battle mode once all enemies have been detected defeated
  //
  int state = STATE_PASSTHRU;
  int battle_menu_previous = 0;
  int battle_menu_current = 0;
  for (;;){
    loop_frequency_delay(&tm);

    // zmq comms; get joystick data
    // fills in the teleop_command struct
    joystick(server_socket, &teleop_command);

    // get data from golden sun
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies_raw, 4);
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies_raw, 5);
    battle_menu_previous = battle_menu_current;
    battle_menu_current = get_battle_menu(pid, wram_ptr);

    // make nice states to send
    export_copy_allies(pid, wram_ptr, allies_raw, allies_send);
    export_copy_enemies(enemies_raw, enemies_send);

    if (state == STATE_PASSTHRU){
      // printf("STATE:                PASSTHRU\n");
      // send keys on 1:1
      passthru(display, &teleop_command);

      // listen to initialize battle mode
      if (teleop_command.battle_init == 1){
        state = STATE_BATTLE_INIT;
      }
    } 

    if (state == STATE_BATTLE_INIT){
      for (size_t i=0; i<4; i++){
        print_data_ally(allies_send+i);
      }

      // reset the action tracking
      memset(action_list, 0, ACTIONS_MAX);

      // get the initial state; the enemy input data
      // advance to STATE_BATTLE_CMD
      if (teleop_command.button_a == 1){
        state = STATE_BATTLE_CMD;
      }
    }

    if (state == STATE_BATTLE_CMD){
      // printf("STATE:        CMD\n");
      passthru(display, &teleop_command);
      // tricky! record the action state
      // TODO how to handle downed characters?
      // TODO record current party order, re=arrange things from allies_raw
      // TODO convert the action commands here into "psyenergy 1C" commands. if bot tries to give such a command and it is invalid, just defend instead
      action_current = action_tracking(pid, wram_ptr, action_list, action_current, &teleop_command); 

      // upon leaving this state, save the input action dataset
      if ((battle_menu_current == 0) && (battle_menu_previous == 1)){
        printf("  end listening for player input in battle\n");
        state = STATE_BATTLE_WATCH;
      }
    }

    if (state == STATE_BATTLE_WATCH){
      printf("STATE: WATCH\n");
      // just mash B until the battle menu flag is once again detected
      key_tap(display, XK_j);
 
      for(int i=0; i<5; i++){
        printf("%u  ", enemies_raw[i].health_current);
      }
      printf("\n");

      // TODO make some new states for these conditions!

      // put some lag in leaving this state? to capture the close ups? make new states for the close up situation? 
      // has changed to command but only for a short time and enemy total health is > 0

      // upon leaving this state, save the battle ally+enemy dataset
      if ((battle_menu_current == 1) && (battle_menu_previous == 0)){
        printf("  end passive watching of the battle.\n");
        state = STATE_BATTLE_INIT;
      }

      // if all enemies are dead... go back to passthrough mode
      if (health_total(enemies_raw, 5) == 0){
        state = STATE_BATTLE_END;
      }
    }
    // TODO there is some condition of enemy attacks where this logic doesn't work...?! :(

    if (state == STATE_BATTLE_END){
      printf("STATE: BATTLE ENDED!\n");
      for(int i=0; i<3; ++i){
        key_tap(display, XK_k); // TODO how many of these to finish the battle?
      }
      state = STATE_PASSTHRU;
    }

  }

  // cleanup!
  zmq_close(server_socket);
  zmq_ctx_destroy(zmq_context);
  
  return 0;
}
