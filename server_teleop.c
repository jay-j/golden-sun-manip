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

#define ACTIONS_MAX 5

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

    /*
    if (change > 0){
      printf("received teleop command: ");
      uint8_t* ptr = (uint8_t*) teleop_command;
      for (size_t i=0; i<11; ++i){
        printf("%u  ", *ptr);
        ++ptr;
      }
      printf("\n");
    }
    */
    
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

FILE* logfile_init(){
  FILE* fd;
  printf("[LOG] FILE INIT\n");
  fd = fopen("test.txt", "w");
  fprintf(fd, "HEADER HERE\n");
  return fd;
}


void write_binary(FILE* fd, uint8_t* data, size_t length){
  for (size_t i=0; i<length; ++i){
    fputc((char) *data, fd);
    ++data;
  }
  fprintf(fd, "\n");
}


void logfile_write_action(FILE* fd, ML_Action_Space* act){
  printf("[LOG] WRITE ACTION\n");
  fprintf(fd, "action here\n");
  write_binary(fd, (uint8_t*) act, sizeof(ML_Action_Space));
  // TODO binary blob, just send the raw ML_Action_State struct
}


void logfile_write_state(FILE* fd, ML_Observation_Space* obs){
  printf("[LOG] WRITE STATE\n");
  fprintf(fd, "state here\n");
  write_binary(fd, (uint8_t*) obs, sizeof(ML_Observation_Space));
  // TODO binary blob, just send the raw ML_Observation_State struct
}


void logfile_close(FILE* fd){
  printf("[LOG] CLOSE\n");
  fprintf(fd, "CLOSER HERE\n");
  fclose(fd);
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
  Unit allies_raw[ALLIES];
  Unit enemies_raw[ENEMIES_MAX];
  Battle_Action actions_raw[BATTLE_ACTION_QUEUE_MAX_LENGTH];

  ML_Observation_Space observation_space;

  ML_Action_Space action_space;

  // log file pointer
  FILE* logfile;

  printf("Two second delay starting...\n");
  usleep(2000*1e3);
  printf("go!\n");

  // special button to initialize a battle mode. initialize when the menu is up (so first action is fight)
  // record game state
  // also directly send battle commands.. but accumulate values for them. 
  //  e.g. simulate state to know advanced to what action, what subaction, what target.
  // record net battle commands
  // automatically exit battle mode once all enemies have been detected defeated
  
  int state = STATE_PASSTHRU;
  int battle_menu_previous = 0;
  int battle_menu_current = 0;
  for (;;){
    loop_frequency_delay(&tm);

    // zmq comms; get joystick data
    // fills in the teleop_command struct
    joystick(server_socket, &teleop_command);

    // get data from golden sun
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies_raw, ALLIES);
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies_raw, ENEMIES_MAX);
    get_battle_action_queue(pid, wram_ptr, actions_raw);
    battle_menu_previous = battle_menu_current;
    battle_menu_current = get_battle_menu(actions_raw); // TODO modify to take the action state
    // TODO djinn data
    // TODO battle action list

    // make nice states to send
    export_copy_allies(pid, wram_ptr, allies_raw, observation_space.allies);
    export_copy_enemies(enemies_raw, observation_space.enemies);
    get_djinn(pid, wram_ptr, allies_raw, &observation_space.djinn);

    ////////// for normal gameplay //////////
    if (state == STATE_PASSTHRU){
      // send keys on 1:1
      passthru(display, &teleop_command);

      // listen to initialize battle mode
      if (teleop_command.battle_init == 1){
        logfile = logfile_init();
        state = STATE_BATTLE_INIT;
      }
    } 

    ////////// just before commands start - record the STATE //////////
    if (state == STATE_BATTLE_INIT){
      /*
      for (size_t i=0; i<ALLIES; i++){
        print_data_ally(allies_send+i);
      }
      */
 
      for(int i=0; i<ENEMIES_MAX; i++){
        printf("%u  ", enemies_raw[i].health_current);
      }
      printf("\n");

      // TODO write state
      logfile_write_state(logfile, &observation_space);

      // get the initial state; the enemy input data
      state = STATE_BATTLE_CMD;
    }

    ////////// player is entering commands, wait for the exit, then record the ACTION //////////
    if (state == STATE_BATTLE_CMD){
      passthru(display, &teleop_command);

      // upon leaving this state, save the input action dataset
      if (get_byte(pid, wram_ptr, MEMORY_OFFSET_BATTLE_MENU) == 0){
        printf("  end listening for player input in battle\n");
        get_battle_action_queue(pid, wram_ptr, actions_raw);
        export_action_state(actions_raw, action_space.actions); 
        logfile_write_action(logfile, &action_space);
        state = STATE_BATTLE_WATCH;
      }
    }

    ////////// mash the b button while the battle goes by //////////
    if (state == STATE_BATTLE_WATCH){
      // just mash B until the battle menu flag is once again detected
      key_tap(display, XK_j);
 
      // upon leaving this state, save the battle ally+enemy dataset
      if ((battle_menu_current == 1) && (battle_menu_previous == 0)){
        printf("  end passive watching of the battle.\n");
        state = STATE_BATTLE_INIT;
      }

      // if all enemies are dead... go back to passthrough mode
      if (health_total(enemies_raw, ENEMIES_MAX) == 0){
        state = STATE_BATTLE_END;
      }
    }

    ////////// battle is over, record state one more time, close up the log file //////////
    if (state == STATE_BATTLE_END){
      printf("STATE: BATTLE ENDED!\n");
      // TODO log state
      // TODO close log file
      logfile_write_state(logfile, &observation_space);
      logfile_close(logfile);

      state = STATE_PASSTHRU;
    }

  }

  // cleanup!
  zmq_close(server_socket);
  zmq_ctx_destroy(zmq_context);
  
  return 0;
}
