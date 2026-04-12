#!/system/bin/sh

# ======= 경로 설정 =======
# 이전에 말씀하신 경로들을 기준으로 설정했습니다.
logger="/data/local/tmp/SM/logger/0408/logger_1ms_pixel7a_0408"
output_dir="/data/local/tmp/SM/0326loggertest/outputs"
output_file="${output_dir}/0412exp1_max$(date +%m%d_%H%M%S).txt"

mkdir -p ${output_dir}
echo "Initialization done."

# ======= 2. 로거 실행 (Background) =======
# MID 코어(cpu4)에 할당하여 실행
taskset 10 ${logger} ${output_file} &
LOGGER_PID=$!
echo "Logger started (PID: ${LOGGER_PID}) -> Output: ${output_file}"

# 최저 클럭 상태로 5초 유지
echo "Maintaining Min Freq for 5s..."
sleep 5
# ======= 1. 거버너 performance로 변경 및 최저 클럭 초기화 =======
echo "=== Step 1: Set Governor to Performance & Min Freq ==="

# 전 코어 공통: 거버너부터 고정
for i in $(seq 0 7); do
    echo "performance" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
done
# ======= 1. 모든 코어 최저 클럭으로 초기화 (안정화) =======
echo "=== Step 1: Initialize to Min Freq ==="
for i in $(seq 0 3); do echo 300000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq; echo 300000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq; done
for i in $(seq 4 5); do echo 400000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq; echo 400000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq; done
for i in $(seq 6 7); do echo 500000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq; echo 500000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq; done



# ======= 3. 클럭 1단계 상승 (Trigger) =======
echo "=== Step 2: Scaling Up (Trigger) ==="
echo "Trigger Time: $(date '+%H:%M:%S.%N')"

# # LITTLE (0-3) -> 574000
# for i in $(seq 0 3); do
#     echo 574000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq
#     echo 574000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq
# done

# # MID (4-5) -> 553000
# for i in $(seq 4 5); do
#     echo 553000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq
#     echo 553000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq
# done

# BIG (6-7) -> 851000
for i in $(seq 6 7); do
    echo 2850000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq
    echo 2850000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq
done

# 상승된 클럭으로 5초 더 로깅
echo "Logging at higher freq for 5s..."
sleep 5

# ======= 4. 정리 및 복구 =======
kill -9 ${LOGGER_PID}
echo "Logger stopped."

# # 원복 (필요한 경우 실행)
# echo "Restoring default settings..."
# for i in $(seq 0 7); do
#     echo 2850000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq
#     echo 500000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq
# done

echo "Test Complete. Output saved to: ${output_file}"