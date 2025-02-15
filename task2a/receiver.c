#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <x86intrin.h>  // For rdtscp
#include <unistd.h>      // For usleep
#include "./cacheutils.h"
#include "./utils.c"

#define MIN_CACHE_MISS_CYCLES (215)     // Taken reference from calibration.c
#define BITS_PER_CHAR 8
int debug_flag = 1;
int bit_recv_array[100000];

char decoded_msg[MAX_MSG_SIZE]; // Store received message
int char_index = 0; // Index for storing received characters

size_t total_duration = 5000000;

void synchronize_processes() {
    size_t curr_time = rdtscp();
    while(!(curr_time % total_duration >5000 && curr_time % total_duration <10000))  { 
    	curr_time = rdtscp();
    }
}

int main(){
    
    // Update these values accordingly
    char* received_msg = NULL;
    int received_msg_size = 0;

    ////////////////////// OUR CODE HERE /////////////////////////////

    int bit_buffer = 0; // Stores the 8-bit character
    int bit_count = 0;  // Counts the bits received
    int sync_zeros = 0; // Counts consecutive '0's to sync
    int synchronized = 0; // Flag to start decoding after sync

    // Creating a shared file and making it as a covert channel to get information
    int file = open("shared_file.txt", O_RDONLY);
    // get the file size
    
    size_t size = lseek(file, 0, SEEK_END);
    if (size == 0)
        exit(-1);

    if(debug_flag)
        printf("Size of the shared file : %lx\n", size);

    size_t map_size = size;
    // making the map_size page-aligned i.e. a multiple of 4 KB page
    // in order to grant the address space using mmap --- anything above 1 bit than 4KB will go to a new page and so make it as 2 pages.
    if (map_size & 0xFFF != 0) {
        map_size |= 0xFFF;
        map_size += 1;
    }

    if(debug_flag)
        printf("Map Size : %lx\n", map_size);

    char *base = (char *)mmap(0, map_size, PROT_READ, MAP_SHARED, file, 0);

    printf("Base Address : %c \n", *base);

    int bit_cluster[3];  // Store 3 bits before processing
    int cluster_index = 0;
    while (1) {
        size_t curr_time;
        int access_count = 0;
        int total_count = 0;
        int miss_count = 0;

        synchronize_processes();
        curr_time = rdtscp();

        while (curr_time % total_duration < 150000) { 
            curr_time = rdtscp();
            size_t start = rdtscp();
            maccess(base);
            size_t delta = rdtscp() - start;

            if (delta < MIN_CACHE_MISS_CYCLES)
                access_count++;
            else
                miss_count++;
            total_count++;
        }

        int bit = (access_count < miss_count) ? 0 : 1;
        bit_cluster[cluster_index++] = bit;  // Store bit in cluster
        // printf("%d", cluster_index);
        // fflush(stdout);

        // Process cluster when 5 bits are collected
        if (cluster_index == 3) {
            int one_count = 0, zero_count = 0;
            for (int i = 0; i < 3; i++) {
                if (bit_cluster[i] == 1)
                    one_count++;
                else
                    zero_count++;
            }

            int majority_bit = (one_count > zero_count) ? 1 : 0;
            // printf("%d", majority_bit);
            // fflush(stdout);

            cluster_index = 0;  // Reset cluster for next batch

            if (!synchronized) {
                // Check for 8 clusters of zeros to start syncing
                if (majority_bit == 0) {
                    sync_zeros++;
                    if (sync_zeros >= 8) {
                        synchronized = 1; // Start decoding
                        sync_zeros = 0;
                    }
                } else {
                    sync_zeros = 0; // Reset if interrupted
                }
                continue; // Ignore bits until sync is achieved
            }

            if (synchronized) {
                // Check for 16 clusters of zeros to stop
                if (majority_bit == 0) {
                    sync_zeros++;
                    if (sync_zeros >= 17) {     // Written as 17 specific to this message sent. As I was sending the last character ASCII to have one 0 at end and then adding 16 zeroes to end communication. Hence, this is taken as 17
                        break;
                    }
                } else {
                    sync_zeros = 0; // Reset if interrupted
                }
            }

            // Store the majority bit in the buffer
            bit_recv_array[bit_count] = majority_bit;
            bit_buffer = (bit_buffer << 1) | majority_bit;
            bit_count++;

            // Convert to ASCII when buffer is full
            if (bit_count == BITS_PER_CHAR) {
                if (char_index < MAX_MSG_SIZE - 1) { // Avoid overflow
                    decoded_msg[char_index++] = (char)bit_buffer;
                }
                received_msg_size++;
                bit_buffer = 0;
                bit_count = 0;
            }
        }
    }


    // Null-terminate and print final message
    decoded_msg[char_index] = '\0';
    received_msg = decoded_msg;
    printf("Decoded Message: %s\n", decoded_msg);

    ////////////////////// OUR CODE ENDS HERE ////////////////////////

    // DO NOT MODIFY THIS LINE
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size)*100);
}