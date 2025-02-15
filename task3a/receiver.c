#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <x86intrin.h>  // For rdtscp
#include <unistd.h>      // For usleep
#include "./utils.c"

int debug_flag = 1;

// Occupancy Global Variables
char decoded_msg[MAX_MSG_SIZE]; // Store received message
int char_index = 0; // Index for storing received characters

// FR Global Variables
char image_recv_array[1000000];

// Calibration should be done for OCCUPANCY
long long threshold = 4000000;

// Calibration should be done for FLUSH+RELOAD
#define MIN_CACHE_MISS_CYCLES (215)     // Taken reference from calibration.c

int main() {

    //// OCCUPANCY CODE SNIPPET
    // Update these values accordingly
    char* received_msg = NULL;
    int received_msg_size = 0;

    srand(time(NULL));
    initialize_pointer_chase();

    int bit_buffer = 0; // Stores the 8-bit character
    long bit_count = 0;  // Counts the bits received
    int sync_zeros = 0; // Counts consecutive '0's to sync
    int synchronized = 0; // Flag to start decoding after sync

    while(1) { // Adjust iterations based on expected message length
        synchronize_processes_occupancy();
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

    received_msg = decoded_msg;
    received_msg_size = char_index-1;
    printf("Decoded Message: %s\n", decoded_msg);

    ////////////////////// OUR CODE ENDS HERE ////////////////////////

    // DO NOT MODIFY THIS LINE
    printf("Accuracy (%%): %f\n", check_accuracy_occupancy(received_msg, received_msg_size)*100);
    
    // *************** FLUSH + RELOAD ---- IMAGE
    bit_count = 0;

    int file = open(received_msg, O_RDONLY);
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

    int bit_cluster[3];  // Store 5 bits before processing
    int cluster_index = 0;
    while (1) {
        size_t curr_time;
        int access_count = 0;
        int total_count = 0;
        int miss_count = 0;

        size_t total_duration = 5000000;

        synchronize_processes_fr();
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

        // Process cluster when 3 bits are collected
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
                if (majority_bit == 1) {
                    sync_zeros++;
                    if (sync_zeros >= 64) {
                        break;
                    }
                } else {
                    sync_zeros = 0; // Reset if interrupted
                }
            }

            // Store the majority bit in the buffer
            image_recv_array[bit_count] = majority_bit;
            bit_count++;
        }
    }

    printf("%s\n", image_recv_array);
    printf("%ld\n", bit_count - 63);    // 63 bits post sync

    const char *output_filename = "output_image.jpg";

    convert_to_image(output_filename, image_recv_array, bit_count - 63);

    printf("Image reconstructed and saved as %s\n", output_filename);

    munmap(base, map_size);
    close(file);
    
    return 0;
}

// #define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(long long)) // 48MB
// #define STRIDE 1 // Can be changed for experiments
// #define BITS_PER_CHAR 8

// char decoded_msg[MAX_MSG_SIZE]; // Store received message
// int char_index = 0; // Index for storing received characters

// #define MIN_CACHE_MISS_CYCLES (215)     // Taken reference from calibration.c
// #define BITS_PER_CHAR 8
// int debug_flag = 1;
// int bit_recv_array[100000];

// char decoded_image[MAX_MSG_SIZE]; // Store received message
// int image_index = 0; // Index for storing received characters

// size_t total_duration = 5000000;


// // COMPLETE MISSES   ----- 3631149
// // COMPLETE ACCESSES ----- 4829588

// long long arr[ARRAY_SIZE];
// int next_index[ARRAY_SIZE / STRIDE]; 
// long long threshold = 4200000;

// void synchronize_processes() {
//     size_t curr_time = rdtscp();
//     size_t total_duration = 2000000000;
//     while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 15000))  { 
//     	curr_time = rdtscp();
//     }
// }

// // Shuffling
// void shuffle_indices(int *indices, int n) {
//     for (int i = n - 1; i > 0; i--) {
//         int j = rand() % (i + 1);
//         int temp = indices[i];
//         indices[i] = indices[j];
//         indices[j] = temp;
//     }
// }

// // Initialize pointer chasing
// void initialize_pointer_chase() {
//     for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
//         next_index[i] = i * STRIDE;
//     }
//     shuffle_indices(next_index, ARRAY_SIZE / STRIDE);
// }

// // Pointer chasing function
// long long pointer_chasing(long duration_ns) {
//     volatile long long sum = 0;
//     int index = 0;
//     struct timespec start, end;
//     long long access_count = 0;

//     clock_gettime(CLOCK_MONOTONIC, &start);
//     do {
//         // for (int i = 0; i < ARRAY_SIZE / STRIDE; i++) {
//             index = next_index[index];
//             sum += arr[index];
//             access_count++;
//         // }
//         clock_gettime(CLOCK_MONOTONIC, &end);
//     } while ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec) < duration_ns);

//     return access_count;
// }

// int main() {
//     srand(time(NULL));
//     initialize_pointer_chase();

//     int bit_buffer = 0; // Stores the 8-bit character
//     int bit_count = 0;  // Counts the bits received
//     int sync_zeros = 0; // Counts consecutive '0's to sync
//     int synchronized = 0; // Flag to start decoding after sync

//     while(1) { // Adjust iterations based on expected message length
//         synchronize_processes();
//         long long access_count = pointer_chasing(200000000L); // Run for 200ms

//         int bit = (access_count < threshold) ? 0 : 1;
//         printf("%d", bit);
//         fflush(stdout); // Ensure real-time output

//         if (!synchronized) {
//             // Check for 8 consecutive zeros to start syncing
//             if (bit == 0) {
//                 sync_zeros++;
//                 if (sync_zeros >= 8) {
//                     synchronized = 1; // Start decoding
//                     // printf("SYNC ACHIEVED");
//                     sync_zeros = 0;
//                 }
//             } else {
//                 sync_zeros = 0; // Reset if interrupted
//             }
//             continue; // Ignore bits until sync is achieved
//         }

//         if (synchronized) {
//             // Check for 16 consecutive ones to start syncing
//             if (bit == 1) {
//                 sync_zeros++;
//                 if (sync_zeros >= 16) {
//                     break;
//                 }
//             } else {
//                 sync_zeros = 0; // Reset if interrupted
//             }
//         }

//         // Store the bit in the buffer
//         bit_buffer = (bit_buffer << 1) | bit;
//         bit_count++;

//         // Convert 8-bit buffer to ASCII when full
//         // Convert 8-bit buffer to ASCII when full
//         if (bit_count == BITS_PER_CHAR) {
//             if (char_index < MAX_MSG_SIZE - 1) { // Avoid overflow
//                 decoded_msg[char_index++] = (char)bit_buffer;
//             }
//             bit_buffer = 0;
//             bit_count = 0;
//         }

//         usleep(100000); // Sleep for 100ms
//     }

//     // Null-terminate and print final message
//     decoded_msg[char_index-1] = '\0';
//     printf("Decoded Message: %s\n", decoded_msg);

//     // Update these values accordingly
//     char* received_msg = NULL;
//     int received_msg_size = 0;

//     ////////////////////// OUR CODE HERE /////////////////////////////

//     bit_buffer = 0; // Stores the 8-bit character
//     bit_count = 0;  // Counts the bits received
//     sync_zeros = 0; // Counts consecutive '0's to sync
//     synchronized = 0; // Flag to start decoding after sync

//     // Creating a shared file and making it as a covert channel to get information
//     int file = open(decoded_msg, O_RDONLY);
//     // get the file size
    
//     size_t size = lseek(file, 0, SEEK_END);
//     if (size == 0)
//         exit(-1);

//     if(debug_flag)
//         printf("Size of the shared file : %lx\n", size);

//     size_t map_size = size;
//     // making the map_size page-aligned i.e. a multiple of 4 KB page
//     // in order to grant the address space using mmap --- anything above 1 bit than 4KB will go to a new page and so make it as 2 pages.
//     if (map_size & 0xFFF != 0) {
//         map_size |= 0xFFF;
//         map_size += 1;
//     }

//     if(debug_flag)
//         printf("Map Size : %lx\n", map_size);

//     char *base = (char *)mmap(0, map_size, PROT_READ, MAP_SHARED, file, 0);

//     printf("Base Address : %c \n", *base);

//     int bit_cluster[3];  // Store 5 bits before processing
//     int cluster_index = 0;
//     while (1) {
//         size_t curr_time;
//         int access_count = 0;
//         int total_count = 0;
//         int miss_count = 0;

//         synchronize_processes();
//         curr_time = rdtscp();

//         while (curr_time % total_duration < 150000) { 
//             curr_time = rdtscp();
//             size_t start = rdtscp();
//             maccess(base);
//             size_t delta = rdtscp() - start;

//             if (delta < MIN_CACHE_MISS_CYCLES)
//                 access_count++;
//             else
//                 miss_count++;
//             total_count++;
//         }

//         int bit = (access_count < miss_count) ? 0 : 1;
//         bit_cluster[cluster_index++] = bit;  // Store bit in cluster
//         // printf("%d", cluster_index);
//         // fflush(stdout);

//         // Process cluster when 5 bits are collected
//         if (cluster_index == 3) {
//             int one_count = 0, zero_count = 0;
//             for (int i = 0; i < 3; i++) {
//                 if (bit_cluster[i] == 1)
//                     one_count++;
//                 else
//                     zero_count++;
//             }

//             int majority_bit = (one_count > zero_count) ? 1 : 0;
//             // printf("%d", majority_bit);
//             // fflush(stdout);

//             cluster_index = 0;  // Reset cluster for next batch

//             if (!synchronized) {
//                 // Check for 8 clusters of zeros to start syncing
//                 if (majority_bit == 0) {
//                     sync_zeros++;
//                     if (sync_zeros >= 8) {
//                         synchronized = 1; // Start decoding
//                         sync_zeros = 0;
//                     }
//                 } else {
//                     sync_zeros = 0; // Reset if interrupted
//                 }
//                 continue; // Ignore bits until sync is achieved
//             }

//             if (synchronized) {
//                 // Check for 16 clusters of zeros to stop
//                 if (majority_bit == 0) {
//                     sync_zeros++;
//                     if (sync_zeros >= 17) {
//                         break;
//                     }
//                 } else {
//                     sync_zeros = 0; // Reset if interrupted
//                 }
//             }

//             // Store the majority bit in the buffer
//             bit_recv_array[bit_count] = majority_bit;
//             bit_buffer = (bit_buffer << 1) | majority_bit;
//             bit_count++;

//             // Convert to ASCII when buffer is full
//             if (bit_count == BITS_PER_CHAR) {
//                 if (char_index < MAX_MSG_SIZE - 1) { // Avoid overflow
//                     decoded_msg[char_index++] = (char)bit_buffer;
//                 }
//                 received_msg_size++;
//                 bit_buffer = 0;
//                 bit_count = 0;
//             }
//         }
//     }


//     // Null-terminate and print final message
//     decoded_image[image_index] = '\0';
//     received_msg = decoded_image;
//     printf("Decoded Message: %s\n", decoded_msg);

//     ////////////////////// OUR CODE ENDS HERE ////////////////////////

//     // DO NOT MODIFY THIS LINE
//     printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size)*100);
// }