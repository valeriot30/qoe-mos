#include "utils.h"


long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

bool check_file_mode(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            return true;
        }
    }
    return false;
}

// TODO improve this using O(1) and bit ops
float float_array_mean(float* array, size_t size) {

    int sum = 0;

    for(int i = 0; i < size; i++) {
        sum += array[i];
    }

    return sum / (int) size;
}

int store_segment(int segmentNumber, unsigned char* segmentData, unsigned int segmentDataSize) {
    char buf[FILE_PATH_SIZE + sizeof(int)];

    snprintf(buf, sizeof(buf), "./segments/segment-%d.ts", segmentNumber);
      //printf("%s", buf);
      
    FILE* fptr;

    fptr = fopen(buf, "wb+");

    if(fptr == NULL) {
        printf("Error opening the file");
        return -1;
    }

    fwrite(segmentData, sizeof segmentData[0], segmentDataSize, fptr);

    fclose(fptr);
}
