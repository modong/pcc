import matplotlib.pyplot as plt
import copy
import matplotlib.animation as animation
import time
import os

f, axarr = plt.subplots(2, 1, sharey=True, sharex=True)

base_time_line = -1
which_flow = 0

time_offset = {}
base_line = {}
base_line["rate_tcp1"] = -1
base_line["rate_tcp2"] = -1
base_line["rate_pcc1"] = -1
base_line["rate_pcc1"] = -1
dataArray = {}

def greater(a,b):
    if a == -1:
        return True
    if b == -1:
        return False
    return a>b

def animate_together(i):
    os.system("scp modong2@sender1.demopair3.uiucscheduling.emulab.net:/local/rate_pcc /home/mo/pcc/convergence/rate_pcc1")
    os.system("scp modong2@sender2.demopair3.uiucscheduling.emulab.net:/local/rate_pcc /home/mo/pcc/convergence/rate_pcc2")
    os.system("scp modong2@sender1.demopair4.uiucscheduling.emulab.net:/local/rate_tcp /home/mo/pcc/convergence/rate_tcp1")
    os.system("scp modong2@sender2.demopair4.uiucscheduling.emulab.net:/local/rate_tcp /home/mo/pcc/convergence/rate_tcp2")

    pullData_tcp1 = open("rate_tcp1", "r")
    line = pullData_tcp1.readline()
    if line != "":
        base_line["tcp1"] = int(line)

    for k in range(6):
        pullData_tcp1.readline()
    dataArray["tcp1"] = pullData_tcp1.read().split("\n")
    pullData_tcp1.close()

    pullData_tcp2 = open("rate_tcp2", "r")
    line = pullData_tcp2.readline()
    if line != "":
        base_line["tcp2"] = int(line)

    for k in range(6):
        pullData_tcp2.readline()
    dataArray["tcp2"] = pullData_tcp2.read().split("\n")
    pullData_tcp2.close()

    pullData_pcc1 = open("rate_pcc1", "r")
    line = pullData_pcc1.readline()
    if line != "":
        base_line["pcc1"] = int(line)
    dataArray["pcc1"] = pullData_pcc1.read().split("\n")
    pullData_pcc1.close()


    pullData_pcc2 = open("rate_pcc2", "r")
    line = pullData_pcc2.readline()
    if line != "":
        base_line["pcc2"] = int(line)
    dataArray["pcc2"] = pullData_pcc2.read().split("\n")
    pullData_pcc2.close()
    base_point = -1

    for flow in base_line:
        if greater(base_point, base_line[flow]):
            base_point = base_line[flow]

    time_offset= copy.deepcopy(base_line)
    for flow in time_offset:
        time_offset[flow] = time_offset[flow] - base_point

    axarr[1].clear()
    axarr[1].set_xlabel("Time (s)")
    axarr[1].set_ylabel("Throughput (Mbit/s)")
    axarr[0].set_title("TCP convergence demo")
    axarr[0].clear()
    axarr[0].set_xlabel("Time (s)")
    axarr[0].set_ylabel("Throughput (Mbit/s)")
    axarr[0].set_title("PCC convergence demo")

    for flow in dataArray:
        xar = []
        yar = []
        if flow == "tcp1" or flow == "tcp2":
            for eachLine in dataArray[flow]:
                if eachLine != "":
                    tmp = eachLine.split(" ")
                    y = tmp[len(tmp)-2]
                    yar.append(float(y))
            for k in range(len(dataArray[flow])-1):
                xar.append(k+time_offset[flow])
            axarr[1].plot(xar, yar)
        else:
            for eachLine in dataArray[flow]:
                if eachLine != "":
                    x, y = eachLine.split(' ')
                    xar.append(float(x) + time_offset[flow])
                    yar.append(float(y))
            axarr[0].plot(xar, yar)


ani = animation.FuncAnimation(f, animate_together, interval=2000)
f.set_size_inches(8,6)
mng = plt.get_current_fig_manager()
plt.show()
mng.frame.Maximize(True)
