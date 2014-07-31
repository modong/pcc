import os


def do_args():
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("-b", "--bandwidth", dest="bw", default="100Mbit/s",
            help="URL of rabbitmq server")
    p.add_argument("-d","--delay", dest="delay", default="30ms",
            help="URL of rabbitmq server")
    p.add_argument("-q","--buffer", dest="queue", default="50slots",
            help="user name for the rabbitmq server")
    p.add_argument("-l", '--loss', dest='loss', default="0",
            help="password for rabbitmq server")
    return p.parse_args()

def main():
    opts = do_args()
    if opts.loss != "0":
        print "I am here"
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair1.uiucscheduling.emulab.net \"~/setloss.sh {}\"".format(opts.loss))
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair2.uiucscheduling.emulab.net \"~/setloss.sh {}\"".format(opts.loss))
    elif opts.loss == "0":
        print "iiiiiii"
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair1.uiucscheduling.emulab.net \"sudo iptables -F\"")
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair2.uiucscheduling.emulab.net \"sudo iptables -F\"")

    os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@bridge0.demopair1.uiucscheduling.emulab.net \"~/setpipe.sh {} {} {} {}\"".format("bw",
                                                                                                                                              opts.bw,
                                                                                                                                              "delay",
                                                                                                                                              opts.delay))

    if opts.queue != "50slots":
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@bridge0.demopair2.uiucscheduling.emulab.net \"~/setpipe.sh {} {} {} {} {} {}\"".format("bw",
                                                                                                                                                        opts.bw,
                                                                                                                                                        "delay",
                                                                                                                                                        opts.delay,
                                                                                                                                                        "queue",
                                                                                                                                                        opts.queue))
    else:
        os.system("ssh -t -t -o StrictHostKeyChecking=no modong2@bridge0.demopair2.uiucscheduling.emulab.net \"~/setpipe.sh {} {} {} {}\"".format("bw",
                                                                                                                                                  opts.bw,
                                                                                                                                                  "delay",
                                                                                                                                                  opts.delay))




if __name__ == "__main__":
    main()
