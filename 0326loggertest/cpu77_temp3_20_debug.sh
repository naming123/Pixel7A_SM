#!/bin/bash

#======결과물 path======#
eembc=/data/local/tmp/CY/models/EEMBC_SH
logger=/data/local/tmp/SM/logger/CY/logger_1s_pixel7a

output_dir=/data/local/tmp/SM/0326loggertest/outputs
perf_dir=/data/local/tmp/SM/0326loggertest/perf

eembc_output=${output_dir}/eembc_out_1big_cpu7_temp3_20.txt
LOGGER_WORKDIR=/data/local/tmp/SM/0326loggertest
LOGGER_NAME="nnet_1BIG_cpu77_temp3_20"

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
LITTLE=0F
MID=30
BIG=C0

# ============================================================
# [DEBUG] 0. 환경 기본 정보
# ============================================================
echo "====== [DEBUG] ENVIRONMENT ======"
echo "Shell: $SHELL / $(readlink /proc/$$/exe 2>/dev/null)"
echo "UID: $(id)"
echo "Date: $(date)"
echo "Kernel: $(uname -r)"

# ============================================================
# [DEBUG] 1. 바이너리 존재 및 실행 권한 확인
# ============================================================
echo ""
echo "====== [DEBUG] BINARY CHECK ======"
for bin in "${logger}" "${eembc}/nnet"; do
    if [ -f "$bin" ]; then
        echo "[OK] EXISTS   : $bin"
        ls -l "$bin"
        # ELF 아키텍처 확인 (file 명령 없으면 readelf 대체)
        if command -v file >/dev/null 2>&1; then
            file "$bin"
        else
            # ELF magic + arch 바이트 직접 읽기
            echo "  ELF class/arch: $(dd if=$bin bs=1 skip=4 count=2 2>/dev/null | od -An -tx1)"
        fi
    else
        echo "[ERR] NOT FOUND: $bin"
    fi
done

# ============================================================
# [DEBUG] 2. output 디렉토리 권한 및 쓰기 테스트
# ============================================================
echo ""
echo "====== [DEBUG] OUTPUT DIR CHECK ======"
echo "output_dir=${output_dir}"
if [ -d "${output_dir}" ]; then
    echo "[OK] dir exists"
    ls -ld "${output_dir}"
    # 실제 쓰기 가능 여부 테스트
    TEST_FILE="${output_dir}/.write_test_$$"
    if touch "${TEST_FILE}" 2>/dev/null; then
        echo "[OK] write permission OK"
        rm -f "${TEST_FILE}"
    else
        echo "[ERR] CANNOT WRITE to ${output_dir}"
    fi
else
    echo "[ERR] dir NOT FOUND: ${output_dir} → 생성 시도"
    mkdir -p "${output_dir}" && echo "[OK] created" || echo "[ERR] mkdir failed"
fi

# eembc_output 파일 미리 생성 (fwrite NULL 방지)
echo ""
echo "====== [DEBUG] PRE-CREATE OUTPUT FILE ======"
touch "${eembc_output}" 2>/dev/null && echo "[OK] ${eembc_output} pre-created" \
    || echo "[ERR] cannot create ${eembc_output}"
ls -l "${eembc_output}"

# ============================================================
# [DEBUG] 3. taskset 동작 확인
# ============================================================
echo ""
echo "====== [DEBUG] TASKSET CHECK ======"
if command -v taskset >/dev/null 2>&1; then
    echo "[OK] taskset found: $(command -v taskset)"
    # 현재 shell의 affinity 확인
    taskset -p $$ 2>/dev/null || echo "  taskset -p not supported"
else
    echo "[WARN] taskset not found in PATH"
fi
echo "MID4 mask = ${MID4}, BIG7 mask = ${BIG7}"

# ============================================================
# [DEBUG] 4. pgrep / kill 지원 확인
# ============================================================
echo ""
echo "====== [DEBUG] PGREP / KILL CHECK ======"
if command -v pgrep >/dev/null 2>&1; then
    echo "[OK] pgrep found: $(command -v pgrep)"
else
    echo "[WARN] pgrep NOT found → kill 라인에서 실패할 것"
fi
# Android sh에서 kill 문법 테스트 (존재하지 않는 PID로)
kill -0 99999999 2>&1 | head -1 || true

# ============================================================
# test start
# ============================================================
echo ""
echo "Starting benchmark tests..."
sleep 10

exe=nnet
LOGGER_OUT="${output_dir}/${exe}_1BIG_cpu77_temp3_20.txt"

echo "MID4=${MID4}"
echo "logger=${logger}"
echo "logger_output=${LOGGER_OUT}"

ls -l ${logger}
ls -ld ${output_dir}

# ============================================================
# logger 실행
# ============================================================
echo ""
echo "====== [DEBUG] LAUNCHING LOGGER ======"
mkdir -p ${LOGGER_WORKDIR}/log
cd ${LOGGER_WORKDIR}
echo "CMD: taskset ${MID4} ${logger} ${LOGGER_NAME}"
taskset ${MID4} ${logger} ${LOGGER_NAME} &
LOGGER_PID=$!
cd -
echo "logger PID=${LOGGER_PID}"
sleep 1
# logger가 살아있는지 확인
if kill -0 ${LOGGER_PID} 2>/dev/null; then
    echo "[OK] logger is running (PID=${LOGGER_PID})"
    echo "  logger affinity: $(taskset -p ${LOGGER_PID} 2>/dev/null)"
    cat /proc/${LOGGER_PID}/status | grep -E 'Name|Pid|Cpus_allowed'
else
    echo "[ERR] logger DIED immediately after launch"
fi
# logger가 실제로 파일을 만들었는지
sleep 1
if [ -f "${LOGGER_OUT}" ]; then
    echo "[OK] logger output file created: ${LOGGER_OUT}"
    ls -l "${LOGGER_OUT}"
else
    echo "[WARN] logger output file NOT yet created (may be delayed)"
fi

# ============================================================
# benchmark 실행
# ============================================================
echo ""
echo "====== [DEBUG] LAUNCHING BENCHMARK ======"
echo "BIG7=${BIG7}"
echo "CMD: taskset ${BIG7} ${eembc}/${exe} -v0 -c1 -w1 -i100 >> ${eembc_output}"
taskset ${BIG7} ${eembc}/${exe} -v0 -c1 -w1 -i100 >> ${eembc_output} 2>&1 &
BENCH_PID=$!
echo "bench PID=${BENCH_PID}"
sleep 1
# bench가 살아있는지 확인
if kill -0 ${BENCH_PID} 2>/dev/null; then
    echo "[OK] benchmark is running (PID=${BENCH_PID})"
else
    echo "[ERR] benchmark DIED immediately → eembc_output 확인:"
    cat "${eembc_output}" 2>/dev/null || echo "  (empty or missing)"
fi

APP_PID=${BENCH_PID}
echo "APP PID = ${APP_PID}"

# ============================================================
# [DEBUG] 5. CPU affinity 확인 (프로세스가 살아있을 때)
# ============================================================
echo ""
echo "====== [DEBUG] CPU AFFINITY CHECK ======"
echo "=== Main thread ==="
if [ -f /proc/${APP_PID}/status ]; then
    grep -E 'Cpus_allowed' /proc/${APP_PID}/status
else
    echo "[ERR] /proc/${APP_PID}/status not found (process may have died)"
fi

echo "=== Threads ==="
for t in /proc/${APP_PID}/task/*; do
    tid=${t##*/}
    echo "TID: $tid"
    grep -E 'Cpus_allowed' /proc/${tid}/status 2>/dev/null \
        || echo "  [WARN] /proc/${tid}/status not readable"
done

# ============================================================
# wait & cleanup
# ============================================================
wait ${APP_PID}
EXIT_CODE=$?
echo ""
echo "====== [DEBUG] BENCHMARK EXIT ======"
echo "exit code: ${EXIT_CODE}"
echo "eembc_output 내용 (마지막 20줄):"
tail -20 "${eembc_output}" 2>/dev/null || echo "  (file missing or empty)"

echo ""
echo "====== [DEBUG] LOGGER OUTPUT 확인 ======"
ls -l "${LOGGER_OUT}" 2>/dev/null || echo "  [WARN] logger output not found"
echo "outputs/ 디렉토리 전체:"
ls -lh "${output_dir}/"

# ============================================================
# cleanup: Android-safe kill
# ============================================================
echo ""
echo "====== [DEBUG] CLEANUP ======"
# logger graceful 종료 (flush 대기)
if kill -0 ${LOGGER_PID} 2>/dev/null; then
    echo "sending SIGTERM to logger (PID=${LOGGER_PID})"
    kill -15 ${LOGGER_PID}
    sleep 2
    kill -9 ${LOGGER_PID} 2>/dev/null
fi

for name in benchmark simpleperf; do
    PID_LIST=$(pgrep "$name" 2>/dev/null)
    if [ -n "${PID_LIST}" ]; then
        echo "killing ${name}: PIDs=${PID_LIST}"
        echo "${PID_LIST}" | xargs kill -9 2>/dev/null
    else
        echo "[INFO] no process named '${name}' found"
    fi
done

sleep 5
echo "Benchmark tests completed."