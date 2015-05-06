#PCC: Re-architecting Congestion Control for High Performance

###Read before use:
The utility function in use in the current code base is a latency senstive one we are actively experimenting. To switch back to the default utility function, you just need one line of code change in cc.h. Uncomment cc.h:302 and comment out cc.h:303. And do a make clean;make (Proof of flexibility :))

## How to use PCC

### Installation Guide
Setup:
For sender:
1. go to ./sender

2. make clean

3. make

4. export LD_LIBRARY_PATH=/path/to/install/sender/src

For receiver:

1. go to ./receiver

2. make clean

3. make

4. export LD_LIBRARY_PATH=/path/to/install/receiver/src


### Replicate Demo with Emulab
Comming soon

### Evaluate PCC in Your Network
1. Memory to Memory Data transfer speed test(without using disk): 

  1.1 On receiver, go to recever/app run ./appserver 

  1.2 On sender goto sender/app run ./appclient receiver's address 9000 2>monitor_log

2. File transfer software:

  1.1 On receiver, run ./recvfile
 
  1.2 on Sender, run ./sendfile dst_address 9000 file_to_send 2>monitor_log

## Known Issues
1. We recommend do test on FEDORA 15.0 STD system and a server with at least two cores, on which the performance is best. We have tested it on Unbuntu 12.04LTS and 14.04LTS, it normally works fine but there is an issue of UDP’s sendto() system call (sometimes sendto just slows down awfully) that may break the packet pacing PCC relies on. 

2. Also, if you are running using a low-end CPU without constant time counter in hardware or running in a virtualized environment, especially KVM based VMs,  please run it under static core binding using likwid-pin provided by google, otherwise, the timer will be messed up and the pacing will break.

3. You may notice that pacing is the main performance issue we have. You can turn on non_busy_waiting pacing in udt.h, it gives you inaccurate pacing but avoid PCC eating one core per flow. A better solution we are looking at is [SENIC](https://github.com/sivasankariit/qfq-rl). I would like to use SENIC to replace the pacing module of PCC.

