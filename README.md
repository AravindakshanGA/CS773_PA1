## Submission Details
**TEAM NAME :** Architecture Security Force
Aravindakshan - 23M1188   
Sai Teja      - 23M1172  
Advaith Kiran - 23M1156  

## Task 2A
### Approach
To mount a Flush-Reload covert channel attack, a shared memory is created using mmap and then base address in that memory is used to create a covert channel.
The .txt file is encoded into bits and written into a buffer array of appropriate size.
To synchronize the covert channel, the x86 instruction **rdtscp()** is used.
The function **synchronize_processes()** with appropriate no. of clock ticks is used and encoded bits are transmitted from sender to reciever using Flush-Reload method.
A string of eight 0 bits is sent from sender to reciever to acknowlege the start and end of the message transmission. 

### Calibration Steps
1. Compilation of sources are made with no compiler optimizations by adding -O0 flag. 
2. Run the calibration.c and get the threshold used to mount the attack. (Credits to the Task 1 source)
3. Update this value into the receiver.c and then use taskset command with core indices to run the sender and receiver. Receiver first and then sender.
        taskset -c <core_no> <compiled_object_name>

### Results
- **Accuracy :** 100%
- **Bandwidth:** 191 bps

## Task 2B
### Approach
To mount an Occupancy-based covert channel attack, an array of appropriate size is used to thrash LLC (whose size is 24MB in our machine). A pointer-chase algorithm is used.
The synchronization approach remains same as that of task 2A. However, there is a huge difference in the time interval of transmitting two successive bits resulting in comparatively less bandwidth.
A string of 1 followed by seven 0 bits and sixteen 1 bits are sent from sender to receiver to acknowlege the start and end of the message transmission.

### Calibration Steps
1. Compilation of sources are made with no compiler optimizations by adding -O0 flag. 
2. Run the calibration.c first time in the core where your receiver is planned to run and get the threshold (This will be the access threshold, where no other process is involved). Then run the same calibration code in both the sender and receiver code planned and get the threshold. (This will be the miss threshold, where another process is involved in affecting the LLC)
3. Calculate the threshold by averaging the above two values and update this value into the receiver.c (to the threshold variable) and then use taskset command with core indices to run the sender and receiver. Receiver first and then sender.
        taskset -c <core_no> <compiled_object_name>

### Results
- **Accuracy :** 100% 
- **Bandwidth:** 1.98 bps

## Task 3A
### Approach
To share a heart image, a slightly different approach is used.
Firstly, Occupancy- based covert channel is used to send the file shared between the two processes.
Then, the shared file is used by the receiver to acknowledge the process and start receiving bits via Flush-Reload covert channel.
Some modifications were done in the synchronization part to ensure proper data transmission between sender and receiver.

### Calibration
Follow the calibration process of both Task 2A and 2B and then start the process.

### Results
#### Image Transmission
- **Sent Image :** CS773_PA1/task3a/red_heart.jpg
- **Received Image :** CS773_PA1/task3a/results/output_image_2.jpg
- **Time Taken :** 22 mins
- **Accuracy :** 60-70% 

## Task 3B
### Approach
To share an audio file, similar approach of task 3A is followed except that the input is a .mp3 file. 

### Results
#### Audio Transmission
- **Sent Audio :** CS773_PA1/task3b/audio.mp3
- **Received Audio :** CS773_PA1/task3b/results/output_audio.mp3
- **Time Taken :** 100 mins
- **Accuracy :** 50-60%

## Plagiarism Checklist
1. Have you strictly adhered to the submission guidelines?  
- Yes
2. Have you received or provided any assistance for this assignment beyond public discussions with peers/TAs?  
- No

## Additional Information
All the results are available under results folder of the respective tasks.
