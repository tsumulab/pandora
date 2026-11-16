#!/bin/bash
# Kick-the-tires check (~10 minutes including the build).
#
#   ae/kick_the_tires.sh
#
# Builds everything except the HTM variants, then runs all nine configurations
# with the *small* inputs on 1 and 2 cores for PANDORA (TinySTM, fine-grained
# lock), STAMP (TinySTM, sequential) and Stampede (TinySTM), and checks the
# programs' built-in self-validation. Success = the last line says "PASS".
set -uo pipefail
source "$(dirname "$0")/common.sh"
export RESULTS="$ROOT/results/kick-the-tires"; rm -rf "$RESULTS"
# Fill fresh malloc() memory with garbage so that reads of uninitialized memory
# fail here deterministically instead of depending on the machine.
export MALLOC_PERTURB_=${MALLOC_PERTURB_:-165}

"$AE_DIR/build_all.sh" || { echo "FAIL: build"; exit 1; }
"$AE_DIR/run.sh" --small -r 1 -n "1 2" -t 120 \
   -s "pandora-tinystm pandora-fine-lock stamp-tinystm stamp-seq stampede-tinystm" || true
python3 "$AE_DIR/collect.py" --results "$RESULTS"

python3 - "$RESULTS/runs.csv" <<'PY'
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1])))
bad = [r for r in rows if r["exit"] != "0" or r["valid"] == "no" or not r["time"]]
want = 5 * 9 * 2 - 2 - 9      # minus fine-lock/yada (x2 cores), stamp-seq runs once per config
print(f"{len(rows)} runs (expected {want}), {len(bad)} failed")
print("PASS" if not bad and len(rows) == want else "FAIL")
PY
