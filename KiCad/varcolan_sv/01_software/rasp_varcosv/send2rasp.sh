#/bin/bash

#PIADDR=192.168.30.44
PIADDR=$1

sshpass -p 'saetis' scp ./Debug/varcosv pi@$PIADDR:/data
sshpass -p 'saetis' scp ./varcosv.cfg pi@$PIADDR:/data
sshpass -p 'saetis' scp ../varcosv/database/*.db pi@$PIADDR:/data/database
sshpass -p 'saetis' scp ../varcosv/firmware/unpkg.sh pi@$PIADDR:/data/firmware


