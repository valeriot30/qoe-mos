#include "buffer.h"

void slice_buffer(buffer* buffer, int n, int slice) {

    for (int i = 0; i < n - slice; i++) {
        buffer->data[i] = buffer->data[i + slice];
    }

    for (int i = n - slice; i < n; i++) {
        buffer->data[i] = 0;
    }

    buffer->write_idx -= slice;
}

int get_counter(buffer* buffer) {
    return buffer->counter;
}

int get_buffer_size(buffer* buffer) {
    return buffer->K;
}

void add_element(buffer* buffer, float element) {
    if(element < -1)
        return;

    if(buffer->data == NULL)
        return;

    //pthread_mutex_lock(&buffer->lock);
    buffer->data[buffer->write_idx] = element;

    buffer->write_idx++;

    //if(buffer != -1)
    buffer->counter++;

    //pthread_mutex_unlock(&buffer->lock);

    return;
}

// check if the last element of the array is -1
bool is_still_stalling(buffer* buffer) {
    return buffer->data[buffer->write_idx - 1] == -1;
}

buffer* create_buffer(int K, int duration) {
    buffer* buffer_instance = (buffer*) malloc(sizeof(struct buffer));

    if(buffer_instance == NULL) {
        perror("malloc failed");
        return NULL;
    }

    buffer_instance->data = (float*) calloc(K, sizeof(float));

    if(buffer_instance->data == NULL) {
        return NULL;
    }

    buffer_instance->K = K;
    buffer_instance->write_idx = 0;
    buffer_instance->counter = 1;
    buffer_instance->duration = duration; // in seconds

    //pthread_mutex_init(&buffer_instance->lock, NULL);

    return buffer_instance;
}

int get_segments_duration(buffer* buffer) {
    return buffer->duration;
}

void print_buffer(buffer* buffer) {
	
	for(int i = 0; i < buffer->K; i++) {

		//if(buffer->data[i] == 0) continue;

		printf("| %d |", buffer->data[i]);
	}
	printf("\n");
}

void stalls_to_string(stall stalls[], int size, char *output, int output_size) {
    int offset = 0;
    offset += snprintf(output + offset, output_size - offset, "'[");
    
    for (int i = 0; i < size; i++) {

    	if(stalls[i].start == -1) {
    		continue;
    	}

        offset += snprintf(output + offset, output_size - offset, "[%d, %d]", stalls[i].start, stalls[i].duration);
        if (i < size - 1) {
            offset += snprintf(output + offset, output_size - offset, ", ");
        }
    }
    
    snprintf(output + offset, output_size - offset, "]'");
}