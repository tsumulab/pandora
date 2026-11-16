#!/bin/bash
# Experiments backing the paper's claims (see README, "Step-by-step").
#
#   ae/experiments.sh <E1|E1-full|E2|E2-full|E3|E4|E5|all>   (E5: host only, root)
#
# Core counts default to "1, half, all" of this machine (capped at 40, the
# paper's maximum); override with  CORES="1 2 4 8 ... 40". Repetitions default
# to 3 (paper: mean of 5); override with REPS=5. Each experiment ends by
# running ae/collect.py and ae/plot.py, so results/summary.csv and
# results/figures/ are always up to date. Re-running an experiment resumes.
set -uo pipefail
source "$(dirname "$0")/common.sh"

N=$(ncores); (( N > 40 )) && N=40
CORES="${CORES:-$(echo 1 $((N/2)) $N | tr ' ' '\n' | awk '$1>0' | sort -nu | tr '\n' ' ')}"
MAX=$(echo $CORES | awk '{print $NF}')
REPS="${REPS:-3}"
HTM_SERIES=""; has_rtm && [[ -x $(series_bin pandora-rtm genome) ]] && HTM_SERIES="pandora-rtm stamp-rtm stampede-htm"

run() { "$AE_DIR/run.sh" -r "$REPS" "$@"; }
finish() { python3 "$AE_DIR/collect.py" && python3 "$AE_DIR/plot.py"; }

E1() {  # Figures 2-4 (genome, kmeans_high, ssca2), TinySTM panel
  run -c "genome kmeans_high ssca2" -n "$CORES" \
      -s "stamp-seq pandora-tinystm pandora-fine-lock pandora-coarse-lock stamp-tinystm stampede-tinystm"
}
E1_full() {  # Figures S1-S9: all nine configurations, all TM systems
  run -n "$CORES" -s "stamp-seq pandora-fine-lock pandora-coarse-lock stampede-tinystm \
      $(for t in "${STMS[@]}"; do echo pandora-$t stamp-$t; done) $HTM_SERIES"
}
E2() {  # Figure 5: switch vs non-switch, TinySTM, kmeans_high and ssca2;
        # plus kmeans_high at 1 fiber/core (Sec. 6.2, "about 14x")
  run -c "kmeans_high ssca2" -n "$CORES" \
      -s "stamp-seq pandora-tinystm pandora-tinystm-noswitch pandora-fine-lock"
  run -c "kmeans_high" -n "$MAX" -s "pandora-tinystm-1fpc"
}
E2_full() {  # Figures S10-S12
  run -c "kmeans_high ssca2" -n "$CORES" \
      -s "stamp-seq $(for t in "${STMS[@]}"; do echo pandora-$t pandora-$t-noswitch; done)"
  run -n "$CORES" -s "stamp-seq pandora-tinystm pandora-tinystm-noswitch"
}
E3() {  # Table 4 (#Tx, |R|/|W|, Tx time, aborts STAMP -> PANDORA) and the abort
        # ratios of Sec. 6.2 (switch on/off; Stampede on kmeans), TinySTM at max cores
  run -n "$MAX" -s "pandora-tinystm-stats pandora-tinystm-noswitch-stats stamp-tinystm-stats"
  run -n "$MAX" -c "kmeans_high" -s "stampede-tinystm"
}
E4() {  # Table 4, LOC ratio (fine/TM): deterministic, no measurement
  "$ROOT/scripts/run_all_cloc.sh"
}
E5() {  # Sec. 6.2 profiling breakdowns (perf, on the host as root; see ae/profile.sh)
  [[ -f /.dockerenv ]] && { echo "E5 needs perf on the host; run it outside Docker (sudo ae/experiments.sh E5)"; exit 1; }
  "$AE_DIR/prepare_inputs.sh" >/dev/null
  for sc in "pandora-norec genome" "stamp-norec genome" \
            "pandora-tinystm ssca2" "stamp-tinystm ssca2" \
            "pandora-tinystm intruder" "pandora-tinystm vacation_high"; do
    set -- $sc
    echo "=================== $1 $2 ($MAX cores)"
    "$AE_DIR/profile.sh" "$1" "$2" "$MAX"
  done
  echo "per profile: results/profile/*/top-{self,children}.txt"
}

if [[ -n ${1:-} && ${1:-} != E4 && ${1:-} != E5 ]]; then
  "$AE_DIR/prepare_inputs.sh" >/dev/null || exit 1
  # build on first use (e.g. when kick_the_tires.sh has not been run)
  [[ -x $(series_bin pandora-tinystm genome) && -x $(series_bin stamp-seq genome) \
     && -x $(series_bin stampede-tinystm genome) ]] || "$AE_DIR/build_all.sh" || exit 1
fi

case "${1:-}" in
  E1) E1; finish ;;
  E1-full) E1_full; finish ;;
  E2) E2; finish ;;
  E2-full) E2_full; finish ;;
  E3) E3; finish ;;
  E4) E4 ;;
  E5) E5 ;;
  all) E1_full; E2_full; E3; finish; E4 ;;
  *) sed -n 2,11p "$0"; exit 1 ;;
esac
