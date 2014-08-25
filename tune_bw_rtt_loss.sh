ssh -t -t -o StrictHostKeyChecking=no modong2@bridge0.demopair1.uiucscheduling.emulab.net "~/setpipe.sh $1 $2 $3 $4 $5 $6"
ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair1.uiucscheduling.emulab.net "~/setloss.sh $8"
ssh -t -t -o StrictHostKeyChecking=no modong2@bridge0.demopair2.uiucscheduling.emulab.net "~/setpipe.sh $1 $2 $3 $4 $5 $6"
ssh -t -t -o StrictHostKeyChecking=no modong2@receiver1.demopair2.uiucscheduling.emulab.net "~/setloss.sh $8"

