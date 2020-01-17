#!/bin/bash

while true; 
do
    
    # Calculate current memory usage and store it to the xenstore
    # Grep MemTotal and MemAvailable from /proc/meminfo and calculate the memory usage
    mem_free=$(grep MemFree: /proc/meminfo | awk '{print $2}')
    mem_total=$(grep MemTotal: /proc/meminfo | awk '{print $2}')

    mem_usage=$((mem_total - mem_free))

    xenstore-write data/memload "${mem_usage}"
    xenstore-write data/memtotal "${mem_total}"

    # Calculate current cpu usage and store it to the xenstore
    # Grep cpu from /proc/stat and calculate usage
    period=0.4
    cpu_usage=$(awk '{u=$2+$4; t=$2+$4+$5; if (NR==1){u1=u; t1=t;} else print ($2+$4-u1) * 100 / (t-t1);}' \
            <(grep 'cpu ' /proc/stat) <(sleep ${period}; grep 'cpu ' /proc/stat))

    xenstore-write data/cpuload "${cpu_usage}"

    # CPU hot add
    for CPU_DIR in /sys/devices/system/cpu/cpu[0-9]*
    do
        CPU=${CPU_DIR##*/}
        CPU_STATE_FILE="${CPU_DIR}/online"

        if [ -f "${CPU_STATE_FILE}" ]; then
            if ! ["grep -qx 1 "${CPU_STATE_FILE}""]; then
                echo 1 > "${CPU_STATE_FILE}"
            fi
        fi
    done

    sleep 1;

done

