#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h> // needs install package libxtst-dev

#include "loop_timer.h"

/////////////////////////////////////////////////////////////////////////////////////
void keypress(Display* display, int keycode_sym){
  unsigned int keycode = XKeysymToKeycode(display, keycode_sym);
  XTestFakeKeyEvent(display, keycode, True, 0);
  XTestFakeKeyEvent(display, keycode, False, 0);
  XFlush(display);
}

/////////////////////////////////////////////////////////////////////////////////////

int main(){

  Display *display;
  display = XOpenDisplay(NULL); 
  assert(display != NULL);

  printf("5 second delay, move your cursor!\n");
  usleep(5000*1e3);
  printf("done\n");

  // timers. all in us
	struct LoopTimeManagement tm;
	tm.desired_loop_time = 500*1e3; // in milliseconds; 60Hz=16667
	tm.program_start_time = get_time_us();
	tm.last_loop_start_time = tm.program_start_time; // initialize

  for(int i=0; i<10; i++){
      loop_frequency_delay(&tm);

      keypress(display, XK_k);
  }

  XCloseDisplay(display);

  return 0;
}
