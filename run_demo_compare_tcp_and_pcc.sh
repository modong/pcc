ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair1.uiucscheduling.emulab.net "~/demo_run_pcc.sh 4 &"
ssh -t -t -o StrictHostKeyChecking=no modong2@sender1.demopair2.uiucscheduling.emulab.net "~/demo_run_tcp.sh 4 &"
