#!/bin/bash
# Run the long experiments back to back, unattended, and record how long each took.
#
#   ae/run_all.sh [experiment ...]        default: E1 E1-full E2 E2-full E3 E4
#
# Meant for an overnight run, detached from the terminal, e.g.
#   docker run -d --name pandora-ae-all -v "$PWD":/pandora pandora-ae ae/run_all.sh
#   docker logs -f pandora-ae-all          # follow progress (Ctrl-C stops following only)
# Timings go to results/timings.csv and the full log to results/run_all.log.
# Already finished runs are skipped, so the script can simply be restarted.
set -uo pipefail
source "$(dirname "$0")/common.sh"
EXPS=("$@"); (( ${#EXPS[@]} )) || EXPS=(E1 E1-full E2 E2-full E3 E4)
mkdir -p "$RESULTS"; LOG="$RESULTS/run_all.log"; T="$RESULTS/timings.csv"
[[ -s $T ]] || echo "experiment,start,end,seconds,exit,cores,reps" > "$T"
exec > >(tee -a "$LOG") 2>&1
echo "=== run_all.sh on $(hostname), $(ncores) cores, $(date -Is): ${EXPS[*]}"
for e in "${EXPS[@]}"; do
  s=$(date +%s); echo "=== $e started $(date -Is)"
  "$AE_DIR/experiments.sh" "$e"; rc=$?
  t=$(date +%s)
  echo "$e,$(date -d @$s -Is),$(date -d @$t -Is),$((t-s)),$rc,\"${CORES:-default}\",${REPS:-3}" >> "$T"
  printf '=== %s finished in %dh%02dm (exit %d)\n' "$e" $(((t-s)/3600)) $(((t-s)%3600/60)) "$rc"
done
echo "=== all done $(date -Is); timings in $T"
