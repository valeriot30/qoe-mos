#ifndef EVALUATION_H
#define EVALUATION_H
	
#include "buffer.h"
#include "timer.h"

#include "cJSON/cJSON.h"
#include "evaluation.h"
#include "ws/ws.h"
#include "ws/base64.h"

struct evaluation_data {
	int segments_received_in_p;
	int N;
	int p;

	bool started;

	buffer* buffer;
};

typedef struct evaluation_data evaluation_data;

void* evaluation_task(evaluation_data* data);
void increment_segments_received(evaluation_data* data);
void set_started(evaluation_data* data, bool state);
int get_segments_received(evaluation_data* data);
bool eval_started(evaluation_data* data);
evaluation_data* create_evaluation_data(int N, int p, buffer* assigned_buffer);
#endif