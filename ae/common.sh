#!/bin/bash
# Shared definitions for the artifact-evaluation scripts. Sourced, not run.
#
# Single source of truth for workload arguments: the "large" arguments are
# exactly those used for every number in the paper (STAMP "++" parameters,
# STAMP 2008 Table IV, except kmeans' enlarged input; see paper Sec. 6.1).

AE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$AE_DIR/.." && pwd)"
STAMP_DIR="$ROOT/orig/switch-stamp"      # STAMP baseline (STAMP/TM, STAMP/seq)
STAMPEDE_DIR="$ROOT/orig/stampede"       # Stampede baseline
INPUTS="$ROOT/inputs"
RESULTS="${RESULTS:-$ROOT/results}"

CONFIGS=(genome intruder kmeans_high kmeans_low labyrinth ssca2 vacation_high vacation_low yada)
STMS=(tinystm tl2 norec swisstm)

# binary name for a configuration
bin_of() { case "$1" in kmeans_*) echo kmeans ;; vacation_*) echo vacation ;; *) echo "$1" ;; esac; }

# thread-count flag for a configuration
thr_flag() { case "$1" in kmeans_*) echo -p ;; vacation_*) echo -c ;; *) echo -t ;; esac; }

# workload arguments: args_of <config> <large|small>
args_of() {
  local c=$1 size=$2
  if [[ $size == large ]]; then
    case $c in
      genome)        echo "-g16384 -s64 -n16777216" ;;
      intruder)      echo "-a10 -l128 -n262144 -s1" ;;
      kmeans_high)   echo "-m15 -n15 -t0.00001 -i $INPUTS/kmeans/random-n1048576-d32-c16.txt" ;;
      kmeans_low)    echo "-m40 -n40 -t0.00001 -i $INPUTS/kmeans/random-n1048576-d32-c16.txt" ;;
      labyrinth)     echo "-i $INPUTS/labyrinth/random-x512-y512-z7-n512.txt" ;;
      ssca2)         echo "-s20 -i1.0 -u1.0 -l3 -p3" ;;
      vacation_high) echo "-n4 -q60 -u90 -r1048576 -t4194304" ;;
      vacation_low)  echo "-n2 -q90 -u98 -r1048576 -t4194304" ;;
      yada)          echo "-a15 -i $INPUTS/yada/ttimeu1000000.2" ;;
    esac
  else
    case $c in
      genome)        echo "-g256 -s16 -n16384" ;;
      intruder)      echo "-a10 -l4 -n2038 -s1" ;;
      kmeans_high)   echo "-m15 -n15 -t0.05 -i $INPUTS/kmeans/random-n2048-d16-c16.txt" ;;
      kmeans_low)    echo "-m40 -n40 -t0.05 -i $INPUTS/kmeans/random-n2048-d16-c16.txt" ;;
      labyrinth)     echo "-i $INPUTS/labyrinth/random-x32-y32-z3-n96.txt" ;;
      ssca2)         echo "-s13 -i1.0 -u1.0 -l3 -p3" ;;
      vacation_high) echo "-n4 -q60 -u90 -r16384 -t4096" ;;
      vacation_low)  echo "-n2 -q90 -u98 -r16384 -t4096" ;;
      yada)          echo "-a20 -i $INPUTS/yada/633.2" ;;
    esac
  fi
}

# ---------------------------------------------------------------------------
# Series = one line in a figure. Each maps to <binary dir> + <run policy>.
#   pandora-<tm>            PANDORA/TM, switch on, 4 fibers/core
#   pandora-<tm>-noswitch   PANDORA/TM, switch off, 4 fibers/core (Fig. 5, S10-S12)
#   pandora-fine-lock       PANDORA/fine-lock, 1 fiber/core
#   pandora-coarse-lock     PANDORA/coarse-lock, 1 fiber/core
#   pandora-tinystm-1fpc    PANDORA/TM on TinySTM at 1 fiber/core (kmeans, Sec. 6.2: "about 14x")
#   pandora-tinystm-stats           instrumented TinySTM build, switch on  (Table 4)
#   pandora-tinystm-noswitch-stats  instrumented TinySTM build, switch off (Sec. 6.2 abort ratios)
#   stamp-<tm>              STAMP/TM (1 pthread/core)
#   stamp-tinystm-stats     STAMP/TM on TinySTM with abort counting on (Table 4)
#   stamp-seq               sequential STAMP = speedup denominator
#   stampede-tinystm        Stampede/TM (Stampede "run-tinystm")
#   stampede-htm            Stampede/HTM (Stampede "run-intelhtm")
# <tm> additionally includes "rtm" (Intel TSX) when built with --htm.
# ---------------------------------------------------------------------------
series_bin() {
  local s=$1 c=$2 b; b=$(bin_of "$c")
  case $s in
    pandora-tinystm-1fpc) echo "$ROOT/build/tinystm/$b" ;;
    pandora-*)       echo "$ROOT/build/${s#pandora-}/$b" ;;
    stamp-*-stats)   s=${s%-stats}; echo "$STAMP_DIR/build/${s#stamp-}/$b" ;;
    stamp-*)         echo "$STAMP_DIR/build/${s#stamp-}/$b" ;;
    stampede-tinystm) echo "$STAMPEDE_DIR/build-ae/run-tinystm/benchmarks/bin/${b}g" ;;
    stampede-htm)    echo "$STAMPEDE_DIR/build-ae/run-intelhtm/benchmarks/bin/${b}g" ;;
  esac
}

# fibers per core for a series (PANDORA only; others ignore it)
series_tpc() {
  case $1 in
    pandora-fine-lock|pandora-coarse-lock|pandora-tinystm-1fpc) echo 1 ;;
    pandora-*) echo 4 ;;
    *) echo 1 ;;
  esac
}

# build the command line for one run: run_cmd <series> <config> <cores> <size>
run_cmd() {
  local s=$1 c=$2 n=$3 size=$4
  [[ $s == stamp-seq ]] && n=1
  echo "$(series_bin "$s" "$c") $(thr_flag "$c")$n $(args_of "$c" "$size")"
}

# Environment for one run. As in the paper, performance runs of PANDORA and
# STAMP do NOT set STM_STATS (TinySTM then skips its statistics module); the
# "-stats" series set it to print commit/abort counts (Table 4). Stampede's own
# run script always sets STM_STATS=1 and BG_SPEC_SCRUB_CYCLE=0; we keep that.
run_env() {
  local s=$1 e=""
  case $s in
    pandora-*)  e="THR_PER_CORE=$(series_tpc "$s")" ;;
    stampede-*) e="STM_STATS=1 BG_SPEC_SCRUB_CYCLE=0" ;;
  esac
  [[ $s == *-stats ]] && e="$e STM_STATS=1"
  echo "${e# }"
}

ncores() { grep -c ^processor /proc/cpuinfo; }
has_rtm() { grep -qw rtm /proc/cpuinfo; }
