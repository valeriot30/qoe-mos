
#include "evaluation.h"

static inline void generate_evaluation_command(evaluation_data* data, char* output) {

  int* buffer = data->buffer->data;

  int K = get_buffer_size(data->buffer);

  stall stalls[K];

  for(int i = 0; i < K; i++) {
    stalls[i].start = -1;
    stalls[i].duration = -1;
  }
  bool stall_occurred;
  int current_stall_position = 0;

  char command[1024];

  strcpy(command, "python3.10 evaluate.py ");
  for (int j = 0; j < data->N; j++) 
  {

    if(buffer[j] == -1) continue;

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
  data->started = false;
  data->buffer = assigned_buffer;
  data->segments_received_in_p = 0;

  return data;
}

void* evaluation_task(evaluation_data* data) {

  data->segments_received_in_p = 0;

  char command[1024]; // output
  generate_evaluation_command(data, command);

  // disable warning messages (from stdout)
  freopen(NULL_DEVICE, "w", stderr);

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

  char *output_string = cJSON_Print(output_json);

  int result = ws_sendframe_txt(1, output_string);

  if(result == -1) {
  	printf("Error sending back the frame");
  	exit(1);
  }

  const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");
  print_buffer(data->buffer);

  INFO_LOG("MOS");

  slice_buffer(data->buffer, data->N, data->p);

  free(output_string);
  free(output_json);

  return NULL;
}