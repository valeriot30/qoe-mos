#include "timer.h"

static unsigned int current_timer_id = 0;


int cancel_timer(timer* timer) {
  if(timer == NULL) {
    // error
    return 1;
  }

  pthread_cancel(timer->thread_signaling);
  pthread_cancel(timer->thread_executing);
  free(timer->data);
  free(timer);

  return 0;
}


void* thread_send_signal(void* arg) {

    timer* data_timer = (struct timer*) arg;

    if(data_timer == NULL) {
      pthread_exit(NULL);
    }

    struct timer_data* data = (struct timer_data*) data_timer->data;

    if(data == NULL) {
      pthread_exit(NULL);
    }
    unsigned int interval = data->interval;

    struct timespec ts;
    ts.tv_sec = interval;  // seconds
    ts.tv_nsec = 0;        // nanoseconds

    while(1) {
        bool condition = *(data->condition);
        
        if(condition)
        {
          //printf("Thread 1: Waking up and signaling thread 2.\n");
          sem_post(&(data_timer->semaphore));
        }

        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    }

    return NULL;
}

void* thread_execute_task(void* arg) {
    
    timer* timer = (struct timer*) arg;

    if(timer == NULL) {
      pthread_exit(NULL);
    }

    struct timer_data* data = (struct timer_data*) timer->data;

    if(data == NULL) {
      pthread_exit(NULL);
    }

    while (1) {
      sem_wait(&timer->semaphore);
      (data->task)();

      //printf("Thread 2: Received signal and processing.\n");
    }
    //printf("Thread 2: Exiting.\n");
    return NULL;
}


timer* create_timer_cond(void* task, unsigned int interval, bool* condition, bool print_debug) {

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

  sem_init(&curr_timer->semaphore, 0, 0);

  pthread_t thread_signaling, thread_executing;



	int ret = pthread_create(&(thread_signaling), NULL, thread_send_signal, (timer*) curr_timer);
  ret = pthread_create(&(thread_executing), NULL, thread_execute_task, (timer*) curr_timer);
	/*#if defined(__APPLE__)
		pthread_setschedparam(&timer_thread, SCHED_RR, NULL);
	#endif*/

  
  pthread_setschedparam(thread_signaling, SCHED_FIFO, NULL);
  pthread_setschedparam(thread_executing, SCHED_FIFO, NULL);

	if (ret != 0) {
      perror("pthread_create failed");
      free(thread_timer_data);
      free(curr_timer);
      return NULL;
  }

  curr_timer->thread_signaling = thread_signaling;
  curr_timer->thread_executing = thread_executing;

  current_timer_id++;

	return curr_timer;
} 

void timer_join(timer* timer) {
  pthread_join(timer->thread_signaling, NULL);
  pthread_join(timer->thread_executing, NULL);

  sem_destroy(&(timer->semaphore));
}