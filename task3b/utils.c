#include "utils.h"
#include <time.h>
#include "./cacheutils.h"

// DO NOT MODIFY THIS FUNCTION
double check_accuracy_occupancy(char* received_msg, int received_msg_size){
    // FILE *fp = fopen(MSG_FILE, "r");
    // if(fp == NULL){
    //     printf("Error opening file\n");
    //     return 1;
    // }

    char original_msg[MAX_MSG_SIZE] = "shared_file.txt";
    int original_msg_size = 16;
    // char c;
    // while((c = fgetc(fp)) != EOF){
    //     original_msg[original_msg_size++] = c;
    // }
    // fclose(fp);

    int min_size = received_msg_size < original_msg_size ? received_msg_size : original_msg_size;

    int error_count = (original_msg_size - min_size) * 8;
    for(int i = 0; i < min_size; i++){
        char xor_result = received_msg[i] ^ original_msg[i];
        for(int j = 0; j < 8; j++){
            if((xor_result >> j) & 1){
                error_count++;
            }
        }
    }

    return 1-(double)error_count / (original_msg_size * 8);
}

// All below functions are used for Cache Occupancy placed here

// GLOBAL declaration
#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(long long)) // 24MB   
#define STRIDE 1
#define BITS_PER_CHAR 8

// Declarations made for pointer chasing list
long long arr[ARRAY_SIZE];
int next_index[ARRAY_SIZE / STRIDE]; 

void synchronize_processes_occupancy() {
    size_t curr_time = rdtscp();
    size_t total_duration = 2000000000;
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 15000))  { 
    	curr_time = rdtscp();
    }
}

void synchronize_processes_fr() {
    size_t curr_time = rdtscp();
    size_t total_duration = 5000000;
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 10000))  { 
    	curr_time = rdtscp();
    }
}

// Shuffling
void shuffle_indices(int *indices, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

// Initialize pointer chasing
void initialize_pointer_chase() {
    for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
        next_index[i] = i * STRIDE;
    }
    shuffle_indices(next_index, ARRAY_SIZE / STRIDE);
}

// Pointer chasing function
// void pointer_chasing(long duration_ns) {
//     volatile long long sum = 0;
//     int index = 0;
//     struct timespec start, end;

//     clock_gettime(CLOCK_MONOTONIC, &start);
//     do {
//         // for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
//             index = next_index[index];
//             sum += arr[index];
//         // }
//         clock_gettime(CLOCK_MONOTONIC, &end);
//     } while ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec) < duration_ns);
// }

long long pointer_chasing(long duration_ns) {
    volatile long long sum = 0;
    int index = 0;
    struct timespec start, end;
    long long access_count = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        // for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
            index = next_index[index];
            sum += arr[index];
            access_count++;
        // }
        clock_gettime(CLOCK_MONOTONIC, &end);
    } while ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec) < duration_ns);

    return access_count;
}

void convert_to_bit_array(const char *filename, unsigned char **bit_array, long *bit_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    // Get file size in bytes
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for bit array (8 bits per byte)
    unsigned char *data = (unsigned char *)malloc(file_size * 8);
    if (!data) {
        perror("Memory allocation failed");
        fclose(file);
        exit(1);
    }

    unsigned char byte;
    long index = 0;
    
    // Read file and convert bytes to bits
    for (long i = 0; i < file_size; i++) {
        fread(&byte, 1, 1, file);
        for (int j = 7; j >= 0; j--) {
            data[index++] = (byte >> j) & 1;
        }
    }

    fclose(file);
    *bit_array = data;
    *bit_size = file_size * 8;  // Update bit size correctly
}

void convert_to_image(const char *output_filename, unsigned char *bit_array, long bit_size) {
    FILE *file = fopen(output_filename, "wb");
    if (!file) {
        perror("Error opening output file");
        exit(1);
    }

    long byte_size = bit_size / 8;
    for (long i = 0; i < byte_size; i++) {
        unsigned char byte = 0;
        for (int j = 0; j < 8; j++) {
            byte |= (bit_array[i * 8 + j] << (7 - j));
        }
        fwrite(&byte, 1, 1, file);
    }

    fclose(file);
}