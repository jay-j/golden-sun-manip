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

#define TELEOP_COMMAND_MSG_SIZE 12
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
  int8_t anything;    // are any buttons newly pushed?
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


void copy_action_state(ML_Action_Space* action, Teleop_Command* teleop){
  action->dpad_up    = teleop->dpad_up;
  action->dpad_down  = teleop->dpad_down;
  action->dpad_left  = teleop->dpad_left;
  action->dpad_right = teleop->dpad_right;
  action->button_a   = teleop->button_a;
}

////////////////////////////////////////////////////////////////////////////////////

FILE* logfile_init(char* name, long time){
  FILE* fd;
  printf("[LOG] FILE INIT at %ld\n", time);
  // TODO make name merge with suffix and add date and stuff
  char* filename;
  snprintf("%ld-%s.log", time, name);
  fd = fopen(name, "w");
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
}


void logfile_write_state(FILE* fd, ML_Observation_Space* obs){
  printf("[LOG] WRITE STATE\n");
  fprintf(fd, "state here\n");
  write_binary(fd, (uint8_t*) obs, sizeof(ML_Observation_Space));
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
  uint8_t* wram_ptr = find_wram(pid, MEMORY_TYPE_WRAM_BOARD);
  uint8_t* iram_ptr = find_wram(pid, MEMORY_TYPE_WRAM_CHIP);

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

  ML_Observation_Space observation_space;
  ML_Action_Space action_space;

  // log file pointer
  FILE* logfile_state;
  FILE* logfile_action;

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
  long last_a_press = get_time_ms();
  for (;;){
    loop_frequency_delay(&tm);

    // zmq comms; get joystick data
    // fills in the teleop_command struct
    joystick(server_socket, &teleop_command);
    copy_action_state(&action_space, &teleop_command);

    // get data from golden sun
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ALLIES, allies_raw, ALLIES);
    get_unit_data(pid, wram_ptr+MEMORY_OFFSET_ENEMY, enemies_raw, ENEMIES_MAX);
    get_battle_menu_navigation(pid, wram_ptr, iram_ptr, &(observation_space.menu_nav));

    battle_menu_previous = battle_menu_current;
    // battle_menu_current = observation_space.menu_nav.menu_active & (observation_space.menu_nav.menu_l0 != 241);
    battle_menu_current = observation_space.menu_nav.menu_active;
    
    // make nice states to send
    export_copy_allies(pid, wram_ptr, allies_raw, observation_space.allies);
    export_copy_enemies(enemies_raw, observation_space.enemies);
    get_djinn(pid, wram_ptr, allies_raw, observation_space.djinn);

    ////////// for normal gameplay //////////
    if (state == STATE_PASSTHRU){
      // send keys on 1:1
      passthru(display, &teleop_command);

      // listen to initialize battle mode
      if (teleop_command.battle_init == 1){
        printf("[STATE] Battle Init!\n");
        long time = get_time_sec();
        logfile_state = logfile_init("state", time);
        logfile_action = logfile_init("action", time);
        state = STATE_BATTLE_INIT;
      }
    } 

    ////////// just before commands start //////////
    // currently just showing some debug info to the player.. TODO remove this state entirely?
    if (state == STATE_BATTLE_INIT){
      printf("menu_l0=%u\n", observation_space.menu_nav.menu_l0);

      // need to clear some status scrolling text
      if (observation_space.menu_nav.menu_l0 == 241){
        printf("pugh B\n");
        key_tap(display, XK_j);
      }

      // need to advance to commanding the first character
      if (observation_space.menu_nav.menu_l0 == 240){
        if (get_time_ms() - last_a_press > 100){
          printf("push A\n");
          key_tap(display, XK_k);
          last_a_press = get_time_ms();
        }
      }
 
      if (observation_space.menu_nav.menu_l0 == 0){
        printf("[STATE] leaving init to go to command mode.\n");
        state = STATE_BATTLE_CMD;
      }
    }

    ////////// player is entering commands, record STATE and ACTION on every button press //////////
    if (state == STATE_BATTLE_CMD){
      passthru(display, &teleop_command);
      
      // if any button was pressed then save both ACTION and STATE to logs
      if (teleop_command.anything){
        logfile_write_state(logfile_state, &observation_space);
        logfile_write_action(logfile_action, &action_space);
      }

      // upon leaving this state, save the input action dataset
      // printf("Battle Menu State: %u\n", battle_menu_current);
      if (battle_menu_current == 0){
      // if (observation_space.menu_nav.menu_active == 0){
        printf("  end listening for player input in battle\n");
        state = STATE_BATTLE_WATCH;
      }
    }

    ////////// mash the b button while the battle goes by //////////
    if (state == STATE_BATTLE_WATCH){
      // just mash B until the battle menu flag is once again detected
      key_tap(display, XK_j);
 
      // upon leaving this state, save the battle ally+enemy dataset
      if (battle_menu_current == 1){
        printf("  end passive watching of the battle.\n");
        state = STATE_BATTLE_INIT;
      }

      // if all enemies are dead... run battle end logic
      if ((health_total(enemies_raw, ENEMIES_MAX) == 0) && (observation_space.menu_nav.menu_l0 == 0)){
        state = STATE_BATTLE_END;
      }
    }

    ////////// battle is over, record state one more time, close up the log file //////////
    if (state == STATE_BATTLE_END){
      printf("STATE: BATTLE ENDED!\n");

      logfile_write_state(logfile_state, &observation_space);
      logfile_write_action(logfile_action, &action_space);
      logfile_close(logfile_state);
      logfile_close(logfile_action);

      state = STATE_PASSTHRU;
    }

  }

  // cleanup!
  zmq_close(server_socket);
  zmq_ctx_destroy(zmq_context);
  
  return 0;
}
