#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h> // provides usleep()
#include <sys/time.h> // provides us clock for running at controlled rate, diagnostics

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h> // needs install package libxtst-dev

long get_time_us(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL); // seconds and us since UTC Epoch start
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}
long get_time_ms(){
	return (long) get_time_us() / 1e3;
}

struct LoopTimeManagement{
	long program_start_time;
	long program_elapsed_time; 

	long last_loop_start_time;
	long desired_loop_time;
	long consumed_loop_time;
	long actual_loop_time;

	int loop_attempts;
};

void loop_frequency_delay(struct LoopTimeManagement* timer){
	// get the current time
	long now = get_time_us();
	timer->program_elapsed_time = now - timer->program_start_time;

	// calculate how long elapsed this loop
	timer->consumed_loop_time = now - timer->last_loop_start_time;

	long rest_time = timer->desired_loop_time - timer->consumed_loop_time;
	if (rest_time > 0){

		// give up to 32 times to retry the usleep function in case it gets interrupted? may not be necessary
		// keep updating the required time so 
		timer->loop_attempts = 0;
		for (size_t i=0; i<32; i++){
			timer->loop_attempts++; 
      int status = usleep(rest_time);

      if (status == 0){
        break;
      }
      printf("interrupted oh no\n"); 

      // calculate how much time is left
      rest_time -= get_time_us() - now;
      now = get_time_us();
      
		}
	}
	
	// store the 'last loop start time'
	now = get_time_us();
	timer->actual_loop_time = now - timer->last_loop_start_time;
	timer->last_loop_start_time = get_time_us();
}
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
