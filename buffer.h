#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// this is for disabling stderr messages (for the itu-t standard)
#ifdef _WIN32
#define NULL_DEVICE "NUL:"
#define TTY_DEVICE "COM1:"
#else
#define NULL_DEVICE "/dev/null"
#define TTY_DEVICE "/dev/tty"
#endif


#define FILE_PATH_SIZE 30
#define SEGMENT_NUMBER_BYTES 4
#define PLAYOUT_BUFFER_SIZE
#define BUFFER_CMD_SIZE 1024

struct buffer {
	int K;
	int* data;

	// duration of single elements
	int duration;

	// logical counter
	int counter;

	int write_idx;

	pthread_mutex_t lock;
};

struct stall {
	int start;
	int duration;
};
typedef struct buffer buffer;
typedef struct stall stall;

// convert a array of stalls inside the buffer in a string
void stalls_to_string(stall stalls[], int size, char *output, int output_size);
int get_counter(buffer* buffer);
buffer* create_buffer(int K);
int get_segments_duration(buffer* buffer);
bool is_still_stalling(buffer* buffer);
void slice_buffer(buffer* buffer, int n, int slice);
void print_buffer(buffer* buffer);
int get_buffer_size(buffer* buffer);
void add_element(buffer* buffer, int element);

#endif