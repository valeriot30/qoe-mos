/**
Copyright unime.it
**/
#include <stdbool.h>

#include "buffer.h"
#include "timer.h"
#include "cJSON/cJSON.h"
#include "evaluation.h"
#include "ws/ws.h"
#include "ws/base64.h"

const int N = 10;
const int p = 4;

timer* stalling_timer;
timer* evaluation_timer;
buffer* eval_buffer;
evaluation_data* eval_data;

int segments_after_stall = 0;

int last_segment_valid = 0;

const int segment_duration = 2; // in seconds

clock_t start, end;

void* eval_task() {

  if(eval_data == NULL) {
    return NULL;
  }

  //->segments_received_in_p = 0;

  char command[1024]; // output
  generate_evaluation_command(eval_data, command);

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

  if((end / 1000) > segment_duration * eval_data->p){
    ERROR_LOG("Warning: evaluation time [%f] is greater than p, system may diverge", (float) end / 1000);
  }

  char *output_string = cJSON_Print(output_json);

  if(output_string == NULL) {
    return NULL;
  }

  int result = ws_sendframe_txt(1, output_string);

  if(result == -1) {
    printf("Error sending back the frame");
    exit(1);
  }

  const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(output_json, "O46");
  print_buffer(eval_data->buffer);

  INFO_LOG("MOS: %f", O46->valuedouble);

  slice_buffer(eval_data->buffer, eval_data->buffer->K, eval_data->p);

  free(output_string);
  free(output_json);

  return NULL;
}

void* stalling_task() {
  add_element(eval_buffer, -1);
  increment_segments_received(eval_data);

  INFO_LOG("Adding stalling segment to the window");

  return NULL;
}

void* send_mos() {
  if(eval_data->last_output == NULL){
      ERROR_LOG("MOS output is invalid");
      return NULL;
  }

  cJSON* cjson = cJSON_Parse(eval_data->last_output);

  if(cjson == NULL) {
    ERROR_LOG("Cannot parse JSON");
  }

  const cJSON* O46 = cJSON_GetObjectItemCaseSensitive(cjson, "O46");

  INFO_LOG("MOS: %f", O46->valuedouble);

    //int result = ws_sendframe_txt(1, eval_data->last_output);

    //if(result < 0) {
   //   ERROR_LOG("Cannot send MOS to the client");
    //}

  return NULL;
}


bool check_file_mode(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            return true;
        }
    }
    return false;
}


void onopen(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection opened, addr: %s\n", cli);
}

void onclose(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection closed, addr: %s\n", cli);
}

void onmessage(ws_cli_conn_t client,
    const unsigned char *msg, uint64_t size, int type)
{
    //printf("I receive a message: %s (%llu), from: %s\n", msg, size, cli);

    end = clock();

    int counter = get_counter(eval_buffer);

    int p = eval_data->p;
    int N = eval_data->N;
    int segments_received_in_p = eval_data->segments_received_in_p;
    // fix this

    double elapsedTime = 0;

    float seconds = (float)(end - start) / CLOCKS_PER_SEC;

    printf("Expected arrival time of a new segment: %f s \n",(seconds));

    start = clock();

    if(strcmp(msg, "-1") == 0) 
    {
      INFO_LOG("Adding stalling segment to the window");
      resume_timer(stalling_timer);
    }
    else {

      suspend_timer(stalling_timer);
      size_t outlen;


      unsigned char * byteStream = base64_decode(msg, (size_t) size, &outlen);

      /*if(byteStream == NULL) {
        return;
      }*/

      int segmentNumber;
      memcpy(&segmentNumber, byteStream, 4);

      unsigned char *segmentData = byteStream + 4;
      size_t segmentDataSize = outlen - 4;


      printf("Segment: %d \n", segmentNumber);

      if(eval_started(eval_data) && seconds < segment_duration) {
        //INFO_LOG("Skipping segment atoo fast");
      }

      if(segmentNumber < counter) {
        INFO_LOG("Skipping segment after stall");
        //slice_buffer(eval_buffer, 20, 1);
        return;
      }


      char buf[21 + 4];

      snprintf(buf, sizeof(buf), "./segments/segment-%d.ts", segmentNumber);
      //printf("%s", buf);
      
      FILE* fptr;

    	fptr = fopen(buf, "wb+");

    	if(fptr == NULL) {
    		printf("Error opening the file");
    		exit(1);
    	}

    	fwrite(segmentData, sizeof byteStream[0], segmentDataSize, fptr);

    	fclose(fptr);

      if(segmentNumber == (N) && !eval_started(eval_data)) {
       //printf("Evaluation can start\n");
        set_started(eval_data, true);
      }

      add_element(eval_buffer, counter);
      last_segment_valid = counter;
    }

    //print_buffer(eval_buffer);
}

int main(int argc, char** argv) {

	bool file_mode = check_file_mode(argc, argv);

	eval_buffer = create_buffer(100);

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

  stalling_timer = create_timer_cond(stalling_task, 1 * segment_duration, false, true);

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
      timer* timer = create_timer_cond(eval_task, p, false, true);
      resume_timer(timer);
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
	        .thread_loop   = 2,
	        .timeout_ms    = 1000,
	        .evs.onopen    = &onopen,
	        .evs.onclose   = &onclose,
	        .evs.onmessage = &onmessage
	    });

	}

  long long start = 0;
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
  }

	free(eval_buffer);

	return (0);
}
