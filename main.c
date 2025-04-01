/**
Copyright unime.it
**/
#include <stdbool.h>

#include "buffer.h"
#include "timer.h"
#include "json/cJSON.h"
#include "evaluation.h"
#include "ws/ws.h"
#include "ws/base64.h"

#define USE_MULTITHREADING_FOR_WS 1

int N = 0;
int p = 0;

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

void* eval_task() {

  if(eval_data == NULL) {
    return NULL;
  }

  p_counter += 1;

  if(is_stalling) {
    INFO_LOG("Adding stalling segment in the buffer..");
    add_element(eval_buffer, -1);
  }

  if(p_counter < p) {
    return NULL;
  }

  char command[BUFFER_CMD_SIZE]; // output
  int size = generate_evaluation_command(eval_data, command);

  if(size == 0) 
  {
    if(is_still_stalling(eval_data->buffer))
    {
      is_stalling = true;

      INFO_LOG("The window is full of stalls, giving MOS=1 as result");
      cJSON *result = cJSON_CreateObject();

      if(result == NULL) {
        printf("failed creating the result object");
        return NULL;
      }

      cJSON *output = NULL; 
      output = cJSON_CreateString("1");
      cJSON_AddItemToObject(result, "O46", output);

      char* output_string = cJSON_Print(result);

      if(ws_sendframe_txt(1, output_string) == -1) {
        printf("Error sending back the frame");
        return NULL;
      }
    }

    return NULL;  
  }

  // disable warning messages (from stdout)
  freopen(NULL_DEVICE, "w", stderr);

  //printf("Executing command: %s \n", command);

  long long start = timeInMilliseconds();

  FILE* fp = popen(command, "r");

  char path[BUFFER_CMD_SIZE * 2];

  if (fp == NULL) {
    printf("Failed to run command\n" );
    //exit(1);
    return NULL;
  }

  while (fgets(path, sizeof(path), fp) != NULL) {
    //printf("%s", path);
  }

  // re-enable warning messages
  freopen(TTY_DEVICE, "w", stderr);

  cJSON *output_json = cJSON_Parse(path);

  if(output_json == NULL) {
     printf("Failed to parse JSON\n" );
     return NULL;
  }

  long long end = timeInMilliseconds() - start;

  if((end / 1000) > eval_data->buffer->duration * eval_data->p){
    ERROR_LOG("Warning: evaluation time [%f] is greater than p, system may diverge", (float) end / 1000);
  }

  char *output_string = cJSON_Print(output_json);

  if(output_string == NULL) {
    ERROR_LOG("Invalid output string");
    return NULL;
  }

  int result = ws_sendframe_txt(1, output_string);

  if(result == -1) {
    printf("Error sending back the frame");
    return NULL;
  }

  const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");
  print_buffer(eval_data->buffer);

  INFO_LOG("MOS: %f", O46->valuedouble);

  if(eval_data->started)
  {

    slice_buffer(eval_data->buffer, eval_data->buffer->K, eval_data->p);
  }

  p_counter = 0;

  free(output_string);
  free(output_json);

  return NULL;
}

void* stalling_task() {
   INFO_LOG("Adding stalling segment in the buffer..");
   add_element(eval_buffer, -1);

   return NULL;
}


void onopen(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    INFO_LOG("Connection opened, addr: %s\n", cli);
}

void onclose(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    INFO_LOG("Connection closed, addr: %s\n", cli);
}

void onmessage(ws_cli_conn_t client,
    const unsigned char *msg, uint64_t size, int type)
{
    //printf("I receive a message: %s (%llu), from: %s\n", msg, size, cli);

    end = clock();

    int counter = get_counter(eval_buffer);

    int p = eval_data->p;
    int N = eval_data->N;
    // fix this

    float seconds = (float)(end - start) / CLOCKS_PER_SEC;

    printf("Expected arrival time of a new segment: %f s \n",(seconds));

    start = clock();

    if(strcmp(msg, "-1") == 0) 
    {
      INFO_LOG("Adding stalling segment to the window");
      is_stalling = true;
    }
    else {

      // this means that we received a valid segment (buffer is not stalling anymore)
      is_stalling = false;
      size_t outlen;


      unsigned char * byteStream = base64_decode(msg, (size_t) size, &outlen);

      /*if(byteStream == NULL) {
        return;
      }*/

      // fix the endianess
      int segmentNumber;
      memcpy(&segmentNumber, byteStream, SEGMENT_NUMBER_BYTES);

      unsigned char *segmentData = byteStream + SEGMENT_NUMBER_BYTES;
      size_t segmentDataSize = outlen - SEGMENT_NUMBER_BYTES;


      printf("Segment: %d \n", segmentNumber);

      if(eval_started(eval_data) && seconds < eval_data->buffer->duration) {
        //INFO_LOG("Skipping segment atoo fast");
      }

      /*if(segmentNumber < counter) {
        INFO_LOG("Skipping segment after stall");
        //slice_buffer(eval_buffer, 20, 1);

        // Problem: When the system loads other segments after a stall, segments will be
        // appended in the buffer but what happens if the player skips all these segments?
        // 1. We can skip segments up to the segment counter hold by the server
        // 2. We can slice the buffer of 1 in order to re-syncronize with the playout buffer of the player 

        return;
      }*/


      int result = store_segment(segmentNumber, segmentData, segmentDataSize);

      if(result < 0) {
        ERROR_LOG("Error creating the segment");
        return;
      }

      if(segmentNumber == (N) && !eval_started(eval_data)) {
        printf("Evaluation can start\n");
        set_started(eval_data, true);
      }

      add_element(eval_buffer, segmentNumber);
      last_segment_valid = counter;
    }

    //print_buffer(eval_buffer);
}

void print_usage() {
    fprintf(stdout,"--------- EVALUATION SYSTEM -------- \n usage: ./main [N] [p] \n N: the window size \n p: the window step \n");
}

int main(int argc, char** argv) {

	bool file_mode = check_file_mode(argc, argv);

	eval_buffer = create_buffer(100);

  if(argc < 3) {
    print_usage();
    exit(1);
  }

  N = argc >= 2 ? (int) atoi(argv[1]) : DEFAULT_N;
  p = argc >= 3 ? (int) atoi(argv[2]) : DEFAULT_P;

  if(eval_buffer == NULL) {
    perror("error allocating memory");
    ERROR_LOG("There was an error with the allocation of the array \n");
    return 1;
  }

  eval_data = create_evaluation_data(N, p, eval_buffer);

  if(eval_data == NULL) {
    perror("error allocating memory");
    ERROR_LOG("There was an error with the allocation of the evaluation data\n");
    return 1;
  }

  //stalling_timer = create_timer_cond(stalling_task, get_segments_duration(eval_buffer), &(is_stalling), true);

  evaluation_timer = create_timer_cond(eval_task, get_segments_duration(eval_buffer), &(eval_data->started), true);

  if(evaluation_timer == NULL) {
    ERROR_LOG("Error creating the timer");
    exit(1);
  }

	if(file_mode) {
		FILE* fptr;

		fptr = fopen("buffer.txt", "r");

		if(fptr == NULL)
		{
		    printf("Unable to open the file \n");
		    return 1;
		}
		
		fflush(stdin); 
		int i = 0;
		while (!feof (fptr))
		{  
		    fscanf (fptr, "%d", &i);   
        add_element(eval_buffer, i);
		   	printf ("%d \n", i);
		}

    if(get_counter(eval_buffer) >= N)
    {
      printf("Started evaluation \n");
      create_timer_cond(eval_task, p, false, true);
    }

		fclose (fptr); 

    while(1); 
	} 
	else {

		int n = get_counter(eval_buffer);
		printf("The length of the buffer is %d\n", (int) n);  

		printf("Starting evaluation...\n");

    start = clock();

		ws_socket(&(struct ws_server){
	        /*
	         * Bind host, such as:
	         * localhost -> localhost/127.0.0.1
	         * 0.0.0.0   -> global IPv4
	         * ::        -> global IPv4+IPv6 (Dual stack)
	         */
	        .host = "localhost",
	        .port = 3000,
	        .thread_loop   = USE_MULTITHREADING_FOR_WS,
	        .timeout_ms    = 1000,
	        .evs.onopen    = &onopen,
	        .evs.onclose   = &onclose,
	        .evs.onmessage = &onmessage
	    });

	}  

  while(1);

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
  
  //timer_join(stalling_timer);
  timer_join(evaluation_timer);


	free(eval_buffer);
  free(eval_data);

	return (0);
}
