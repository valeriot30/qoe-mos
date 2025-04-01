#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include<sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define FILE_PATH_SIZE 30

long long timeInMilliseconds(void);
bool check_file_mode(int argc, char *argv[]);
int store_segment(int segmentNumber, unsigned char* segmentData, unsigned int segmentDataSize);
#endif