// LLC Size -- 24MiB
// Using cores of my machine - 4 and 7 (Have private l1 and l2 and shared l3)

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include "./utils.c"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <x86intrin.h>  // For rdtscp

int debug_flag = 1;

// Occupancy based global declarations
int bit_array[(MAX_MSG_SIZE) * BITS_PER_CHAR];      // used only for occupancy

void store_bits(char *msg, int msg_size, int *bit_array) {
    int index = 8;
    for (int i = 0; i < msg_size; i++) {
        // printf("%c %d\n", msg[i], int(msg[i]));
        for (int j = BITS_PER_CHAR - 1; j >= 0; j--) {
            bit_array[index++] = (msg[i] >> j) & 1;
        }
    }
}

int main() {
    int msg_size = 16;

    srand(time(NULL));
    initialize_pointer_chase();

    bit_array[0] = 1;
    for (int i=1; i<8; i++) {
        bit_array[i] = 0;
    }
    store_bits("shared_file.txt", msg_size, bit_array);
    for (int i=(msg_size+1) * BITS_PER_CHAR; i<(msg_size+3) * BITS_PER_CHAR; i++) {
        bit_array[i] = 1;
    }

    clock_t start = clock();

    for (int iter = 0; iter < (msg_size + 3) * BITS_PER_CHAR; iter++) {
        if (bit_array[iter] == 0) {
            synchronize_processes_occupancy();
            pointer_chasing(200000000L); // Run pointer chasing for 200ms
        }
        else {
            synchronize_processes_occupancy();
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
    printf("Occupancy Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %d\n", msg_size);
    printf("Bits per second: %f\n", msg_size * 8 / time_taken);
    // **********************************************

    // ************ FLUSH + RELOAD --- IMAGE
    const char *input_filename = "red_heart.jpg";
    unsigned char *image_bit_array;
    long image_bit_size;

    convert_to_bit_array(input_filename, &image_bit_array, &image_bit_size);

    printf("TRANSMITTING BITS : %ld \n", image_bit_size);

    clock_t start_fr = clock();

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

    int sync_start_array[8];
    int sync_end_array[16];
    size_t total_duration = 5000000;

    if(debug_flag)
        printf("Map Size : %lx\n", map_size);

    char *base = (char *)mmap(0, map_size, PROT_READ, MAP_SHARED, file, 0);

    for (int i=0; i<8; i++) {
        sync_start_array[i] = 0;
    }
    for (int i=0; i<16; i++) {
        sync_end_array[i] = 1;
    }

    printf("Base Address : %c \n", *base);

    for (int iter = 0; iter < 8 ; iter ++) {
        for(int j=0; j<3; j++) {
        size_t curr_time;
        int times_sent = 0;
        synchronize_processes_fr();
        curr_time = rdtscp();
        while(curr_time % total_duration < 150000)  { 
            curr_time = rdtscp();
            // TODO: Flush data if bit is 0. else access it
            if(sync_start_array[iter]) {
                maccess(base);
            }
            else {
                flush(base);
            }
        }
        }
        // printf("%d",bit_array[iter]);
        // fflush(stdout);
    }

    for (int iter = 0; iter < image_bit_size ; iter ++) {
        for(int j=0; j<3; j++) {
        size_t curr_time;
        int times_sent = 0;
        synchronize_processes_fr();
        curr_time = rdtscp();
        while(curr_time % total_duration < 150000)  { 
            curr_time = rdtscp();
            // TODO: Flush data if bit is 0. else access it
            if(image_bit_array[iter]) {
                maccess(base);
            }
            else {
                flush(base);
            }
        }
        }
        // printf("%d",bit_array[iter]);
        // fflush(stdout);
    }

    for (int iter = 0; iter < 64 ; iter ++) {
        for(int j=0; j<3; j++) {
        size_t curr_time;
        int times_sent = 0;
        synchronize_processes_fr();
        curr_time = rdtscp();
        while(curr_time % total_duration < 150000)  { 
            curr_time = rdtscp();
            // TODO: Flush data if bit is 0. else access it
            if(sync_end_array[iter % 16]) {
                maccess(base);
            }
            else {
                flush(base);
            }
        }
        }
        // printf("%d",bit_array[iter]);
        // fflush(stdout);
    }

    printf("\n");



    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS SECTION **********
    clock_t end_fr = clock();
    time_taken = ((double)end_fr - start_fr) / CLOCKS_PER_SEC;
    printf("Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %ld\n", image_bit_size);
    printf("Bits per second: %f\n", image_bit_size / time_taken);
    // **********************************************

    return 0;
}

// #define ARRAY_SIZE (48 * 1024 * 1024 / sizeof(long long)) // 48MB   
// #define STRIDE 1
// #define BITS_PER_CHAR 8

// size_t total_duration = 5000000;

// int debug_flag = 1;

// int bit_array[(MAX_MSG_SIZE) * BITS_PER_CHAR];

// int sync_start_array[8];
// int sync_end_array[16];

// long long arr[ARRAY_SIZE];
// int next_index[ARRAY_SIZE / STRIDE]; 

// void store_bits(char *msg, int msg_size, int *bit_array) {
//     int index = 8;
//     for (int i = 0; i < msg_size; i++) {
//         // printf("%c %d\n", msg[i], int(msg[i]));
//         for (int j = BITS_PER_CHAR - 1; j >= 0; j--) {
//             bit_array[index++] = (msg[i] >> j) & 1;
//         }
//     }
// }

// void convert_to_bit_array(const char *filename, unsigned char **bit_array, long *size) {
//     FILE *file = fopen(filename, "rb");
//     if (!file) {
//         perror("Error opening file");
//         exit(1);
//     }

//     // Get file size
//     fseek(file, 0, SEEK_END);
//     *size = ftell(file);
//     rewind(file);

//     // Allocate memory for bit array (8 bits per byte)
//     unsigned char *data = (unsigned char *)malloc((*size) * 8);
//     if (!data) {
//         perror("Memory allocation failed");
//         fclose(file);
//         exit(1);
//     }

//     unsigned char byte;
//     long index = 8;
    
//     // Read file and convert bytes to bits
//     for (long i = 0; i < *size; i++) {
//         fread(&byte, 1, 1, file);
//         for (int j = 7; j >= 0; j--) {
//             data[index++] = (byte >> j) & 1;
//         }
//     }

//     fclose(file);
//     *bit_array = data;
// }

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

// int main() {    
//     srand(time(NULL));
//     initialize_pointer_chase();

//     int msg_size = 15;

//     for (int i=0; i<8; i++) {
//         bit_array[i] = 0;
//     }
//     store_bits("shared_file.txt", msg_size, bit_array);
//     for (int i=(msg_size+1) * BITS_PER_CHAR; i<(msg_size+3) * BITS_PER_CHAR; i++) {
//         bit_array[i] = 1;
//     }

//     // Sending it through occupancy channel
//     for (int iter = 0; iter < (msg_size + 3) * BITS_PER_CHAR; iter++) {
//         if (bit_array[iter] == 0) {
//             synchronize_processes();
//             pointer_chasing(200000000L); // Run pointer chasing for 200ms
//         }
//         else {
//             synchronize_processes();
//             usleep(200000);
//         }
//         // printf("%d", bit_array[iter]);
//         printf("%d", bit_array[iter]);
//         fflush(stdout); // Ensure real-time output
//         usleep(100000); // Sleep for 100ms
//     }

//     // ********** DO NOT MODIFY THIS SECTION **********
//     const char *input_filename = "red_heart.jpg";
//     unsigned char *bit_array;
//     long bit_size;

//     convert_to_bit_array(input_filename, &bit_array, &bit_size);

//     // Print first 64 bits as a test
//     printf("First 64 bits of the image:\n");
//     for (int i = 0; i < 64; i++) {
//         printf("%d", bit_array[i]);
//         if ((i + 1) % 8 == 0) printf(" ");
//     }
//     printf("\n");

//     for (int i=0; i<8; i++) {
//         sync_start_array[i] = 0;
//     }
//     for (int i=0; i<16; i++) {
//         sync_end_array[i] = 1;
//     }

//     clock_t start = clock();
//     // **********************************************
//     // ********** YOUR CODE STARTS HERE **********

//     // FILE PARSING AND STORING THEM IN BIT BASED ARRAY
//     // int bit_array[(MAX_MSG_SIZE + 1) * BITS_PER_CHAR];

//     // Print the bit array
//     // printf("Bit representation:\n");
//     // for (int i = 0; i < msg_size * BITS_PER_CHAR; i++) {
//     //     // for(int j=0; j<4; j++)
//     //         printf("%d", bit_array[i]);
//     //     if ((i + 1) % BITS_PER_CHAR == 0) {
//     //         // printf(" -- ");
//     //     }
//     // }
//     // printf("\n");

//     // Creating a shared space
//     int file = open("shared_file.txt", O_RDONLY);
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

//     for (int iter = 0; iter < 8 ; iter ++) {
//         for(int j=0; j<3; j++) {
//         size_t curr_time;
//         int times_sent = 0;
//         synchronize_processes();
//         curr_time = rdtscp();
//         while(curr_time % total_duration < 150000)  { 
//             curr_time = rdtscp();
//             // TODO: Flush data if bit is 0. else access it
//             if(sync_start_array[iter]) {
//                 maccess(base);
//             }
//             else {
//                 flush(base);
//             }
//         }
//         }
//         // printf("%d",bit_array[iter]);
//         // fflush(stdout);
//     }

//     for (int iter = 0; iter < bit_size ; iter++) {
//         for(int j=0; j<3; j++) {
//         size_t curr_time;
//         int times_sent = 0;
//         synchronize_processes();
//         curr_time = rdtscp();
//         while(curr_time % total_duration < 150000)  { 
//             curr_time = rdtscp();
//             // TODO: Flush data if bit is 0. else access it
//             if(bit_array[iter]) {
//                 maccess(base);
//             }
//             else {
//                 flush(base);
//             }
//         }
//         }
//         // printf("%d",bit_array[iter]);
//         // fflush(stdout);
//     }

//     for (int iter = 0; iter < 16 ; iter ++) {
//         for(int j=0; j<3; j++) {
//         size_t curr_time;
//         int times_sent = 0;
//         synchronize_processes();
//         curr_time = rdtscp();
//         while(curr_time % total_duration < 150000)  { 
//             curr_time = rdtscp();
//             // TODO: Flush data if bit is 0. else access it
//             if(sync_end_array[iter]) {
//                 maccess(base);
//             }
//             else {
//                 flush(base);
//             }
//         }
//         }
//         // printf("%d",bit_array[iter]);
//         // fflush(stdout);
//     }

//     printf("\n");



//     // ********** YOUR CODE ENDS HERE **********
//     // ********** DO NOT MODIFY THIS SECTION **********
//     clock_t end = clock();
//     double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
//     printf("Message sent successfully\n");
//     printf("Time taken to send the message: %f\n", time_taken);
//     printf("Message size: %d\n", msg_size);
//     printf("Bits per second: %f\n", msg_size * 8 / time_taken);
//     // **********************************************

//     return 0;
// }