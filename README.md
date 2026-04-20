Raxzus_heap_research

This is a novel heap idea on a x86-32bit custom kernel. The idea is that we shift heap allocation to hardware MMU instead of keeping it in software, By doing this we have as of now a prototype that is resistant to fragmentaion, it also has a look up time of O(1) and the wors and best case scenarios differs with max 16 cycles. Below is images showing the heap running a stress test and as seen it survives and also the cycles on the 32 bit system is 632 +- 10. As we also can see in the images, the lookup is indeed O(1). 

<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/ff08392d-4fad-4fe8-8d07-c36fd4936ceb" />

<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/ea6f9f03-7ad9-46fc-92ac-b971ed911b30" />
