#!/bin/bash
# Build every binary used in the paper's evaluation.
#
#   ae/build_all.sh [--htm] [--only pandora|stamp|stampede]
#
#   --htm   also build the Intel TSX (RTM) variants. Compiling works on any
#           x86-64 gcc; *running* them needs a CPU with TSX enabled.
#
# Output:
#   build/<series>/...                     PANDORA (this tree)
#   orig/switch-stamp/build/<tm|seq>/...   STAMP baseline
#   orig/stampede/build-ae/run-*/...       Stampede baseline
# Build logs go to results/build-logs/.
set -euo pipefail
source "$(dirname "$0")/common.sh"

HTM=0; ONLY=""
while (( $# )); do
  case $1 in
    --htm) HTM=1 ;;
    --only) ONLY=$2; shift ;;
    -h|--help) sed -n 2,14p "$0"; exit 0 ;;
    *) echo "unknown option $1"; exit 1 ;;
  esac; shift
done
WANT() { [[ -z $ONLY || $ONLY == "$1" ]]; }

LOG="$RESULTS/build-logs"; mkdir -p "$LOG"
WL=(genome intruder kmeans labyrinth ssca2 vacation yada)
ok()   { printf '  %-34s ok\n' "$1"; }
fail() { printf '  %-34s FAILED (see %s)\n' "$1" "$2"; exit 1; }

# runtime libraries: build_rt <tree> <dir> <logname>
build_rt() {
  local tree=$1 d=$2 n=$3
  mkdir -p "$tree/$d/lib" 2>/dev/null || true      # tinySTM expects lib/ to exist
  ( cd "$tree/$d" && make clean && make ) >"$LOG/$n.log" 2>&1 || fail "$n" "$LOG/$n.log"
  ok "$n"
}

# one PANDORA variant: pandora_variant <backend> <out_dir> [ENV=VAL...]
pandora_variant() {
  local be=$1 out=$2; shift 2
  for w in "${WL[@]}"; do
    [[ $be == fine-lock && $w == yada ]] && continue   # no deadlock-free fine-grained yada (Sec. 6.1)
    env "$@" "$ROOT/scripts/sbuilder.sh" "$be" "$w" "$out" >"$LOG/pandora-$out-$w.log" 2>&1 \
      || fail "pandora $out/$w" "$LOG/pandora-$out-$w.log"
  done
  ok "pandora $out"
}

if WANT pandora || WANT stamp; then
  "$AE_DIR/build_allocator.sh" >"$LOG/allocator.log" 2>&1 || fail allocator "$LOG/allocator.log"
  ok "allocator (TCMalloc)"
fi

if WANT pandora; then
  echo "== PANDORA"
  build_rt "$ROOT" tinySTM-1.0.5 pandora-rt-tinystm
  for r in tl2 norec swisstm; do build_rt "$ROOT" $r pandora-rt-$r; done
  ( cd "$ROOT/ULT/libfibre" && make lib ) >"$LOG/pandora-rt-libfibre.log" 2>&1 \
    || fail pandora-rt-libfibre "$LOG/pandora-rt-libfibre.log"; ok pandora-rt-libfibre
  TMS=("${STMS[@]}"); (( HTM )) && TMS+=(rtm)
  for tm in "${TMS[@]}"; do
    pandora_variant "$tm" "$tm"
    pandora_variant "$tm" "$tm-noswitch" NO_SWITCH=1
  done
  pandora_variant fine-lock   fine-lock
  pandora_variant coarse-lock coarse-lock
  pandora_variant tinystm tinystm-stats          AE_CFLAGS=-DTINYSTM_STATS
  pandora_variant tinystm tinystm-noswitch-stats AE_CFLAGS=-DTINYSTM_STATS NO_SWITCH=1
fi

if WANT stamp; then
  echo "== STAMP (orig/switch-stamp)"
  build_rt "$STAMP_DIR" tinySTM-1.0.5 stamp-rt-tinystm
  for r in tl2 norec swisstm; do build_rt "$STAMP_DIR" $r stamp-rt-$r; done
  BES=(seq "${STMS[@]}"); (( HTM )) && BES+=(rtm)
  for be in "${BES[@]}"; do
    for w in "${WL[@]}"; do
      ( cd "$STAMP_DIR" && ./sbuilder.sh "$be" "$w" ) >"$LOG/stamp-$be-$w.log" 2>&1 \
        || fail "stamp $be/$w" "$LOG/stamp-$be-$w.log"
    done
    ok "stamp $be"
  done
fi

if WANT stampede; then
  echo "== Stampede (orig/stampede)"
  # Stampede ships two patched Galois trees (STM / HTM); CMakeLists.txt expects
  # ./Galois-2.2.1, so we point that link at the right one and use a separate
  # build directory per variant (no need to wipe build/ when switching).
  VARS=(tinystm); (( HTM )) && VARS+=(htm)
  for v in "${VARS[@]}"; do
    case $v in
      tinystm) galois=Galois-2.2.1_STM; target=run-tinystm ;;
      htm)     galois=Galois-2.2.1_HTM; target=run-htm ;;   # -> run-intelhtm on x86
    esac
    ln -sfn "$galois" "$STAMPEDE_DIR/Galois-2.2.1"
    d="$STAMPEDE_DIR/build-ae"; mkdir -p "$d"
    sub=$target; [[ $v == htm ]] && sub=run-intelhtm
    rm -rf "${d:?}/$sub"
    ( cd "$d" && SOURCE_DIR="$STAMPEDE_DIR" "$STAMPEDE_DIR/scripts/configure" "$target" \
        && make -C "$sub" -j"$(ncores)" ) >"$LOG/stampede-$v.log" 2>&1 \
      || fail "stampede $v" "$LOG/stampede-$v.log"
    ok "stampede $v"
  done
fi
echo "All requested builds completed."
