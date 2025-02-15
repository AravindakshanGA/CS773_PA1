#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <x86intrin.h>  // For rdtscp
#include <unistd.h>      // For usleep
#include "./cacheutils.h"
#include "./utils.h"


// #define MAX_MSG_SIZE 1024
#define BITS_PER_CHAR 8
int bit_array[(MAX_MSG_SIZE + 1) * BITS_PER_CHAR];

size_t total_duration = 5000000;

int debug_flag = 1;     // Debug is on. Set 0 to turn OFF

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
    while(!(curr_time % total_duration > 5000 && curr_time % total_duration < 10000))  { 
    	curr_time = rdtscp();
    }
}

int main(){

    // ********** DO NOT MODIFY THIS SECTION **********
    FILE *fp = fopen(MSG_FILE, "r");
    if(fp == NULL){
        printf("Error opening file\n");
        return 1;
    }

    char msg[MAX_MSG_SIZE];
    int msg_size = 0;
    char c;
    while((c = fgetc(fp)) != EOF){
        msg[msg_size++] = c;
    }
    fclose(fp);

    clock_t start = clock();
    // **********************************************
    // ********** YOUR CODE STARTS HERE **********

    // FILE PARSING AND STORING THEM IN BIT BASED ARRAY
    // int bit_array[(MAX_MSG_SIZE + 1) * BITS_PER_CHAR];
    for (int i=0; i<8; i++) {
        bit_array[i] = 0;
    }
    store_bits(msg, msg_size, bit_array);
    for (int i=(msg_size+1) * BITS_PER_CHAR; i<(msg_size+3) * BITS_PER_CHAR; i++) {
        bit_array[i] = 0;
    }

    // Print the bit array
    // printf("Bit representation:\n");
    // for (int i = 0; i < msg_size * BITS_PER_CHAR; i++) {
    //     // for(int j=0; j<4; j++)
    //         printf("%d", bit_array[i]);
    //     if ((i + 1) % BITS_PER_CHAR == 0) {
    //         // printf(" -- ");
    //     }
    // }
    // printf("\n");

    // Creating a shared space
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

    for (int iter = 0; iter < (msg_size + 3) * BITS_PER_CHAR ; iter ++) {
        for(int j=0; j<3; j++) {
        size_t curr_time;
        int times_sent = 0;
        synchronize_processes();
        curr_time = rdtscp();
        while(curr_time % total_duration < 150000)  { 
            curr_time = rdtscp();
            // TODO: Flush data if bit is 0. else access it
            if(bit_array[iter]) {
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
    clock_t end = clock();
    double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
    printf("Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %d\n", msg_size);
    printf("Bits per second: %f\n", msg_size * 8 / time_taken);
    // **********************************************
}
