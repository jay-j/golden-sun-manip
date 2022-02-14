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
#include "memory_utils.h"
#include "loop_timer.h"

#define STATE_PASSTHRU 0
#define STATE_BATTLE_INIT 1
#define STATE_BATTLE_CMD 2
#define STATE_BATTLE_WATCH 3

///////////////////////////////////////////////////////////////////////////////////
// TODO write function to convert whatever gets read 
// TODO how to handle key *holds* not just individual presses? 
// have independent event handling for push and release

void key_tap(Display* display, int keycode_sym){
  unsigned int keycode = XKeysymToKeycode(display, keycode_sym);
  XTestFakeKeyEvent(display, keycode, True, 0);
  XTestFakeKeyEvent(display, keycode, False, 0);
  XFlush(display);
}

////////////////////////////////////////////////////////////////////////////////////

#define TELEOP_COMMAND_MSG_SIZE 11
struct {
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
} teleop_command;

void joystick(void* socket){
  int rc = zmq_recv(socket, &teleop_command, TELEOP_COMMAND_MSG_SIZE, ZMQ_DONTWAIT);

  // only assign data if a message was received
  if (rc != -1){
    // TODO assign variables based on data in teleop_command object
    printf("received teleop command\n");
    
  }
  else{
    // zero the teleop command structure; zero means no commands this cycle!
    void* result = memset(&teleop_command, 0, TELEOP_COMMAND_MSG_SIZE);
    assert(result != NULL);
  }
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


  printf("Two second delay starting...\n");
  usleep(2000*1e3);

  
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
  int state = STATE_BATTLE_CMD;
  int battle_menu_previous = 0;
  int battle_menu_current = 0;
  for (;;){
    loop_frequency_delay(&tm);

    // zmq comms; get joystick data
    joystick(server_socket);

    battle_menu_previous = battle_menu_current;
    battle_menu_current = get_battle_menu(pid, wram_ptr);

    if (state == STATE_PASSTHRU){
      // send keys on 1:1
      // listen to initialize battle mode
    } 

    if (state == STATE_BATTLE_INIT){
      // get the initial state; the enemy input data
      // advance to STATE_BATTLE_CMD
    }

    if (state == STATE_BATTLE_CMD){
      printf("STATE:        CMD\n");
      // tricky! record the action state
      // TODO how to handle downed characters?

      // upon leaving this state, save the input action dataset
      if ((battle_menu_current == 0) && (battle_menu_previous == 1)){
        printf("  end listening for player input in battle\n");
        state = STATE_BATTLE_WATCH;
      }
    }

    if (state == STATE_BATTLE_WATCH){
      printf("STATE: WATCH\n");
      // just mash A until the battle menu flag is once again detected
      key_tap(display, XK_k);

      // upon leaving this state, save the battle ally+enemy dataset
      if ((battle_menu_current == 1) && (battle_menu_previous == 0)){
        printf("  end passive watching of the battle.\n");
        state = STATE_BATTLE_CMD;
      }
    }
    // TODO there is some condition of enemy attacks where this logic doesn't work...?! :(

  }

  // cleanup!
  zmq_close(server_socket);
  zmq_ctx_destroy(zmq_context);
  
  return 0;
}
