#!/bin/bash

# Resolve this script's location and the repository root (works on both the
# top-level run and the re-sourced pass below).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ -z "$__INCLUDE" ]]; then __INCLUDE=1 && . "$SCRIPT_DIR/msr.sh"


# thread num  ex) thr="1 2 4 8 10"
thr="1 2 `seq -s ' ' 4 4 64`"

# rep
rep=10

echo "thr = [${thr}], rep = ${rep}"


# format: measure <build> <program> <thr_per_core>
# measure seq genome 1
# measure tinystm genome 4
# measure fine-lock genome 1
measure coarse-lock genome 1


else

. "$ROOT_DIR/Logs/summarize.sh"

function measure() {
  use=$1; prog=$2; tpc=$3;

  echo "Start : ${use} ${prog}"

  if [[ ${use} = "seq" ]]; then
    _thr=`yes 1 | head -n ${rep} | tr '\n' ' '`;
    _rep=1
  else
    _thr=${thr}
    _rep=${rep}
  fi

  OUTDIR="$ROOT_DIR/Logs/`TZ=JST-9 date +%y-%m-%d`/`TZ=JST-9 date +%H-%M`_${use}_${prog}@`hostname`"
  mkdir -p ${OUTDIR}

  for i in `seq 1 ${_rep}`; do
    progress_bar $i ${_rep}
    OUTFILE=${OUTDIR}/${prog}_${i}.txt
    hostname > ${OUTFILE}
    for t in ${_thr}; do
      "$SCRIPT_DIR/run.sh" ${use} ${prog} ${t} ${tpc} >> ${OUTFILE}
      echo -e "\n\n" >> ${OUTFILE}
    done
  done

  summarize ${OUTDIR} "${use}"

  echo -e "\nFin."
}

progress_bar() {
    current=$1; total=$2

    progress=$(($current * 50 / ${total}))

    bar="$(yes '#' | head -n ${progress} | tr -d '\n')"
    if [ -z "$bar" ]; then
        bar='_'
    fi

    printf "\r  [%-50s] (%d/%d)" ${bar} ${current} ${total}
}

fi
