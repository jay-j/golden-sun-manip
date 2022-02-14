#include "loop_timer.h"

long get_time_us(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL); // seconds and us since UTC Epoch start
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}
long get_time_ms(){
	return (long) get_time_us() / 1e3;
}

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
		int loop_attempts = 0;
		for (size_t i=0; i<32; i++){
			loop_attempts++; 
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
