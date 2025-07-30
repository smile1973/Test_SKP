#!/bin/bash

#program_path=/home/www/skproxy_bin/SKProxy
program_path=$(dirname $(readlink -f "$0"))

cd $program_path
date=$(date '+%Y%m%d')
date+="_ServerScreenInfo"
filename=$date

cp ../sk_shmap SKShmAP
chmod 755 SKShmAP

./SKShmAP 2>&1 |tee -a ./SKProxyLog/$filename&
