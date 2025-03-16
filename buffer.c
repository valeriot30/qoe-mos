#include "buffer.h"

void slice_buffer(buffer* buffer, int n, int slice) {


    slice = slice % n;

    int temp[n];
    
    for (int i = 0; i < n; i++) {
        temp[i] = buffer->data[(i + slice) % n];
    }
   
    for (int i = 0; i < n; i++) {
        buffer->data[i] = temp[i];
    }

    for(int i = (n - slice); i < n; i++) {
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

void add_element(buffer* buffer, int element) {
    if(element < 0)
        return;

    if(buffer->data == NULL)
        return;

    buffer->data[buffer->write_idx] = element;

    buffer->write_idx++;
    buffer->counter++;

    return;
}

bool is_still_stalling(buffer* buffer) {
    return buffer->data[buffer->write_idx] == -1;
}

buffer* create_buffer(int K) {
    buffer* buffer_instance = (buffer*) malloc(sizeof(struct buffer));

    if(buffer_instance == NULL) {
        perror("malloc failed");
        return NULL;
    }

    buffer_instance->data = (int*) calloc(K, sizeof(int));

    if(buffer_instance->data == NULL) {
        return NULL;
    }

    buffer_instance->K = K;
    buffer_instance->write_idx = 0;
    buffer_instance->counter = 0;
    buffer_instance->segments_after_stall = 0;
    buffer_instance->is_sliding = false;

    return buffer_instance;
}

void print_buffer(buffer* buffer) {
	
	for(int i = 0; i < buffer->K; i++) {

		//if(buffer[i] == 0) continue;

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