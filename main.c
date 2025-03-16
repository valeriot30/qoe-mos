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

timer* stalling_timer;
buffer* eval_buffer;
evaluation_data* eval_data;

void* stalling_task() {
  add_element(eval_buffer, -1);
  increment_segments_received(eval_data);

  INFO_LOG("Adding stalling segment to the window");
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

    int counter = get_counter(eval_buffer);

    int p = eval_data->p;
    int N = eval_data->N;
    int segments_received_in_p = eval_data->segments_received_in_p;
    // fix this

    if(strcmp(msg, "-1") == 0) 
    {
      INFO_LOG("Adding stalling segment to the window");
      resume_timer(stalling_timer);
    }
    else {

      /*if(buffer[write_idx] == -1) {
        segments_after_stall++;
      }

      if(segments_after_stall > 0) {
        segments_after_stall++;
      }

      if(segments_after_stall < counter && segments_after_stall > 0) {
        INFO_LOG("Skipping segment after stall");
        return;
      }*/

      suspend_timer(stalling_timer);

      size_t outlen;

      unsigned char * byteStream = base64_decode(msg, (size_t) size, &outlen);

      char buf[21 + 4];

      snprintf(buf, sizeof(buf), "./segments/segment-%d.ts", counter);
      //printf("%s", buf);
      
      FILE* fptr;

    	fptr = fopen(buf, "wb+");

    	if(fptr == NULL) {
    		printf("Error opening the file");
    		exit(1);
    	}

    	fwrite(byteStream, sizeof byteStream[0], outlen, fptr);

    	fclose(fptr);

    	INFO_LOG("Segment: %d", counter);


     //pthread_mutex_lock(&lock); 
     //pthread_mutex_unlock(&lock);


      if(counter == (N) && !eval_started(eval_data)) {
       //printf("Evaluation can start\n");
        evaluation_task(eval_data);
        set_started(eval_data, true);
      }

      add_element(eval_buffer, counter);
    }

    increment_segments_received(eval_data);

    if(segments_received_in_p == p && eval_started(eval_data)) {
      evaluation_task(eval_data);
    }

    print_buffer(eval_buffer);
}

int main(int argc, char** argv) {

	bool file_mode = check_file_mode(argc, argv);

	eval_buffer = create_buffer(20);

  if(eval_buffer == NULL) {
    perror("error allocating memory");
    ERROR_LOG("There was an error with the allocation of the array \n");
    return 1;
  }

  int N = 12;
  int p = 4;

  eval_data = create_evaluation_data(N, p, eval_buffer);

  if(eval_data == NULL) {
    perror("error allocating memory");
    ERROR_LOG("There was an error with the allocation of the evaluation data\n");
    return 1;
  }

  stalling_timer = create_timer_cond(stalling_task, 1000, false, true);

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
		
		fclose (fptr);  
	} 
	else {

		int n = get_counter(eval_buffer);
		printf("The length of the buffer is %d\n", (int) n);  

		printf("Starting evaluation...\n");

		ws_socket(&(struct ws_server){
	        /*
	         * Bind host, such as:
	         * localhost -> localhost/127.0.0.1
	         * 0.0.0.0   -> global IPv4
	         * ::        -> global IPv4+IPv6 (Dual stack)
	         */
	        .host = "localhost",
	        .port = 3000,
	        .thread_loop   = 1,
	        .timeout_ms    = 1000,
	        .evs.onopen    = &onopen,
	        .evs.onclose   = &onclose,
	        .evs.onmessage = &onmessage
	    });

	}
	while(1);

	free(eval_buffer);

	return 0;
}
