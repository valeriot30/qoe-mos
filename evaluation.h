#ifndef EVALUATION_H
#define EVALUATION_H
	
#include "buffer.h"
#include "timer.h"

#include "json/cJSON.h"
#include "evaluation.h"
#include "ws/ws.h"
#include "ws/base64.h"

#define DEFAULT_N 10
#define DEFAULT_P 4

struct evaluation_data {
	int N;
	int p;

	char* last_output;

	bool started;

	buffer* buffer;
};

typedef struct evaluation_data evaluation_data;

void print_evaluation_window(evaluation_data* data);
int generate_evaluation_command(evaluation_data* data, char* output);
void* evaluation_task(evaluation_data* data);
void set_started(evaluation_data* data, bool state);
bool eval_started(evaluation_data* data);
evaluation_data* create_evaluation_data(int N, int p, buffer* assigned_buffer);
#endif