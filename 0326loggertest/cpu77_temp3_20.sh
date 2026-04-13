#!/bin/bash

#======결과물 path======#
eembc=/data/local/tmp/CY/models/EEMBC_SH
logger=/data/local/tmp/SM/logger/CY/logger_1s_pixel7a

output_dir=/data/local/tmp/SM/0326loggertest/outputs
perf_dir=/data/local/tmp/SM/0326loggertest/perf

eembc_output=${output_dir}/eembc_out_1big_cpu7_temp3_20.txt


# #======코어 번호 정하기======#
# BIGA=80
# BIGB=40
# BIG1=80
# BIG2=C0

# MIDA=20
# MIDB=10
# MIDC=08
# MIDD=04
# MIDE=02
# MIDF=01

# BIG2MID6=FF
# ===== CPU mask (per core) =====
LIT0=01
LIT1=02
LIT2=04
LIT3=08
MID4=10
MID5=20
BIG6=40
BIG7=80

# ===== cluster mask =====
LITTLE=0F   # cpu 0~3
MID=30      # cpu 4~5
BIG=C0      # cpu 6~7




# test start
echo "Starting benchmark tests..."

# device_init
sleep 10

exe=nnet

# nnet
# 
echo "MID4=${MID4}"
echo "logger=${logger}"
echo "output=${output_dir}/${exe}_1BIG_cpu77_temp3_20.txt"

ls -l ${logger}
ls -ld ${output_dir}

# logger 실행 직전 확인
taskset ${MID4} ${logger} ${output_dir}/${exe}_1BIG_cpu77_temp3_20.txt &
echo "logger PID=$!"

# benchmark도 동일
echo "BIG7=${BIG7}"
echo "eembc=${eembc}/${exe}"

taskset ${BIG7} ${eembc}/${exe} -v0 -c1 -w1 -i110 >> ${eembc_output} &
echo "bench PID=$!"

#============#
APP_PID=$!
echo "APP PID = ${APP_PID}"
echo "=== Main thread ==="
#=========#
grep -E 'Cpus_allowed_list' /proc/${APP_PID}/status


#===========#
echo "=== Threads ==="
for t in /proc/${APP_PID}/task/*; do
    tid=${t##*/}
    echo "TID: $tid"
    grep -E 'Cpus_allowed_list' /proc/${tid}/status
done
#============#
wait ${APP_PID}

kill -9 $(pgrep benchmark)
kill -9 $(pgrep simpleperf)
kill -9 $(pgrep logger)
sleep 5

echo "Benchmark tests completed."