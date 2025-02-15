// LLC Size -- 24MiB
// Using cores of my machine - 4 and 7 (Have private l1 and l2 and shared l3)

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include "./utils.h"
#include "./cacheutils.h"

#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(long long)) // 24MB   
#define STRIDE 1
#define BITS_PER_CHAR 8

int bit_array[(MAX_MSG_SIZE) * BITS_PER_CHAR];

void store_bits(char *msg, int msg_size, int *bit_array) {
    int index = 8;
    for (int i = 0; i < msg_size; i++) {
        // printf("%c %d\n", msg[i], int(msg[i]));
        for (int j = BITS_PER_CHAR - 1; j >= 0; j--) {
            bit_array[index++] = (msg[i] >> j) & 1;
        }
    }
}

void synchronize_processes() {
    size_t curr_time = rdtscp();
    size_t total_duration = 2000000000;
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 15000))  { 
    	curr_time = rdtscp();
    }
}

long long arr[ARRAY_SIZE];
int next_index[ARRAY_SIZE / STRIDE]; 

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
void pointer_chasing(long duration_ns) {
    volatile long long sum = 0;
    int index = 0;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        // for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
            index = next_index[index];
            sum += arr[index];
        // }
        clock_gettime(CLOCK_MONOTONIC, &end);
    } while ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec) < duration_ns);
}

int main() {
    // ********** DO NOT MODIFY THIS SECTION **********
    FILE *fp = fopen(MSG_FILE, "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    char msg[MAX_MSG_SIZE];
    int msg_size = 0;
    char c;
    while ((c = fgetc(fp)) != EOF) {
        msg[msg_size++] = c;
    }
    fclose(fp);

    clock_t start = clock();
    // **********************************************
    
    srand(time(NULL));
    initialize_pointer_chase();

    bit_array[0] = 1;
    for (int i=1; i<8; i++) {
        bit_array[i] = 0;
    }
    store_bits(msg, msg_size, bit_array);
    for (int i=(msg_size+1) * BITS_PER_CHAR; i<(msg_size+3) * BITS_PER_CHAR; i++) {
        bit_array[i] = 1;
    }

    // Print the bit array
    // printf("Bit representation:\n");
    // for (int i = 0; i < (msg_size + 1) * BITS_PER_CHAR; i++) {
    //     printf("%d", bit_array[i]);
    //     if ((i + 1) % BITS_PER_CHAR == 0) {
    //         // printf(" ");
    //     }
    // }
    // printf("\n");

    for (int iter = 0; iter < (msg_size + 3) * BITS_PER_CHAR; iter++) {
        if (bit_array[iter] == 0) {
            synchronize_processes();
            pointer_chasing(200000000L); // Run pointer chasing for 200ms
        }
        else {
            synchronize_processes();
            usleep(200000);
        }
        // printf("%d", bit_array[iter]);
        printf("%d", bit_array[iter]);
        fflush(stdout); // Ensure real-time output
        usleep(100000); // Sleep for 100ms
    }

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS SECTION **********
    clock_t end = clock();
    double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
    printf("Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %d\n", msg_size);
    printf("Bits per second: %f\n", msg_size * 8 / time_taken);
    // **********************************************


    return 0;
}