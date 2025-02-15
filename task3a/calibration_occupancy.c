#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include "./cacheutils.h"

#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(long long)) // 24MB
#define STRIDE 1 // Can be changed for experiments

/*
    FILL IN BEFORE STARTING
        CORE ID :
        HIT TIME : 4379323
        MISS TIME : 3382354
*/

long long arr[ARRAY_SIZE];
int next_index[ARRAY_SIZE / STRIDE]; 
long long threshold = 0;

// Shuffling
void shuffle_indices(int *indices, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

void synchronize_processes() {
    size_t curr_time = rdtscp();
    size_t total_duration = 1000000000;
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 15000))  { 
    	curr_time = rdtscp();
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
    srand(time(NULL));
    initialize_pointer_chase();

    for (int j = 0; j < 2; j++) {
        for (int test = 0; test < 100; test++) {
            long long access_count = pointer_chasing(200000000L); // 20ms

            printf("Total memory accesses in 200ms: %lld\n", access_count);
            threshold += access_count;
            // sched_yield();
            usleep(100000); // 100 ms sleep
        }
        threshold /= 100;
        printf("Threshold Set: %lld\n", threshold);
        threshold = 0;
    }

    return 0;
}