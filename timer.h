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
#include <semaphore.h>
#include "utils.h"
#include "logger.h"

#define NULL ((void *) 0

struct timer_data {
	unsigned int interval;
	bool* condition;
	void (*task)();
	bool print_debug;

	long long start;

	long long end;
};

typedef struct timer_data timer_data;

// a struct representing the abstraction of the timer
// the timer is implemented as a thread signaling the action to execute and the other thread listening to it. They are synchronized with a semaphore
struct timer {
	struct timer_data* data;
	pthread_t thread_signaling;
	pthread_t thread_executing;

	sem_t semaphore;
};
typedef struct timer timer;

long long timeInMilliseconds();
timer* create_timer(void* task, int interval, bool print_debug);
int resume_timer(timer* timer);
int suspend_timer(timer* timer);
int cancel_timer(timer* timer);
void timer_join(timer* timer);
timer* create_timer_cond(void* task, unsigned int interval, bool* condition, bool print_debug);
#endif