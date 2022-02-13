#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h> // provides usleep()
#include <sys/time.h> // provides us clock for running at controlled rate, diagnostics

#include <X11/Xlib.h>
#include <X11/keysym.h>
//#include <X11/extensions/XTest.h> // needs install package libxtst-dev

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
	struct timespec desired_sleep;
	struct timespec remaining_sleep;
};


// TODO switch to usleep? https://linuxtut.com/en/5fc76fb094dfe06b03c9/
// nanosleep is giving warnings, annoying
void loop_frequency_delay(struct LoopTimeManagement* timer){
	// get the current time
	long now = get_time_us();
	timer->program_elapsed_time = now - timer->program_start_time;

	// calculate how long elapsed this loop
	timer->consumed_loop_time = now - timer->last_loop_start_time;

	long rest_time = timer->desired_loop_time - timer->consumed_loop_time;
	if (rest_time > 0){
		timer->desired_sleep.tv_sec = 0;
		timer->desired_sleep.tv_nsec = 1000*rest_time;

		// give up to 32 times to retry the nanosleep function in case it gets interrupted? may not be necessary
		// keep updating the required time so 
		timer->loop_attempts = 0;
		for (size_t i=0; i<32; i++){
			timer->loop_attempts++; 
			if (nanosleep(&(timer->desired_sleep), &(timer->remaining_sleep)) == 0){
				break;
			}
			timer->desired_sleep = timer->remaining_sleep;
		}
	}
	
	// store the 'last loop start time'
	now = get_time_us();
	timer->actual_loop_time = now - timer->last_loop_start_time;
	timer->last_loop_start_time = get_time_us();
}
/////////////////////////////////////////////////////////////////////////////////////
XKeyEvent createKeyEvent(Display* display, Window win, Window winRoot, Bool press, int keycode, int modifiers){
  XKeyEvent event;
  event.display = display;
  event.window = win;
  event.root = winRoot;
  event.subwindow = None; // ???
  event.time = CurrentTime; // ???
  event.x = 1;
  event.y = 1;
  event.x_root = 1;
  event.y_root = 1;
  event.same_screen = True;
  event.keycode = XKeysymToKeycode(display, keycode);
  event.state = modifiers; 

  if(press){
    event.type = KeyPress;
    printf("key press\n");
  }
  else{
    event.type = KeyRelease;
    printf("key release\n");
  }
  return event;
}

/////////////////////////////////////////////////////////////////////////////////////

int main(){

  Display *display;
  display = XOpenDisplay(NULL); // unclear if this should be 0 or NULL
  assert(display != NULL);

  Window root_window; // myself
  Window target_window; // the game
  Window root_ret;
  Window child_ret;

  int root_x, root_y, win_x, win_y;
  unsigned int mask_ret;

  // root_window = XRootWindow(display, DefaultScreen(display));
  root_window = XDefaultRootWindow(display);

  printf("5 second delay, move your cursor!\n");
  usleep(5000*1e3);
  printf("done\n");

  printf("XQueryPointer() now!\n");
  XQueryPointer(display, root_window, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask_ret);
  printf("XQueryPointer() done.\n");

  if (child_ret == (Window) NULL){
    printf("child_ret == NULL\n");
    target_window = root_ret;
  }
  else{
    target_window = child_ret;
    printf("found something\n");
  }

  // TODO this hack should just get the current focused window
  int revert;
  XGetInputFocus(display, &target_window, &revert);


  assert(target_window != (Window) NULL);
  XSelectInput(display, target_window, ExposureMask | KeyPressMask | KeyReleaseMask);
  
    // timers. all in us
	struct LoopTimeManagement tm;
	tm.desired_loop_time = 500*1e3; // in milliseconds; 60Hz=16667
	tm.program_start_time = get_time_us();
	tm.last_loop_start_time = tm.program_start_time; // initialize

  int status;

    for(int i=0; i<10; i++){

        loop_frequency_delay(&tm);

        // XK_k
        XKeyEvent event = createKeyEvent(display, target_window, root_window, True, XK_k, 0);
        status = XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*) &event);
        printf("status=%d\n", status);
    
        event = createKeyEvent(display, target_window, root_window, False, XK_k, 0);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*) &event);
 
        // unsigned int keycode = XKeysymToKeycode(display, XK_k);
        //XTestFakeKeyEvent(display, keycode, True, 0);
        //XTestFakeKeyEvent(display, keycode, False, 0);
        XFlush(display);
    }

  XCloseDisplay(display);

  return 0;
}
