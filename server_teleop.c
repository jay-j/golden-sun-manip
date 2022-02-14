// input man-in-the-middle; passes keyboard and records them. And records battle state, etc.
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#include <zmq.h> // for IPC stuff

#include "loop_timer.h"

#define STATE_PASSTHRU 0
#define STATE_BATTLE_INIT 1
#define STATE_BATTLE_CMD 2
#define STATE_BATTLE_WATCH 2

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



#define TELEOP_COMMAND_MSG_SIZE 32
struct 

void joystick(void* socket){
  int rc = zmq_recv(socket, &teleop_command, TELEOP_COMMAND_MSG_SIZE, ZMQ_DONTWAIT);

  // only assign data if a message was received
  if (rc != -1){
    // TODO assign variables based on data in teleop_command object
    
  }
}

////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){

  // find the game process, find where in heap the start of wram is 
  pid_t pid = find_pid();
  uint8_t* wram_ptr = find_wram(pid);

  // clock the state machine
  struct LoopTimeManagement tm;
  tm.desired_loop_time = 33*1e3; // 30Hz, why not?
  tm.program_start_time = get_time_us();
  tm.last_loop_start_time = tm.program_start.time;

  // setup stuff for key pushing
  Display* display = XOpenDisplay(NULL);
  assert(display != NULL);

  // start up zmq server 
  // passes commands 1:1
  void* zmq_context = zmq_ctx_new();
  void* zmq_socket = zmq_socket(zmq_context, ZMQ_PULL);
  int zmq_opt = 1;
  zmq_setsockopt(zmq_socket, ZMQ_CONFLATE, &zmq_opt, sizeof zmq_opt);
  int zmq_status = zmq_bind(socket, "tcp://*:5555");
  assert(zmq_status == 0);

  
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
  for (;;){
    loop_frequency_delay(&tm);

    // zmq comms; get joystick data

    if (state == STATE_PASSTHRU){
      // send keys on 1:1
      // listen to initialize battle mode
    } 


    if (state == STATE_BATTLE_INIT){
      // get the initial state; the enemy input data
      // advance to STATE_BATTLE_CMD
    }

    if (state == STATE_BATTLE_CMD){
      // tricky! record the action state
      // TODO how to handle downed characters?

      // upon leaving this state, save the input action dataset
    }

    if (state == STATE_BATTLE_WATCH){
      // just mash A until the battle menu flag is once again detected
      key_tap(display, XK_a);

      // upon leaving this state, save the battle ally+enemy dataset
    }


  }
  //
  return 0;
}
