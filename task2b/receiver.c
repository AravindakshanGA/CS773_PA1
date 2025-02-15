#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include "./utils.c"
#include "./cacheutils.h"

#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(long long)) // 48MB
#define STRIDE 1 // Can be changed for experiments
#define BITS_PER_CHAR 8

char decoded_msg[MAX_MSG_SIZE]; // Store received message
int char_index = 0; // Index for storing received characters

// COMPLETE MISSES   ----- 387785
// COMPLETE ACCESSES ----- 436658

long long arr[ARRAY_SIZE];
int next_index[ARRAY_SIZE / STRIDE]; 
long long threshold = 4000000;

void synchronize_processes() {
    size_t curr_time = rdtscp();
    size_t total_duration = 2000000000;
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 15000))  { 
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

int main() {
    // Update these values accordingly
    char* received_msg = NULL;
    int received_msg_size = 0;

    srand(time(NULL));
    initialize_pointer_chase();

    int bit_buffer = 0; // Stores the 8-bit character
    int bit_count = 0;  // Counts the bits received
    int sync_zeros = 0; // Counts consecutive '0's to sync
    int synchronized = 0; // Flag to start decoding after sync

    while(1) { // Adjust iterations based on expected message length
        synchronize_processes();
        long long access_count = pointer_chasing(200000000L); // Run for 200ms

        int bit = (access_count < threshold) ? 0 : 1;
        printf("%d", bit);
        fflush(stdout); // Ensure real-time output

        if (!synchronized) {
            // Check for 8 consecutive zeros to start syncing
            if (bit == 0) {
                sync_zeros++;
                if (sync_zeros >= 7) {
                    synchronized = 1; // Start decoding
                    // printf("SYNC ACHIEVED");
                    sync_zeros = 0;
                }
            } else {
                sync_zeros = 0; // Reset if interrupted
            }
            continue; // Ignore bits until sync is achieved
        }

        if (synchronized) {
            // Check for 8 consecutive zeros to start syncing
            if (bit == 1) {
                sync_zeros++;
                if (sync_zeros >= 16) {
                    break;
                }
            } else {
                sync_zeros = 0; // Reset if interrupted
            }
        }

        // Store the bit in the buffer
        bit_buffer = (bit_buffer << 1) | bit;
        bit_count++;

        // Convert 8-bit buffer to ASCII when full
        // Convert 8-bit buffer to ASCII when full
        if (bit_count == BITS_PER_CHAR) {
            if (char_index < MAX_MSG_SIZE - 1) { // Avoid overflow
                decoded_msg[char_index++] = (char)bit_buffer;
            }
            bit_buffer = 0;
            bit_count = 0;
        }

        usleep(100000); // Sleep for 100ms
    }

    // Null-terminate and print final message
    decoded_msg[char_index-1] = '\0';
    printf("Decoded Message: %s\n", decoded_msg);

    received_msg = decoded_msg;
    received_msg_size = char_index-1;
    printf("Decoded Message: %s\n", decoded_msg);

    ////////////////////// OUR CODE ENDS HERE ////////////////////////

    // DO NOT MODIFY THIS LINE
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size)*100);
    return 0;
}