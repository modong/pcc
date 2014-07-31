import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import os

f, axarr = plt.subplots(2, 1, sharey=True, sharex=True)

def animate_tcp(i):
    os.system("scp modong2@sender1.congestion7.uiucscheduling.emulab.net:/local/rate /home/mo/pcc/rate")
    pullData = open("rate", "r")
    for k in range(6):
        pullData.readline()
    dataArray = pullData.read().split("\n")
    pullData.close()
    xar = []
    yar = []
    for eachLine in dataArray:
        if eachLine != "":
            tmp = eachLine.split(" ")
            y = tmp[len(tmp)-2]
            yar.append(float(y))
    for k in range(len(dataArray)-1):
        xar.append(k*0.5)
    ax1.clear()
    ax1.plot(xar, yar)

def animate2(i):
    os.system("scp modong2@sender1.congestion7.uiucscheduling.emulab.net:/local/utility /home/mo/pcc/utility")
    pullData = open("utility", "r")
    dataArray = pullData.read().split("\n")
    pullData.close()
    xar = []
    yar1 = []
    yar2 = []
    seq = 1
    for eachLine in dataArray:
        if eachLine != "":
            y1 = eachLine.split('\t')[0]
            y2 = eachLine.split('\t')[1]
            xar.append(seq)
            seq = seq + 1
            yar1.append(float(y1))
            yar2.append(float(y2))
    ax1.clear()
    ax1.plot(xar, yar1, xar, yar2)





def animate(i):
    os.system("scp modong2@sender1.congestion7.uiucscheduling.emulab.net:/local/rate /home/mo/pcc/rate")
    pullData = open("rate", "r")
    dataArray = pullData.read().split("\n")
    pullData.close()
    xar = []
    yar = []
    for eachLine in dataArray:
        if eachLine != "":
            x, y = eachLine.split(' ')
            xar.append(float(x))
            yar.append(float(y))
    ax1.clear()
    ax1.plot(xar, yar)


def animate_together(i):
    os.system("scp modong2@sender1.demopair1.uiucscheduling.emulab.net:/local/rate* /home/mo/pcc/")
    os.system("scp modong2@sender1.demopair2.uiucscheduling.emulab.net:/local/rate* /home/mo/pcc/")
    pullData_tcp = open("rate_tcp", "r")
    for k in range(6):
        pullData_tcp.readline()
    dataArray_tcp = pullData_tcp.read().split("\n")
    pullData_tcp.close()
    xar = []
    yar = []
    for eachLine in dataArray_tcp:
        if eachLine != "":
            tmp = eachLine.split(" ")
            y = tmp[len(tmp)-2]
            yar.append(float(y))
    for k in range(len(dataArray_tcp)-1):
        xar.append(k)
    axarr[1].clear()
    axarr[1].set_xlabel("Time (s)")
    axarr[1].set_ylabel("Throughput (Mbit/s)")
    axarr[1].set_title("TCP's performance")
    axarr[1].plot(xar, yar)

    pullData = open("rate_pcc", "r")
    dataArray = pullData.read().split("\n")
    pullData.close()
    xar = []
    yar = []
    #xar = [0, 0.5, 1, 1.5, 2.0, 2.5]
    #yar = [0, 37.3, 39.3, 37.2, 40.3, 42.9]
    count = 0
    for eachLine in dataArray:
        count = count+1
        if count > -1:
          if eachLine != "":
              x, y = eachLine.split(' ')
              xar.append(float(x))
              yar.append(float(y))
    axarr[0].clear()
    axarr[0].set_xlabel("Time (s)")
    axarr[0].set_ylabel("Throughput (Mbit/s)")
    axarr[0].set_title("PCC's performance")
    axarr[0].plot(xar, yar)

ani = animation.FuncAnimation(f, animate_together, interval=2000)
f.set_size_inches(8,6)
mng = plt.get_current_fig_manager()
plt.show()
mng.frame.Maximize(True)
