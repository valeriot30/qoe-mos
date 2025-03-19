
#include "evaluation.h"

void generate_evaluation_command(evaluation_data* data, char* output) {

  int* buffer = data->buffer->data;

  int K = get_buffer_size(data->buffer);

  stall stalls[K];

  for(int i = 0; i < K; i++) {
    stalls[i].start = -1;
    stalls[i].duration = -1;
  }
  bool stall_occurred;
  int current_stall_position = 0;

  current_stall_position++;

  char command[1024];

  strcpy(command, "python3 evaluate.py ");
  for (int j = 0; j < data->N; j++) 
  {

    if(buffer[j] == 0) continue;

    char buf[22 + 4];

    if (buffer[j] == -1 && !stall_occurred) {
      stalls[current_stall_position].start = (j + 1);
      stalls[current_stall_position].duration = 1;
      stall_occurred = true;
      continue;
    } else if (buffer[j] == -1 && stall_occurred) {
      stalls[current_stall_position].duration = stalls[current_stall_position].duration + 1;
      stall_occurred = true;
      continue;
    } else if (buffer[j] != -1 && stall_occurred) {
      stall_occurred = false;
      current_stall_position++;
    }
    snprintf(buf, sizeof(buf), "./segments/segment-%d.ts ", buffer[j]);
    strcat(command, buf);
  }

  strcat(command, "-s ");

  char tmp[256];

  stalls_to_string(stalls, K, tmp, 256);

  //printf("%s \n ", tmp);

  strcat(command, tmp);

  memcpy(output, command, sizeof(command));
}

void set_started(evaluation_data* data, bool state) {
  data->started = state;
}

bool eval_started(evaluation_data* data) {
  return data->started;
}

int get_segments_received(evaluation_data* data) {
  return data->segments_received_in_p;
}

void increment_segments_received(evaluation_data* data) {
  if(data == NULL)
    return;

  data->segments_received_in_p++;
}

evaluation_data* create_evaluation_data(int N, int p, buffer* assigned_buffer) {
  evaluation_data* data = (evaluation_data*) malloc(sizeof(struct evaluation_data));

  if(data == NULL) {
    return NULL;
  }

  data->N = N;
  data->p = p;

  INFO_LOG("Initialized evaluation with N: %d, p: %d", data->N, data->p);

  data->started = false;
  data->buffer = assigned_buffer;
  data->segments_received_in_p = 0;
  data->last_output = NULL;

  return data;
}

void print_evaluation_window(evaluation_data* data) {
  for(int i = 0; i < data->N; i++) {

    if(data->buffer->data[i] == 0) continue;

    printf("| %d |", data->buffer->data[i]);
  }
  printf("\n");
}

void* evaluation_task(evaluation_data* data) {

  if(data == NULL){
    exit(1);
  }

  char command[1024]; // output
  generate_evaluation_command(data, command);

  // disable warning messages (from stdout)
  freopen(NULL_DEVICE, "w", stderr);

  //printf("Executing command: %s \n", command);

  long long start = timeInMilliseconds();

  FILE* fp = popen(command, "r");

  char path[2048];

  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  while (fgets(path, sizeof(path), fp) != NULL) {
    //printf("%s", path);
  }

  // re-enable warning messages
  freopen(TTY_DEVICE, "w", stderr);

  cJSON *output_json = cJSON_Parse(path);

  if(output_json == NULL) {
  	 printf("Failed to parse JSON\n" );
  	 exit(1);
  }

  long long end = timeInMilliseconds() - start;

  if((end) > data->p){
    ERROR_LOG("Warning: evaluation time [%f] is greater than p, system may diverge", (float) end / 1000);
  }

  char *output_string = cJSON_Print(output_json);

  if(output_string == NULL) {
    return NULL;
  }

  /*int result = ws_sendframe_txt(1, output_string);

  if(result == -1) {
  	printf("Error sending back the frame");
  	exit(1);
  }*/

  //const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");
  print_buffer(data->buffer);

  if(data->last_output != NULL) {
    free(data->last_output);
  }

  data->last_output = (char*) malloc(sizeof(char) * (strlen(output_string) + 1));
  strcpy(data->last_output, output_string);

  //INFO_LOG("MOS");

  if(data->started)
    slice_buffer(data->buffer, data->buffer->K, (data->p));

  data->segments_received_in_p = 0;

  free(output_string);
  free(output_json);

  return NULL;
}