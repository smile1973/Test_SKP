#!/bin/bash
ps -ae|
while read pid tty time cmd
do
	if [ "$cmd" = sk_proxy ]
	then
		echo $pid "killed"
		kill -15 $pid
		kill -9 $pid
	fi
done

sleep 5s

date=$(date '+%Y%m%d')
date+="_ServerScreenInfo"
filename=$date

program_path=$(dirname $(readlink -f "$0"))
cd $program_path
#cp ../sk_proxy SKProxy
#chmod 755 SKProxy

./sk_proxy 2>&1 |tee -a ./SKProxyLog/$filename&
