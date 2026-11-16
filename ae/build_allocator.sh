#!/bin/bash
# Build the TCMalloc used by the paper: gperftools 2.13 (shipped tarball) with one change,
# allocator/tcmalloc-batch8.patch (8x the batch of objects moved between a thread cache and
# the central free lists). Result: allocator/lib/libtcmalloc.so.4, picked up at run time by
# every PANDORA and STAMP binary (RUNPATH, see common/env.mk). Idempotent.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; A="$ROOT/allocator"
[[ -e $A/lib/libtcmalloc.so.4 ]] && { echo "allocator: already built ($A/lib)"; exit 0; }
echo "4882c5ece69f8691e51ffd6486df7d79dbf43b0c909d84d3c0883e30d27323e7  $A/gperftools-2.13.tar.gz" | sha256sum -c --quiet
B="$A/build"; rm -rf "$B"; mkdir -p "$B"
tar xzf "$A/gperftools-2.13.tar.gz" -C "$B"
( cd "$B" && patch -p0 < "$A/tcmalloc-batch8.patch" )
( cd "$B/gperftools-2.13" && ./configure --disable-static >"$B/configure.log" 2>&1 \
  && make -j"$(nproc)" libtcmalloc.la >"$B/make.log" 2>&1 ) || { echo "allocator build failed (logs in $B)"; exit 1; }
mkdir -p "$A/lib"; cp -P "$B"/gperftools-2.13/.libs/libtcmalloc.so* "$A/lib/"
rm -rf "$B"; echo "allocator: built $A/lib/libtcmalloc.so.4"
