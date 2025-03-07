/**
Copyright unime.it
**/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


bool eval_started = false;
int N = 10;
int p = 2;
int* buffer = NULL;

void* evaluation_task() {
	while(1) {
		sleep(4);
		printf("Counter\n");
	}
}

int main() {

	buffer = (int*) calloc(sizeof(int) * 20, 0);
	
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

	pthread_t evaluation_thread;
	//pthread_create(&evaluation_thread, NULL, &evaluation_task, NULL);

	//while(1);

	return 0;
}
