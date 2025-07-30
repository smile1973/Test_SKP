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
