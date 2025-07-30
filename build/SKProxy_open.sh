#!/bin/bash

#program_path=/home/www/skproxy_bin/SKProxy
program_path=$(dirname $(readlink -f "$0"))

cd $program_path
date=$(date '+%Y%m%d')
date+="_ServerScreenInfo"
filename=$date

if [ "$1" == "cpu" ]; then
    cp ../sk_proxy_cpu SKProxy
else
    cp ../sk_proxy SKProxy
fi

chmod 755 SKProxy

./SKProxy 2>&1 |tee -a ./SKProxyLog/$filename&
