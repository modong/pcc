import sys
import time
import logging
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import os
class NewSnapshotEventHandler(FileSystemEventHandler):
    def __init__(self):
        self.param = {}
        self.param["Bandwidth"] = "100Mbit/s"
        self.param["RTT"] = "30ms"
        self.param["BufferSize"] = "50slots"
        self.param["LossRate"] = "0"
        self.is_start = False

    def on_modified(self, event):
        if event.is_directory is True:
            return
        with open("./parameters.txt", "r") as f:
            param = f.read()
            lines = param.split("\n")
            tmp_param = {}
            for line in lines:
                if line == "stop" and self.is_start:
                    os.system("./stop_demo_compare_tcp_and_pcc.sh")
                    self.is_start = False
                    return
                elif line == "start" and not self.is_start:
                    os.system("./run_demo_compare_tcp_and_pcc.sh")
                    self.is_start = True
                elif line != "" and line !="start" and line != "stop":
                    tmp_param[line.split(" ")[0]] = line.split(" ")[1]
            if tmp_param == self.param:
                return
            else:
                self.param = tmp_param
                if not self.is_start:
                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair2.UIUCScheduling.emulab.net \"killall iperf && ./setup.bash {}\"".format(self.param["protocol"]))
                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair2.UIUCScheduling.emulab.net \"nohup python ~/run_iperf.py &\"")
                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair2.UIUCScheduling.emulab.net \"killall iperf && ./setup.bash {}\"".format(self.param["protocol"]))
                os.system("python ./tune_bw_rtt_loss.py -b {} -d {} -q {} -l {}".format(self.param["Bandwidth"],
                                                                                        self.param["RTT"],
                                                                                        self.param["BufferSize"],
                                                                                        self.param["LossRate"]))

if __name__ == "__main__":
    path = '.'
    event_handler = NewSnapshotEventHandler()
    observer = Observer()
    observer.daemon = True
    observer.schedule(event_handler, path, recursive=False)
    observer.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
