#include "timer.h"

static inline long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}


void* timer_task(void* thread_args) {
	struct timer_data *data;
    data = (struct timer_data *)thread_args;

    if(data == NULL) {
    	printf("Timer data is null\n");
    	return NULL;
    }

    #if !defined(__APPLE__)
  		sched_setscheduler(getpid(), SCHED_RR, NULL);
  	#endif

	while(true) {
		if(data->condition) {

			if(data->print_debug) {
				fflush(stdout);
			}

			if (data->start > 0)
    			data->end = timeInMilliseconds() - data->start;

    		if(data->print_debug) {
    			//INFO_LOG("Time took: %lld\n", data->end);
    		}
			(data->task)();
		}
		sleep(data->interval / 1000);
	}

	free(data);

	return NULL;
}

int cancel_timer(timer* timer) {
  if(timer == NULL) {
    // error
    return 1;
  }

  pthread_cancel(timer->thread);
  free(timer->data);
  free(timer);

  return 0;
}

int resume_timer(timer* timer) {
  if(timer == NULL) {
    return 1;
  }

  timer->data->condition = true;

  
  return 0;
}

int suspend_timer(timer* timer) {
  if(timer == NULL) {
    return 1;
  }

  timer->data->condition = false;

  return 0;
}

timer* create_timer_cond(void* task, int interval, bool condition, bool print_debug) {

  timer* curr_timer = (timer*) malloc(sizeof(struct timer));

  if(curr_timer == NULL) {
     perror("malloc failed");
     return NULL;
  }

	struct timer_data* thread_timer_data = (struct timer_data*) malloc(sizeof(struct timer_data));
	thread_timer_data->start = timeInMilliseconds();

	if (thread_timer_data == NULL) {
        perror("malloc failed");
        return NULL;
    }

  	thread_timer_data->interval = interval;
  	thread_timer_data->condition = condition;
  	thread_timer_data->task = task;
  	thread_timer_data->print_debug = print_debug;

  curr_timer->data = thread_timer_data;

	pthread_t timer_thread;
	int ret = pthread_create(&timer_thread, NULL, timer_task, (void*) thread_timer_data);

	#if defined(__APPLE__)
		pthread_setschedparam(&timer_thread, SCHED_RR, NULL);
	#endif

	if (ret != 0) {
        perror("pthread_create failed");
        free(thread_timer_data);
        return NULL;
  }

  curr_timer->thread = timer_thread;

	return curr_timer;
} 