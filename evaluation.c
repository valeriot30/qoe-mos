
#include "evaluation.h"

  
void increment_nf(evaluation_data* data) {
  data->nf++;
}

bool is_one_step(evaluation_data* data) {
  return data->one_step;
}

int get_mode(evaluation_data* data) {
  return data->mode;
}

/**
 * Note: duration must be a multiple of duration inside buffer.h
 * 
 * 
*/
void add_stall_duration(evaluation_data* data, int duration) {
  //TODO check if duration is a multiple

  data->tft += duration;

  if(duration > data->tfm) {
    data->tfm = duration;
  }
}

int evaluate_segment(evaluation_data* data, int segmentNumber, char* output) {
      char command[BUFFER_CMD_SIZE];  // output
      strcpy(command, "python3 evaluate.py ");
      char buf[FILE_PATH_SIZE + sizeof(int)];
      snprintf(buf, sizeof(buf), "./segments/segment-%d.ts ", segmentNumber);
      strcat(command, buf);

      memcpy(output, command, sizeof(command));

      freopen(NULL_DEVICE, "w", stderr);

      //long long start = timeInMilliseconds();

      FILE* fp = popen(command, "r");

      char path[BUFFER_CMD_SIZE * 2];

      if (fp == NULL) {
        printf("Failed to run command\n");
        // exit(1);
        return -1;
      }

      while (fgets(path, sizeof(path), fp) != NULL) {
        // printf("%s", path);
      }

      // re-enable warning messages
      freopen(TTY_DEVICE, "w", stderr);

      char buffs[16];

      snprintf(buffs, sizeof(buffs), "-m %d", get_mode(data));

      strcat(command, buffs);

      //printf("%s", command);

      memcpy(output, path, sizeof(path));

      return 1;
}

int generate_evaluation_command(evaluation_data* data, char* output) {

  float* buffer = data->buffer->data;

  int K = get_buffer_size(data->buffer);

  stall stalls[K];

  for(int i = 0; i < K; i++) {
    stalls[i].start = -1;
    stalls[i].duration = -1;
  }
  bool stall_occurred = false;
  int current_stall_position = 0;

  current_stall_position++;

  char command[1024];

  int size = 0;

  strcpy(command, "python3 evaluate.py ");
  for (int j = 0; j < data->N; j++) 
  {

    if(buffer[j] == 0) continue;

    char buf[FILE_PATH_SIZE + sizeof(int)];

    if (buffer[j] == -1 && !stall_occurred) {
      stalls[current_stall_position].start = (j + 1);
      stalls[current_stall_position].duration = data->buffer->duration;
      stall_occurred = true;
      continue;
    } else if (buffer[j] == -1 && stall_occurred) {
      stalls[current_stall_position].duration = stalls[current_stall_position].duration + data->buffer->duration;
      stall_occurred = true;
      continue;
    } else if (buffer[j] != -1 && stall_occurred) {
      //increment_nf(data);

      stall current_stall = stalls[current_stall_position];

      // todo check new window 
      add_stall_duration(data, current_stall.duration);
      stall_occurred = false;
      current_stall_position++;
    }
    snprintf(buf, sizeof(buf), "./segments/segment-%d.ts ", (int) buffer[j]);
    strcat(command, buf);
    size++;
  }

  if(size == 0) return size;

  strcat(command, "-s ");

  char tmp[256];

  stalls_to_string(stalls, K, tmp, 256);

  printf("%s \n ", tmp);

  strcat(command, tmp);

  char buf[16];

  snprintf(buf, sizeof(buf), "-m %d", get_mode(data));

  strcat(command, buf);

  //printf("%s", command);

  memcpy(output, command, sizeof(command));

  return size;
}

void add_arrival_time(evaluation_data* data, float arrival_time) {
  int counter = get_counter(data->buffer);

  data->arrivals[counter] = arrival_time;

  INFO_LOG("Avg arrival time: %f", float_array_mean(data->arrivals, counter));
}

void add_departure_time(evaluation_data* data, float departure_time) {
  int counter = get_counter(data->buffer);

  data->departures[counter] = departure_time;

  INFO_LOG("Avg departure time: %f", float_array_mean(data->departures, counter));
}

void set_started(evaluation_data* data, bool state) {
  data->started = state;
}

bool eval_started(evaluation_data* data) {
  return data->started;
}

/**
 * one step is for evaluating the segment one at time, e.g you evaluate one segment and you store the MOS
 * in the buffer, then you you evaluate N MOS inside the buffer by averaging
**/
evaluation_data* create_evaluation_data(int N, int p, bool one_step, int mode, buffer* assigned_buffer) {
  evaluation_data* data = (evaluation_data*) malloc(sizeof(struct evaluation_data));

  if(data == NULL) {
    return NULL;
  }

  data->N = N;
  data->p = p;

  data->nf = 0;
  data->tfm = 0;
  data->tft = 0;
  data->arrivals = calloc(200, sizeof(float));
  data->departures = calloc(200, sizeof(float));

  data->started = false;
  data->one_step = one_step;
  data->mode = mode;
  data->buffer = assigned_buffer;
  data->last_output = NULL;

  return data;
}

void print_evaluation_window(evaluation_data* data) {
  for(int i = 0; i < data->N; i++) {

    if(data->buffer->data[i] == 0) continue;

    printf("| %f |", data->buffer->data[i]);
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

  //data->segments_received_in_p = 0;

  free(output_string);
  free(output_json);

  return NULL;
}
