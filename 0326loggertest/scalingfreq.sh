#!/system/bin/sh
# Fix all CPUs to 500000 kHz and run nnet with logger

# ======= Path =======
eembc=/data/local/tmp/CY/models/EEMBC_SH
logger=/data/local/tmp/SM/logger/7alogger/logger_1s_pixel7a_logx

output_dir=/data/local/tmp/SM/0326loggertest/outputs

# ======= CPU mask =======
MID4=10
BIG7=80

exe=nnet

# ======= Fix all CPUs to 500000 kHz =======
echo "=== Fixing all CPUs to 500000 kHz ==="
for i in $(seq 0 7); do
    CPUFREQ_PATH="/sys/devices/system/cpu/cpu${i}/cpufreq"
    if [ -f "${CPUFREQ_PATH}/scaling_max_freq" ]; then
        echo 500000 > ${CPUFREQ_PATH}/scaling_max_freq
        echo 500000 > ${CPUFREQ_PATH}/scaling_min_freq
        echo "cpu${i}: $(cat ${CPUFREQ_PATH}/scaling_cur_freq)"
    else
        echo "cpu${i}: cpufreq not found, skip"
    fi
done

echo "=== Freq lock done ==="
sleep 2

# ======= Logger 실행 =======
taskset ${MID4} ${logger} ${output_dir}/${exe}_500kHz_allcpu.txt &
echo "logger PID=$!"

# ======= nnet 실행 =======
taskset ${BIG7} ${eembc}/${exe} -v0 -c1 -w1 -i110 &
APP_PID=$!
echo "nnet PID=${APP_PID}"

# ======= Thread affinity 확인 =======
echo "=== Main thread ==="
grep -E 'Cpus_allowed_list' /proc/${APP_PID}/status

echo "=== Threads ==="
for t in /proc/${APP_PID}/task/*; do
    tid=${t##*/}
    echo "TID: $tid"
    grep -E 'Cpus_allowed_list' /proc/${tid}/status
done

# ======= 완료 대기 및 정리 =======
wait ${APP_PID}

kill -9 $(pgrep logger)
sleep 5

# ======= Freq 복원 =======
echo "=== Restoring freq ==="
for i in $(seq 0 7); do
    CPUFREQ_PATH="/sys/devices/system/cpu/cpu${i}/cpufreq"
    if [ -f "${CPUFREQ_PATH}/scaling_max_freq" ]; then
        echo 2850000 > ${CPUFREQ_PATH}/scaling_max_freq
        echo 500000  > ${CPUFREQ_PATH}/scaling_min_freq
    fi
done

echo "=== Done ==="