#!/bin/bash
ps -ae|
while read pid tty time cmd
do
	if [ "$cmd" = SKShmAP ]
	then
		echo $pid "killed"
		kill -15 $pid
		kill -9 $pid
	fi
done

sleep 6s

date=$(date '+%Y%m%d')
date+="_ServerScreenInfo"
filename=$date

program_path=$(dirname $(readlink -f "$0"))
cd $program_path
cp ../sk_shmap SKShmAP
chmod 755 SKShmAP

./SKShmAP 2>&1 |tee -a ./SKProxyLog/$filename&
