#!/bin/bash
# Remove what the AE scripts generated.
#
#   ae/clean.sh            build products only (all binaries, runtime libraries, build dirs)
#   ae/clean.sh --dist     also results/, generated large inputs and LOC logs:
#                          returns the tree to its shipped state ("make distclean")
set -uo pipefail
source "$(dirname "$0")/common.sh"
DIST=0; [[ ${1:-} == --dist ]] && DIST=1

say() { echo "  - $*"; }
mk_clean() { [[ -f $1/Makefile ]] && ( cd "$1" && make clean ) >/dev/null 2>&1; }

echo "cleaning build products"
rm -rf "$ROOT/build" "$STAMP_DIR/build" && say "build/, orig/switch-stamp/build/"
rm -rf "$ROOT/allocator/lib" "$ROOT/allocator/build" && say "allocator/lib"
rm -rf "$ROOT/ae/__pycache__"
for t in "$ROOT" "$STAMP_DIR"; do
  for r in tinySTM-1.0.5 tl2 norec swisstm; do mk_clean "$t/$r"; done
  mk_clean "$t/ULT/libfibre"
  rmdir "$t/tl2/lib" "$t/norec/lib" 2>/dev/null
  find "$t/lib" "$t/ULT/fibre" -maxdepth 1 \( -name '*.o' -o -name '*.d' \) -delete 2>/dev/null
  rm -f "$t/lib/tm.h" "$t/common/Makefile"
done
find "$ROOT" "$STAMP_DIR" -path "$STAMPEDE_DIR" -prune -o \
     \( -name '*.o' -o -name '*.d' -o -name '*.a' -o -name '*.so' -o -name '*.so.*' \) -type f -print0 \
     | xargs -0 rm -f
say "runtime libraries and object files"
for d in swisstm "$STAMP_DIR/swisstm"; do      # SwissTM's test programs
  ( cd "$ROOT" && rm -f "$d/target/obj/intset-rbtree" "$d/target/obj/rbforest" "$d/intset-rbtree" "$d/rbforest" )
done
if [[ -d $STAMPEDE_DIR ]]; then
  rm -rf "$STAMPEDE_DIR/build-ae" "$STAMPEDE_DIR/Galois-2.2.1" && say "orig/stampede/build-ae, Galois-2.2.1 link"
fi

if (( DIST )); then
  echo "cleaning results and generated inputs"
  rm -rf "$ROOT/results" && say "results/"
  rm -f "$INPUTS/kmeans/random-n1048576-d32-c16.txt" "$INPUTS"/yada/ttimeu1000000.2.{ele,node,poly}
  say "generated kmeans/yada inputs (the .gz files are kept)"
  rm -rf "$ROOT/Logs/cloc_results" "$ROOT/Logs/cloc_summary_logs" && say "Logs/cloc_*"
  find "$ROOT/Logs" -mindepth 1 -maxdepth 1 -type d -name '[0-9]*' -exec rm -rf {} + 2>/dev/null
fi
echo "done."
