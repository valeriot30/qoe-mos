/**
Copyright unime.it
**/
#include <stdbool.h>

#include "buffer.h"
#include "timer.h"
#include "cJSON/cJSON.h"
#include <semaphore.h>
#include "evaluation.h"
#include "ws/ws.h"
#include "ws/base64.h"

const int N = 10;
const int p = 4;

#define USE_MULTITHREADING_FOR_WS 1

timer* stalling_timer;
timer* evaluation_timer;
buffer* eval_buffer;
evaluation_data* eval_data;

int segments_after_stall = 0;

int last_segment_valid = 0;

sem_t sem;
sem_t sem_stalls;

pthread_t thread_exec, thread_signal, stalling_thread, thread_send_stalling;

bool running = true;

clock_t start;
clock_t end;

bool is_stalling = false;

void* eval_task() {

  if(eval_data == NULL) {
    return NULL;
  }

  //->segments_received_in_p = 0;

  char command[BUFFER_CMD_SIZE]; // output
  int size = generate_evaluation_command(eval_data, command);

  if(size == 0) {
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
    slice_buffer(eval_data->buffer, eval_data->buffer->K, eval_data->p);

  free(output_string);
  free(output_json);

  return NULL;
}

void* thread_send_signal(void* arg) {
    while(1) {
        if(eval_data->started)
        {
          //printf("Thread 1: Waking up and signaling thread 2.\n");
          sem_post(&sem);
          sleep(p * eval_data->buffer->duration);
        }
    }
    
    running = false;
    sem_post(&sem);

    return NULL;
}

void* thread_execute_eval(void* arg) {
    while (running) {
      sem_wait(&sem);
      eval_task();

      //printf("Thread 2: Received signal and processing.\n");
    }
    //printf("Thread 2: Exiting.\n");
    return NULL;
}

void* thread_send_stall(void* arg) {
  while(1) {
    if(is_stalling) {
      sem_post(&sem_stalls);
      sleep(eval_data->buffer->duration);
    }
  }

  sem_post(&sem_stalls);  
  return NULL;
}

void* stalling_task() {
  while(1)
  {
    if(is_stalling)
    {
      sem_wait(&sem_stalls);
      add_element(eval_buffer, -1);
      increment_segments_received(eval_data);

      INFO_LOG("Adding stalling segment to the window");
    }
  }

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

      if(segmentNumber < counter) {
        INFO_LOG("Skipping segment after stall");
        //slice_buffer(eval_buffer, 20, 1);
        return;
      }


      char buf[FILE_PATH_SIZE + sizeof(int)];

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
        printf("Evaluation can start\n");
        set_started(eval_data, true);
      }

      add_element(eval_buffer, segmentNumber);
      last_segment_valid = counter;
    }

    //print_buffer(eval_buffer);
}

int main(int argc, char** argv) {

	bool file_mode = check_file_mode(argc, argv);

	eval_buffer = create_buffer(100);

  sem_init(&sem, 0, 0);
  sem_init(&sem_stalls, 0, 0);

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

  pthread_create(&thread_send_stalling, NULL, thread_send_stall, NULL);

  pthread_create(&stalling_thread, NULL, stalling_task, NULL);

  pthread_create(&thread_signal, NULL, thread_send_signal, NULL);

  pthread_create(&thread_exec, NULL, thread_execute_eval, NULL);

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

 
    //printf("Thread 2: Exiting.\n");

  pthread_join(thread_send_stalling, NULL);
  pthread_join(stalling_thread, NULL);
  pthread_join(thread_signal, NULL);
  pthread_join(thread_exec, NULL);


	free(eval_buffer);

	return (0);
}
