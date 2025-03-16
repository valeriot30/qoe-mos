#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include<sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include<sched.h>
#include "logger.h"

struct timer_data {
	int interval;
	int condition;
	void (*task)();
	bool print_debug;

	long long start;

	long long end;
};

typedef struct timer_data timer_data;

// a struct representing the abstraction of the timer
struct timer {
	struct timer_data* data;
	pthread_t thread;
};
typedef struct timer timer;

timer* create_timer(void* task, int interval, bool print_debug);
int resume_timer(timer* timer);
int suspend_timer(timer* timer);
int cancel_timer(timer* timer);
timer* create_timer_cond(void* task, int interval, bool condition, bool print_debug);
#endif