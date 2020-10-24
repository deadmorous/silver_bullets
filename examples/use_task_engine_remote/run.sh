#!/bin/bash

echo "Server ip " $1

current_ip=$(hostname --ip-address)
binpath=$2

if [ "$current_ip" == "$1" ]; then
    $binpath/use_remote_task_engine_worker
else
    $binpath/use_remote_task_engine_main --host $1
fi
