for i in {1..4..1}
do
    for j in {3..4..1}
    do
ssh -t -t -o StrictHostKeyChecking=no modong2@receiver$i.demopair$j.uiucscheduling.emulab.net "killall iperf"
ssh -t -t -o StrictHostKeyChecking=no modong2@receiver$i.demopair$j.uiucscheduling.emulab.net "~/demo_receiver_run.sh &"
ssh -t -t -o StrictHostKeyChecking=no modong2@receiver$i.demopair$j.uiucscheduling.emulab.net "nohup python ~/run_iperf.py &"
done
done
