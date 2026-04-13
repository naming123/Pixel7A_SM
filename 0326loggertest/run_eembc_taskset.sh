#!/bin/sh

# $1 : benchmark name for execution
# $2 : # of iteration for benchmark
# $3 : # of cores to run benchmark
# $4 : log name for execution

bench_name=$1
num_iter=$2
num_core=$3
log_name=$4
task_setting=$5



taskset 0F ../logger/CY/logger_argv_100ms_thermal_test ${bench_name}_${log_name}_${num_iter}i_${num_core}c &
cd /data/local/tmp/CY/models/EEMBC_SH
taskset -a ${task_setting} ./${bench_name} -v0 -c${num_core} -w1 -i${num_iter} > /data/local/tmp/SM/0326loggertest/outputs/${bench_name}_${log_name}_${num_iter}i_${num_core}c_perf.txt
kill -9 $(pgrep logger)
