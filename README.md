PCC
========

Description:

More than two decades of research have shown that TCP's performance suffers in many environments. Efforts to improve its performance by providing rate feedback from network devices are hard to deploy. Numerous end host modifications of TCP address specific problems but fail to broadly improve performance across a range of challenging network conditions. Jumping out of the TCP family's architecture's deficiency, we propose Performance-oriented Congestion Control (PCC), a new congestion control architecture in which each sender selfishly controls its sending strategy based on empirically observed performance metrics. We show through large-scale experiments that PCC can significantly benefit multiple real-world applications with robustly improved performance across multiple challenging scenarios. Despite individual senders' selfish behavior, we prove that PCC \pbgnew{converges to a} fair equilibrium; moreover, experiments show even better fairness and stability than TCP. With PCC's significantly improved robustness and safety, we discuss different deployment scenarios of PCC and point out that PCC joins \pbgnew{an evolving} trend towards a better congestion control architecture.

HOWTO:
===

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


NOTE: We recommend do test on FEDORA 10.0 STD system, on which the performance is best. We have tested it on Unbuntu 12.04LTS and 13.04LTS, it normally works fine but there could be some unknown issues and we are actively figuring it out. Also, if you are running using a low-end CPU without constant time counter in hardware or running in a virtualized environment, especially KVM based VMs,  please run it under static core binding using likwid-pin provided by google.

TestPerformance:
===
1. Memory to Memory Data transfer speed test(without using disk): 
  1.1 On receiver, go to recever/app run ./appserver 
  1.3 On sender goto sender/app run ./appclient receiver's address 9000 2>monitor_log

2. File transfer software:
  1.1 On receiver, run ./recvfile
  1.2 on Sender, run ./sendfile dst_address 9000 file_to_send 2>monitor_log
