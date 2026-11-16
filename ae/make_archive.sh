#!/bin/bash
# Maintainers only: produce the distribution archives from a clean tree.
#
#   ae/make_archive.sh            -> ../pandora-artifact-ae.tar.gz      (complete; for HotCRP)
#   ae/make_archive.sh --public   -> ../pandora-artifact-public.tar.gz  (orig/stampede replaced
#                                    by ae/stampede/*.diff + fetch.sh; for Zenodo)
#
# Always run on Linux: the tree contains files whose names differ only in case
# (e.g. ULT/libfibre/src/libfibre/{Fibre.h,fibre.h}), which Windows tools merge.
set -euo pipefail
source "$(dirname "$0")/common.sh"
PUBLIC=0; [[ ${1:-} == --public ]] && PUBLIC=1
"$AE_DIR/clean.sh" --dist >/dev/null
name=$(basename "$ROOT"); parent=$(dirname "$ROOT")
X=(--exclude="$name/.git" --exclude="$name/results" --exclude="__pycache__")
if (( PUBLIC )); then
  for d in stampede galois-stm galois-htm; do
    [[ -s $AE_DIR/stampede/$d.diff ]] || { echo "missing ae/stampede/$d.diff: run ae/stampede/make_patches.sh"; exit 1; }
  done
  X+=(--exclude="$name/orig/stampede"); out="$parent/pandora-artifact-public.tar.gz"
else
  out="$parent/pandora-artifact-ae.tar.gz"
fi
tar --owner=0 --group=0 --sort=name -C "$parent" "${X[@]}" -czf "$out" "$name"
ls -l "$out"; sha256sum "$out"
