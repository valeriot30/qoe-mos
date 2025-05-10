#ifndef EVALUATION_H
#define EVALUATION_H
	
#include "buffer.h"
#include "timer.h"

#include "lib/json/cJSON.h"
#include "evaluation.h"
#include "lib/ws/ws.h"
#include "lib/ws/base64.h"

#define DEFAULT_N 10
#define DEFAULT_P 4
#define DEFAULT_D 1

struct evaluation_data {
	int N;
	int p;

	char* last_output;

	bool started;

	bool one_step;

	int mode;

	buffer* buffer;

	// total number of stalls
	int nf;

	// the total duration of stalls
	int tft;

	// the maximum duration of a single stall
	int tfm;

	float* departures;
	float* arrivals;
};

typedef struct evaluation_data evaluation_data;

void add_arrival_time(evaluation_data* data, float arrival_time);
void add_departure_time(evaluation_data* data, float departure_time);
void add_stall_duration(evaluation_data* data, int duration);
void print_evaluation_window(evaluation_data* data);
void increment_nf(evaluation_data* data);
int generate_evaluation_command(evaluation_data* data, char* output);
void* evaluation_task(evaluation_data* data);
void set_started(evaluation_data* data, bool state);
bool eval_started(evaluation_data* data);
bool is_one_step(evaluation_data* data);
int evaluate_segment(evaluation_data* data, int segmentNumber, char* output);
int get_mode(evaluation_data* data);
evaluation_data* create_evaluation_data(int N, int p, bool one_step, int mode, buffer* assigned_buffer);
#endif
