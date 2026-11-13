#!/bin/sh
# Runs the REAL event_db / dispatcher / department code on the FreeRTOS POSIX port (Linux / WSL).
# TFTP is replaced by reading the event files from ../../events.
#   phase A: 100 ms downloads    phase B: burst (severity order)    phase C: overload 10 ms
set -e
cd "$(dirname "$0")"
[ -d FreeRTOS-Kernel ] || git clone -q --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git
K=FreeRTOS-Kernel; R=../../..
gcc -std=gnu11 -O1 -Wall -Wextra -Wno-unused-parameter -Iinc -I$K/include \
  -I$K/portable/ThirdParty/GCC/Posix -I$K/portable/ThirdParty/GCC/Posix/utils -I$R/RTG/Inc \
  sim_main.c $R/RTG/Src/event_db.c $R/RTG/Src/event_db_core.c $R/RTG/Src/dispatcher.c \
  $R/RTG/Src/department.c $R/RTG/Src/city_stats.c $R/RTG/Src/log.c $R/RTG/Src/city_event.c \
  $R/RTG/Src/tests.c $R/RTG/Src/tftp_proto.c \
  $K/tasks.c $K/queue.c $K/list.c $K/portable/MemMang/heap_4.c \
  $K/portable/ThirdParty/GCC/Posix/port.c $K/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c \
  -lpthread -o sim
./sim ../../events | tee sim_output.txt | grep -E "@|passed|#9000"
