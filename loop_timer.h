#ifndef LOOP_TIMER_H
#define LOOP_TIMER_H

#include <stdio.h>
#include <unistd.h> // provides usleep()
#include <sys/time.h> // provides us clock for running at controlled rate, diagnostics

long get_time_us();
long get_time_ms();
long get_time_sec();

struct LoopTimeManagement{
	long program_start_time;
	long program_elapsed_time; 

	long last_loop_start_time;
	long desired_loop_time;
	long consumed_loop_time;
	long actual_loop_time;
};

void loop_frequency_delay(struct LoopTimeManagement* timer);

#endif // header guard