#!/bin/bash
# Run a set of series x configurations x core counts x repetitions.
#
#   ae/run.sh -s "<series...>" -c "<configs...>" -n "<cores...>" [-r reps] [--small] [-t timeout_s]
#
# Example:
#   ae/run.sh -s "pandora-tinystm stamp-tinystm stamp-seq" -c "genome kmeans_high" -n "1 20 40" -r 5
#
# Raw output of every run is written to
#   results/raw/<size>/<series>/<config>/c<cores>_r<rep>.txt
# and is reduced by ae/collect.py. Existing raw files are kept (re-running
# resumes where it stopped); delete results/raw to start over.
set -uo pipefail
source "$(dirname "$0")/common.sh"

SERIES=""; CFGS="${CONFIGS[*]}"; CORES=""; REPS=5; SIZE=large; TMO=1800
while (( $# )); do
  case $1 in
    -s) SERIES=$2; shift ;;
    -c) CFGS=$2; shift ;;
    -n) CORES=$2; shift ;;
    -r) REPS=$2; shift ;;
    -t) TMO=$2; shift ;;
    --small) SIZE=small ;;
    -h|--help) sed -n 2,13p "$0"; exit 0 ;;
    *) echo "unknown option $1"; exit 1 ;;
  esac; shift
done
[[ -n $SERIES && -n $CORES ]] || { sed -n 2,13p "$0"; exit 1; }

# the large kmeans/yada inputs are generated/decompressed on first use
if [[ $SIZE == large ]] && [[ " $CFGS " =~ \ (kmeans_high|kmeans_low|yada)\  ]]; then
  "$AE_DIR/prepare_inputs.sh" >/dev/null || { echo "could not prepare the large inputs (run ae/prepare_inputs.sh)"; exit 1; }
fi

total=0; done_=0; failed=0
for s in $SERIES; do for c in $CFGS; do
  [[ $s == pandora-fine-lock && $c == yada ]] && continue
  for n in $CORES; do [[ $s == stamp-seq && $n != "${CORES%% *}" ]] && continue; total=$((total+REPS)); done
done; done

for s in $SERIES; do
  for c in $CFGS; do
    [[ $s == pandora-fine-lock && $c == yada ]] && continue   # no fine-grained yada (Sec. 6.1)
    b=$(series_bin "$s" "$c")
    [[ -x $b ]] || { echo "missing binary for $s/$c: $b (run ae/build_all.sh)"; exit 1; }
    for n in $CORES; do
      # the sequential baseline does not depend on the core count: run it once
      [[ $s == stamp-seq && $n != "${CORES%% *}" ]] && continue
      dir="$RESULTS/raw/$SIZE/$s/$c"; mkdir -p "$dir"
      for r in $(seq 1 "$REPS"); do
        f="$dir/c${n}_r${r}.txt"
        done_=$((done_+1))
        if [[ -s $f ]] && grep -q '^AE-EXIT 0$' "$f"; then continue; fi
        printf '[%d/%d] %s %s cores=%s rep=%s\n' "$done_" "$total" "$s" "$c" "$n" "$r"
        {
          echo "AE-SERIES $s"; echo "AE-CONFIG $c"; echo "AE-CORES $n"; echo "AE-REP $r"
          echo "AE-HOST $(hostname)"; echo "AE-CMD $(run_env "$s") $(run_cmd "$s" "$c" "$n" "$SIZE")"
          env $(run_env "$s") timeout "$TMO" $(run_cmd "$s" "$c" "$n" "$SIZE") 2>&1
          echo "AE-EXIT $?"
        } > "$f"
        grep -q '^AE-EXIT 0$' "$f" || { echo "   -> FAILED (see $f)"; failed=$((failed+1)); }
      done
    done
  done
done
echo "finished: $total runs, $failed failed"
(( failed == 0 ))
