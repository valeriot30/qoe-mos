/**
Copyright unime.it
**/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#include<sys/time.h>

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}


bool eval_started = false;
int N = 10;
int p = 2;
int* buffer = NULL;

long long start = 0;
long long end = 0;

struct stall {
	int start;
	int duration;
};
typedef struct stall stall;

void slice_buffer(int arr[], int n, int slice) {
    slice = slice % n;

    int temp[n];
    
    for (int i = 0; i < n; i++) {
        temp[i] = arr[(i + slice) % n];
    }
   
    for (int i = 0; i < n; i++) {
        arr[i] = temp[i];
    }
}

void print_buffer(int* buffer, int K) {
	for(int i = 0; i < K; i++) {
		printf("%d | ", buffer[i]);
	}
}

void* evaluation_task() {
	while(true) {
		if(eval_started)
		{
			if(start > 0)
				end = timeInMilliseconds() - start;
			//printf("Time passed: %lld \n", end);

			print_buffer(buffer, 20);

			stall stalls[20];

			bool stall_occurred;
			int current_stall_position = 0;


			// TODO improve stalls detection
			stalls[current_stall_position].start = 0;
			stalls[current_stall_position].duration = 0;

			for(int j = 0; j < 20; j++) {
				if(buffer[j] == -1 && !stall_occurred) {
					stalls[current_stall_position].start = j;
					stall_occurred = true;
				}
				else if(buffer[j] == -1 && stall_occurred) {
					stalls[current_stall_position].duration = stalls[current_stall_position].duration + 1;
				}
				else if(buffer[j] != -1 && stall_occurred) {
					stall_occurred = false;
				}
			}

			// to the evaluation



			//printf("MOS: 5 \t Evaluation took: %lld ms\n", end);
			start = timeInMilliseconds();
			slice_buffer(buffer, 20, 2);

			sleep(4);
		}
	}

	return NULL;
}

int main() {

	pthread_t evaluation_thread;
	pthread_create(&evaluation_thread, NULL, evaluation_task, NULL);

	buffer = (int*) calloc(20, sizeof(int));
	
	if(buffer == NULL) {
	    printf("There was an error with the allocation of the array");
	    return 1;
	}

	FILE* fptr;

	fptr = fopen("buffer.txt", "r");

	if(fptr == NULL)
	{
	    printf("Unable to open the file");
	    return 1;
	}

	int i = 0;
	
	fflush(stdin); 

	int counter = 0;
	while (!feof (fptr))
	{  
	    fscanf (fptr, "%d", &i);   
	    buffer[counter] = i;  
	   	printf ("%d \n", i);
	   	counter++;
	}
	
	fclose (fptr);  

	size_t n = (int) counter; 	
	printf("The length of the buffer is %d\n", (int) n);  

	if(n > N) {
	   printf("Evaluation can start\n");
	   eval_started = true;
	}

	printf("Starting evaluation...\n");

	while(1);

	return 0;
}
