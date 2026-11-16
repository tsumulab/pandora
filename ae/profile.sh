#!/bin/bash
# Optional: the execution-time breakdowns of Sec. 6.2 (perf). Run on the HOST, not in
# Docker (perf must match the running kernel). Binaries built inside the Docker
# image run on an Ubuntu 20.04 host as they are (the host needs perf and TCMalloc:
#   apt install linux-tools-$(uname -r) libgoogle-perftools4 );
# on other hosts, rebuild there with ae/build_all.sh --only pandora / --only stamp.
#
#   sudo ae/profile.sh <series> <config> [cores]      (default: 40 cores)
#
# Examples (paper, TinySTM/NOrec at 40 cores):
#   sudo ae/profile.sh pandora-norec genome      # NOrec commit share under PANDORA (79.3 %)
#   sudo ae/profile.sh stamp-norec   genome      #   ... and under STAMP (12.9 %)
#   sudo ae/profile.sh pandora-tinystm ssca2     # costliest TM operations under PANDORA
#   sudo ae/profile.sh stamp-tinystm   ssca2     #   ... and under STAMP
#   sudo ae/profile.sh pandora-tinystm intruder  # rollback vs. transactional reads
#   sudo ae/profile.sh pandora-tinystm vacation_high
# The report lists the functions with the largest "self" share of samples; the
# TM runtime's begin/read/write/commit/rollback functions are the ones quoted in Sec. 6.2.
set -uo pipefail
source "$(dirname "$0")/common.sh"
s=${1:?series}; c=${2:?config}; n=${3:-40}
command -v perf >/dev/null || { echo "perf not found (Ubuntu: apt install linux-tools-\$(uname -r))"; exit 1; }
b=$(series_bin "$s" "$c")
[[ -x $b ]] || { echo "missing $b: build first, e.g. in Docker: ae/build_all.sh"; exit 1; }
out="$RESULTS/profile/$s-$c-c$n"; mkdir -p "$out"
echo "profiling: $(run_env "$s") $(run_cmd "$s" "$c" "$n" large)"
env $(run_env "$s") perf record -g -o "$out/perf.data" $(run_cmd "$s" "$c" "$n" large) > "$out/run.txt" 2>&1 \
  || { echo "perf record failed, see $out/run.txt"; exit 1; }
# two views of the same samples: "self" (time in the function itself) and
# "children" (inclusive: the function and everything it calls)
perf report -i "$out/perf.data" --no-children --sort dso,symbol -g none --stdio \
  > "$out/report-self.txt" 2> "$out/report.err" || true
perf report -i "$out/perf.data" --children --sort dso,symbol -g none --stdio \
  > "$out/report-children.txt" 2>> "$out/report.err" || true
for v in self children; do
  grep -E '^ *[0-9]+(\.[0-9]+)?%' "$out/report-$v.txt" | head -25 | sed 's/ *- *- *$//' > "$out/top-$v.txt" || true
done
if [[ -s $out/top-self.txt ]]; then
  echo "-- self (top 10)";      head -10 "$out/top-self.txt"
  echo "-- children (top 10)";  head -10 "$out/top-children.txt"
else
  echo "could not extract symbols; see $out/report-*.txt and $out/report.err"
fi
echo "full reports: $out/report-{self,children}.txt  (interactive: perf report -i $out/perf.data)"
