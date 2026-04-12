#!/system/bin/sh
set -eu

echo "[DBG] script start"

########################################
# user knobs
########################################
SKIN_LIST="off"
WORKLOAD_LIST="nnet sha cjpeg parser core linear_alg loops radix2 zip"
PERF_LOCK=0
COOLDOWN_SEC=300

########################################
# fixed paths
########################################
eembc=/data/local/tmp/SM/models/EEMBC_SH
logger=/data/local/tmp/SM/logger/logger_10ms_pixel7a1

########################################
# taskset masks
########################################
BIG1=80
LITA=08

echo "[DBG] vars: BIG1=$BIG1 LITA=$LITA"
echo "[DBG] eembc=$eembc"
echo "[DBG] logger=$logger"

########################################
# dirs
########################################
OUT_DIR=./outputs
mkdir -p "$OUT_DIR"
echo "[DBG] OUT_DIR=$OUT_DIR"

########################################
# helpers
########################################
perf_on() {
  echo "[DBG] perf_on (PERF_LOCK=$PERF_LOCK)"
  [ "$PERF_LOCK" -eq 1 ] || return 0

  for cpu_path in /sys/devices/system/cpu/cpu[0-9]*; do
    gov="$cpu_path/cpufreq/scaling_governor"
    [ -f "$gov" ] || continue
    echo performance > "$gov" 2>/dev/null || true
  done
}

perf_off() {
  echo "[DBG] perf_off (PERF_LOCK=$PERF_LOCK)"
  [ "$PERF_LOCK" -eq 1 ] || return 0

  for cpu_path in /sys/devices/system/cpu/cpu[0-9]*; do
    gov="$cpu_path/cpufreq/scaling_governor"
    [ -f "$gov" ] || continue
    echo schedutil > "$gov" 2>/dev/null || true
  done
}


pick_iter() {
  case "$1" in
    nnet) echo 22728 ;;
    sha) echo 759026 ;;
    cjpeg) echo 538374 ;;
    parser) echo 132479 ;;
    core) echo 4999 ;;
    linear_alg) echo 465941 ;;
    loops) echo 18148 ;;
    radix2) echo 1980988 ;;
    zip) echo 301026 ;;
    *) echo 100 ;;
  esac
}

start_logger() {
  echo "[DBG] start_logger: taskset $LITA $logger $1"
  taskset $LITA "$logger" "$1" &
  LOGGER_PID=$!
  echo "[DBG] LOGGER_PID=$LOGGER_PID"
}

stop_logger() {
  if [ -n "${LOGGER_PID:-}" ]; then
    echo "[DBG] stop_logger: kill $LOGGER_PID"
    kill "$LOGGER_PID" 2>/dev/null || true
  else
    echo "[DBG] stop_logger: no LOGGER_PID"
  fi
}

run_workload() {
  echo "[DBG] run_workload: taskset $BIG1 $eembc/$1 -i $2"
  taskset $BIG1 "$eembc/$1" -v0 -c1 -w1 -i"$2" >> "$3"
}

trap 'echo "[DBG] trap EXIT"; stop_logger; perf_off' EXIT

########################################
# main
########################################
for SKIN_MODE in $SKIN_LIST; do
  echo "[DBG] === SKIN_MODE=$SKIN_MODE ==="
  perf_on

  for wl in $WORKLOAD_LIST; do
    echo "[DBG] --- workload=$wl ---"
    iter="$(pick_iter "$wl")"
    tag="wl-$wl-iter-$iter"
    out="$OUT_DIR/$tag.txt"

    echo "[DBG] iter=$iter"
    echo "[DBG] out=$out"

    start_logger "$out"
    start_ts=$(date +%s)
    echo "[DBG] start_ts=$start_ts"

    run_workload "$wl" "$iter" "$out"
    rc=$?
    echo "[DBG] workload rc=$rc"

    stop_logger
    end_ts=$(date +%s)
    echo "[DBG] end_ts=$end_ts"

    echo "[DONE] wl=$wl iter=$iter rc=$rc elapsed=$((end_ts-start_ts))s" >> "$out"
    sleep 5
  done

  perf_off
  echo "[DBG] cooldown $COOLDOWN_SEC sec"
  sleep "$COOLDOWN_SEC"
done

echo "[DBG] ALL EXPERIMENTS COMPLETED"
