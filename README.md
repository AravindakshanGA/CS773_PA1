## Submission Details
Aravindakshan - 23M1188   
Sai Teja      - 23M1172  
Advaith Kiran - 23M1156  

## Task 2A
### Approach
To mount a Flush-Reload covert channel attack, a shared memory is created using mmap and then base address in that memory is used to create a covert channel.
The .txt file is encoded into bits and written into a buffer array of appropriate size.
To synchronize the covert channel, the x86 instruction rdtscp() is used 



### Results
