## Submission Details
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

### Results
- **Accuracy :** 100%
- **Bandwidth:** 191 bps

## Task 2B
### Approach
To mount an Occupancy-based covert channel attack, an array of appropriate size is used to thrash LLC (whose size is 24MB in our machine). A pointer-chase algorithm is used.
The synchronization approach remains same as that of task 2A. However, there is a huge difference in the time interval of transmitting two successive bits resulting in comparatively less bandwidth.
A string of 1 followed by seven 0 bits and eight 1bits are sent from sender to receiver to acknowlege the start and end of the message transmission.

### Results
- **Accuracy :** 100% 
- **Bandwidth:** 1.98 bps
