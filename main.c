/**
Copyright unime.it
**/
#include <stdbool.h>

#include "buffer.h"
#include "evaluation.h"
#include "lib/json/cJSON.h"
#include "timer.h"
#include "lib/ws/base64.h"
#include "lib/ws/ws.h"

#define USE_MULTITHREADING_FOR_WS 0

timer* stalling_timer;
timer* evaluation_timer;
buffer* eval_buffer;
evaluation_data* eval_data;

int segments_after_stall = 0;

int last_segment_valid = 0;

bool running = true;

clock_t start;
clock_t end;

bool is_stalling = false;

int p_counter = 0;

bool discarding_segments = false;

int gaps_counter = 0;

int stall_counter = 0;

void* eval_task() {

  // local variables to this function
  cJSON* output_json;
  char* output_string;

  if (eval_data == NULL) {
    return NULL;
  }


  if(!is_one_step(eval_data))
  {
    char command[BUFFER_CMD_SIZE];  // output
    int size = generate_evaluation_command(eval_data, command);

    if (size == 0) {
      if (is_stalling) {
        // is_stalling = true;

        INFO_LOG("The window is full of stalls, giving MOS=1 as result");
        cJSON* result = cJSON_CreateObject();

        if (result == NULL) {
          ERROR_LOG("failed creating the result object");
          return NULL;
        }

        cJSON* output = NULL;
        output = cJSON_CreateString("1");
        cJSON_AddItemToObject(result, "O46", output);

        char* output_string = cJSON_Print(result);

        if (ws_sendframe_txt(1, output_string) == -1) {
          ERROR_LOG("Error sending back the frame");
          return NULL;
        }
      }

      goto slice_buffer;
    }

    // disable warning messages (from stdout)
    freopen(NULL_DEVICE, "w", stderr);

    // printf("Executing command: %s \n", command);

    long long start = timeInMilliseconds();

    FILE* fp = popen(command, "r");

    char path[BUFFER_CMD_SIZE * 2];

    if (fp == NULL) {
      printf("Failed to run command\n");
      // exit(1);
      return NULL;
    }

    while (fgets(path, sizeof(path), fp) != NULL) {
      // printf("%s", path);
    }

    printf("%s", path);

    // re-enable warning messages
    freopen(TTY_DEVICE, "w", stderr);

    output_json = cJSON_Parse(path);

    if (output_json == NULL) {
      printf("Failed to parse JSON\n");
      return NULL;
    }

    long long end = timeInMilliseconds() - start;

    if ((end / 1000) > eval_data->buffer->duration * eval_data->p) {
      ERROR_LOG(
          "Warning: evaluation time [%fs] is greater than duration * p [%ds], "
          "system may diverge",
          ((float)end / 1000), eval_data->buffer->duration * eval_data->p);
    }
  }
  else
  {
     float avg_mos = float_array_mean(eval_data->buffer->data, eval_data->N);

     INFO_LOG("%f", avg_mos);

     output_json = cJSON_CreateObject();

     cJSON* o46 = NULL;

     o46 = cJSON_CreateNumber(avg_mos);

     cJSON_AddItemToObject(output_json, "O46", o46);
  }


    int eval_nf = eval_data->nf;
    int eval_tft = eval_data->tft;
    int eval_tfm = eval_data->tfm;

    cJSON* nf = NULL;
    cJSON* tfm = NULL;
    cJSON* tft = NULL;

    tfm = cJSON_CreateNumber(eval_tfm);
    nf = cJSON_CreateNumber(eval_nf);
    tft = cJSON_CreateNumber(eval_tft);

    cJSON_AddItemToObject(output_json, "TFM", tfm);
    cJSON_AddItemToObject(output_json, "TFT", tft);
    cJSON_AddItemToObject(output_json, "NF", nf);

  output_string = cJSON_Print(output_json);

  if (output_string == NULL) {
    ERROR_LOG("Invalid output string");
    return NULL;
  }

  int result = ws_sendframe_txt(1, output_string);

  if (result == -1) {
    printf("Error sending back the frame");
    return NULL;
  }

  const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");

  if (O46 == NULL) {
    ERROR_LOG("Cannot get MOS value");
  }

  print_buffer(eval_data->buffer);

  INFO_LOG("MOS: %f", O46->valuedouble);

  free(output_string);
  free(output_json);

slice_buffer:
  if (eval_data->started) {
    slice_buffer(eval_data->buffer, eval_data->buffer->K, eval_data->p);
  }

  return NULL;
}

void* stalling_task() {
  INFO_LOG("Adding stalling segment in the buffer..");
  add_element(eval_buffer, is_one_step(eval_data) ? 1 : -1);
  stall_counter++;

  return NULL;
}

void onopen(ws_cli_conn_t client) {
  char* cli;
  cli = ws_getaddress(client);
  INFO_LOG("Connection opened, addr: %s\n", cli);
}

void onclose(ws_cli_conn_t client) {
  char* cli;
  cli = ws_getaddress(client);
  INFO_LOG("Connection closed, addr: %s\n", cli);
}

void onmessage(ws_cli_conn_t client, const unsigned char* msg, uint64_t size,
               int type) {
  // printf("I receive a message: %s (%llu), from: %s\n", msg, size, cli);

  end = clock();

  int counter = get_counter(eval_buffer);

  int p = eval_data->p;
  int N = eval_data->N;
  // fix this

  float seconds = (float)(end - start) / CLOCKS_PER_SEC;

  INFO_LOG("Arrival time of a new segment: %f s", (seconds));

  add_arrival_time(eval_data, seconds);

  start = clock();

  // fprintf(stderr, "%s", msg);

  if (strncmp(msg, "-1", size) == 0) {
    // add_element(eval_buffer, -1);
    INFO_LOG("Adding stalling segment to the window \n");
    add_element(eval_buffer, is_one_step(eval_data) ? 1 : -1);
    increment_nf(eval_data);
    is_stalling = true;
    stall_counter++;
  } else {
    /**
     * here starts the discarding segments mechanism
     */

    /*
    if(is_stalling) {
      discarding_segments = true;
    }

    if(discarding_segments) {
      gaps_counter++;
    }

    if(discarding_segments)
    {
      if(gaps_counter < stall_counter) {
        ERROR_LOG("Discarding %d segment after stall", gaps_counter);
        return;
      }
      else {
        discarding_segments = false;
        gaps_counter = 0;
        stall_counter = 0;
      }
    }*/

    // this means that we received a valid segment (buffer is not stalling
    // anymore)
    is_stalling = false;
    size_t outlen;

    unsigned char* byteStream = base64_decode(msg, (size_t)size, &outlen);

    /*if(byteStream == NULL) {
      return;
    }*/

    // fix the endianess
    int segmentNumber;
    memcpy(&segmentNumber, byteStream, SEGMENT_NUMBER_BYTES);

    unsigned char* segmentData = byteStream + SEGMENT_NUMBER_BYTES;
    size_t segmentDataSize = outlen - SEGMENT_NUMBER_BYTES;

    INFO_LOG("Segment: %d", segmentNumber);

    if (eval_started(eval_data) && seconds < eval_data->buffer->duration) {
      // INFO_LOG("Skipping segment atoo fast");
    }

    /*if(segmentNumber < counter) {
      INFO_LOG("Skipping segment after stall");
      //slice_buffer(eval_buffer, 20, 1);

      // Problem: When the system loads other segments after a stall, segments
    will be
      // appended in the buffer but what happens if the player skips all these
    segments?
      // 1. We can skip segments up to the segment counter hold by the server
      // 2. We can slice the buffer of k segments in order to re-syncronize with the
    playout buffer of the player

      return;
    }*/

    int result = store_segment(segmentNumber, segmentData, segmentDataSize);

    if (result < 0) {
      ERROR_LOG("Error creating the segment");
      return;
    }

    if (segmentNumber == (N) && !eval_started(eval_data)) {
      INFO_LOG("Evaluation can start\n");
      set_started(eval_data, true);
    }


    if(is_one_step(eval_data))
    {
      char output[BUFFER_CMD_SIZE * 2];

      int result = evaluate_segment(eval_data, segmentNumber, output);

      if(result == -1) {
        ERROR_LOG("Error evaluating segment %d", segmentNumber);
      }

      cJSON* output_json = cJSON_Parse(output);

      const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");

      INFO_LOG("%f", O46->valuedouble);

      add_element(eval_buffer, O46->valuedouble);
    }
    else
    {
      add_element(eval_buffer, (int) segmentNumber);
    }
    last_segment_valid = counter;
  }

  // print_buffer(eval_buffer);
}

void print_usage() {
  fprintf(
      stdout,
      "--------- EVALUATION SYSTEM -------- \n usage: ./main [N] [p] [d] [one_step] [port] [mode] \n N: "
      "the window size \n p: the window step \n d: the segments duration\n one_step: choose if you want to evaluate N segments in the window or 1 segment at time and then average N MOS \n port: The listening port of the evaluation system \n"
      " mode: the mode of operation for the ITU standard \n"
      "\n usage: ./main [config_file] \n"  
      " [config_file] is a json file containing all the parameters. See config.example.json \n"
      );
}

int main(int argc, char** argv) {

  if(argc < 2) {
    print_usage();
    exit(1);
  }

  uint8_t N = 0;
  uint8_t p = 0;
  uint8_t d = 0;
  uint32_t port = 0;
  uint8_t one_step_mode = 0;

  int mode = 0;

  const char *filename = argv[1];
  const char *ext = strrchr(filename, '.');

  if(ext != NULL && strcmp(ext, ".json") == 0)
  {
    char buffer[1024];

    FILE *fptr = fopen(filename, "r");

    if(fptr == NULL) {
      ERROR_LOG("Cannot find file");
    }
    int len = fread(buffer, 1, sizeof(buffer), fptr); 
      
    fclose(fptr);

    cJSON *config_file = cJSON_Parse(buffer);

    if (config_file == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            ERROR_LOG("Error before: %s\n", error_ptr);
        }
        cJSON_Delete(config_file); 
        exit(1);
    }

    const cJSON *Nobj = cJSON_GetObjectItemCaseSensitive(config_file, "N");
    const cJSON *pobj = cJSON_GetObjectItemCaseSensitive(config_file, "p");
    const cJSON *dobj = cJSON_GetObjectItemCaseSensitive(config_file, "d");
    const cJSON *portobject = cJSON_GetObjectItemCaseSensitive(config_file, "port");
    const cJSON *onestepobject= cJSON_GetObjectItemCaseSensitive(config_file, "one_step");
    const cJSON *modeobject= cJSON_GetObjectItemCaseSensitive(config_file, "mode");

    if(cJSON_IsNumber(Nobj) && Nobj->valueint > 0)
      N = Nobj->valueint;

    if(cJSON_IsNumber(pobj) && dobj->valueint > 0)
      d = dobj->valueint;

    if(cJSON_IsNumber(dobj) && pobj->valueint > 0)
      p = pobj->valueint;

    if(cJSON_IsNumber(portobject) && portobject->valueint > 0)
      port = portobject->valueint;

    if(cJSON_IsNumber(onestepobject) && onestepobject->valueint != NULL)
      one_step_mode = onestepobject->valueint;

    if(cJSON_IsNumber(modeobject) && modeobject->valueint != NULL)
      mode = modeobject->valueint;
  }
  else 
  {

    if (argc < 7) {
      print_usage();
      exit(1);
    }

    N = argc >= 2 ?              (int)atoi(argv[1]) : DEFAULT_N;
    p = argc >= 3 ?              (int)atoi(argv[2]) : DEFAULT_P;
    d = argc >= 4 ?              (int)atoi(argv[3]) : DEFAULT_D;
    one_step_mode =  argc >= 5 ? (int)atoi(argv[4]) : 0;
    port = argc >= 6 ?           (int)atoi(argv[5]) : 0;
    mode = argc >= 7 ?           (int)atoi(argv[6]) : 0;

    if(mode > 3 || mode < 0) {
      mode = 0;
    } 
  }


  eval_buffer = create_buffer(1000, d);

  if (eval_buffer == NULL) {
    perror("error allocating memory");
    ERROR_LOG("There was an error with the allocation of the array");
    return 1;
  }

  eval_data = create_evaluation_data(N, p, one_step_mode, mode, eval_buffer);

  if (eval_data == NULL) {
    perror("error allocating memory");
    ERROR_LOG(
        "There was an error with the allocation of the evaluation data");
    return 1;
  }

  INFO_LOG("Initialized evaluation with N: %d, p: %d, d: %d in mode %d", N, p, d, mode);

  stalling_timer = create_timer_cond(stalling_task, get_segments_duration(eval_buffer), &(is_stalling), true);
  evaluation_timer = create_timer_cond(eval_task, p * get_segments_duration(eval_buffer),
                        &(eval_data->started), true);

  if (evaluation_timer == NULL) {
    ERROR_LOG("Error creating the timer");
    exit(1);
  }

  INFO_LOG("Starting evaluation...\n");

  start = clock();

  INFO_LOG("Listening on port %d", port);

  ws_socket(&(struct ws_server){/*
                                   * Bind host, such as:
                                   * localhost -> localhost/127.0.0.1
                                   * 0.0.0.0   -> global IPv4
                                   * ::        -> global IPv4+IPv6 (Dual stack)
                                   */
                                  .host = "localhost",
                                  .port = port,
                                  .thread_loop = USE_MULTITHREADING_FOR_WS,
                                  .timeout_ms = 1000,
                                  .evs.onopen = &onopen,
                                  .evs.onclose = &onclose,
                                  .evs.onmessage = &onmessage});
  timer_join(stalling_timer);
  timer_join(evaluation_timer);

  /*long long start = 0;
  long long end = 0;

  while(1) {
    if(eval_data->started) {

      if(start > 0)
        end = timeInMilliseconds() - start;

      printf("Time took: %lld ms \n", end);

      eval_task();
      start = timeInMilliseconds();
      sleep(p * segment_duration);
    }
  }*/

  free(eval_buffer);
  free(eval_data);

  return (0);
}
