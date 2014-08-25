import sys
import time
import logging
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import os
class NewSnapshotEventHandler(FileSystemEventHandler):
    def __init__(self):
        self.flow1_start = False
        self.flow2_start = False

    def on_modified(self, event):
        if event.is_directory is True:
            return
        with open("./parameters_convergence.txt", "r") as f:
            param = f.read()
            lines = param.split("\n")
            for line in lines:
#                if line == "reset":
#                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair3.UIUCScheduling.emulab.net \"killall appclient && echo "">/local/rate_pcc\"")
#                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair3.UIUCScheduling.emulab.net \"killall appclient && echo "">/local/rate_pcc\"")
#                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair4.UIUCScheduling.emulab.net \"killall iperf && echo ""> /local/rate_tcp\"")
#                    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair4.UIUCScheduling.emulab.net \"killall iperf && echo ""> /local/rate_tcp\"")
#                    return
                if line != "":
                    flow_name = line.split(" ")[0]
                    action = line.split(" ")[1]
                    if flow_name == "flow1":
                        if self.flow1_start and action == "stop":
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair3.UIUCScheduling.emulab.net \"killall appclient\"")
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair4.UIUCScheduling.emulab.net \"killall iperf\"")
                            self.flow1_start = False
                        if not self.flow1_start and action == "start":
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair3.UIUCScheduling.emulab.net \"~/demo_run_pcc_convergence.sh 3 &\"")
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair4.UIUCScheduling.emulab.net \"~/demo_run_tcp_convergence.sh 3 &\"")
                            self.flow1_start = True
                    if flow_name == "flow2":
                        if self.flow2_start and action == "stop":
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair3.UIUCScheduling.emulab.net \"killall appclient\"")
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair4.UIUCScheduling.emulab.net \"killall iperf\"")
                            self.flow2_start = False
                        if not self.flow2_start and action == "start":
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair3.UIUCScheduling.emulab.net \"~/demo_run_pcc_convergence.sh 4 &\"")
                            os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@sender2.demopair4.UIUCScheduling.emulab.net \"~/demo_run_tcp_convergence.sh 4 &\"")
                            self.flow2_start = True


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
