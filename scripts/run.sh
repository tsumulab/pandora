#!/bin/bash -e

# Resolve the repository root from this script's location and run from there,
# so paths like ./inputs and ./build work regardless of the caller's directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

is_setaffinity=false
# is_setaffinity=true

input_dir="./inputs"

Progs=("genome" "intruder" "kmeans_high" "kmeans_low" "labyrinth"
        "ssca2" "vacation_high" "vacation_low" "yada" "bayes")

largeArgs=(
  "-g16384 -s64 -n16777216" # genome
  "-a10 -l128 -n262144 -s1" # intrueder
#  "-m15 -n15 -t0.00001 -i ${input_dir}/kmeans/random-n65536-d32-c16.txt" # kmeans_high
#  "-m40 -n40 -t0.00001 -i ${input_dir}/kmeans/random-n65536-d32-c16.txt" # kmeans_low
  "-m15 -n15 -t0.00001 -i ${input_dir}/kmeans/random-n1048576-d32-c16.txt" # kmeans_high
  "-m40 -n40 -t0.00001 -i ${input_dir}/kmeans/random-n1048576-d32-c16.txt" # kmeans_low

  "-i ${input_dir}/labyrinth/random-x512-y512-z7-n512.txt" # labyrinth
  "-s20 -i1.0 -u1.0 -l3 -p3" # ssca2
  "-n4 -q60 -u90 -r1048576 -t4194304" # vacation_high
  "-n2 -q90 -u98 -r1048576 -t4194304" # vacation_low
  "-a15 -i ${input_dir}/yada/ttimeu1000000.2" # yada
  "-v32 -r4096 -n10 -p40 -i2 -e8 -s1" # bayes
)

smallArgs=(
  "-g256 -s16 -n16384" # genome
  "-a10 -l4 -n2038 -s1" # intrueder
  "-m15 -n15 -t0.05 -i ${input_dir}/kmeans/random-n2048-d16-c16.txt" # kmeans_high
  "-m40 -n40 -t0.05 -i ${input_dir}/kmeans/random-n2048-d16-c16.txt" # kmeans_low
  "-i ${input_dir}/labyrinth/random-x32-y32-z3-n96.txt" # labyrinth
  "-s13 -i1.0 -u1.0 -l3 -p3" # ssca2
  "-n4 -q60 -u90 -r16384 -t4096" # vacation_high
  "-n2 -q90 -u98 -r16384 -t4096" # vacation_low
  "-a20 -i ${input_dir}/yada/633.2" # yada
  "-v32 -r1024 -n2 -p20 -s0 -i2 -e2" # bayes
)


args=()
do_small=false # default is large inputs
do_debug=false
do_perf=false
while (( $# > 0 )); do
  case $1 in
    -h | --help)
      echo "./run.sh <backend> <program> <num_of_core> [<thr_per_core>]"
      exit 0
      ;;
    -l | --large)
      do_small=false
      ;;
    -s | --small)
      do_small=true
      ;;
    -g | --debug)
      do_debug=true
      ;;
    -p | --perf)
      do_perf=true
      ;;
    -* | --*)
      echo "No such option : $1"
      exit 1
      ;;
    *)
      args+=($1)
      ;;
  esac
  shift
done


# Do debug or perf
prefix=""
if "${do_debug}" && "${do_perf}"; then
  echo "Either debug or perf option is available."
  exit 1
elif "${do_debug}"; then
  prefix="gdb --args"
elif "${do_perf}"; then
  # prefix="perf record -a -o ./Perf_data/`TZ=JST-9 date +%y-%m-%d_%H-%M`"
  prefix="perf stat -r 10 -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses"
fi


# Check if backend exists
backend=${args[0]}
if find ./build -type d | grep -qx "./build/${backend}"; then
  :
else
  exit 1
fi


# Set the number of threads
nthr=${args[2]}
if [[ ${args[0]} = "seq" ]]; then nthr=1; fi


# Check benchmark
for i in `seq 0 $((${#Progs[@]}-1))`; do
  if [[ ${args[1]} = ${Progs[i]} ]]; then
    if [[ $i == 2 ]] || [[ $i == 3 ]]; then
      cmd="${prefix} ./build/${backend}/kmeans -p${nthr}"
    elif [[ $i == 6 ]] || [[ $i == 7 ]]; then
      cmd="${prefix} ./build/${backend}/vacation -c${nthr}"
    else
      cmd="${prefix} ./build/${backend}/${Progs[i]} -t${nthr}"
    fi
    if "${do_small}"; then
      cmd=${cmd}" ${smallArgs[i]}"
    else
      cmd=${cmd}" ${largeArgs[i]}"
    fi
    break
  fi
done


# Run
if [[ -n ${cmd} ]]; then
  if [[ ${#args[@]} == 4 ]]; then
    export THR_PER_CORE=${args[3]}
  fi

  # if "${is_setaffinity}"; then
  #   cpu_cores=`fgrep 'cpu cores' /proc/cpuinfo | sort -u | sed 's/.*: //'`
  #   if [ "${nthr}" -lt "${cpu_cores}" ]; then
  #     export FibreCpuSet="1-${nthr}"
  #   elif [ "${nthr}" -eq "${cpu_cores}" ]; then
  #     export FibreCpuSet="1-$((${nthr}-1)),0"
  #   else
  #     echo "== WARNING ===================================================================="
  #     echo " The number of specified execution cores exceeds the number of PHYSICAL cores."
  #     echo " This may have some effect on the execution."
  #     echo " '`hostname`' has ${cpu_cores} cores."
  #     echo "==============================================================================="
  #   fi
  # fi

  ${cmd}
fi