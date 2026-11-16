#!/bin/bash
# Reconstruct orig/stampede from upstream sources + our patches.
#
#   ae/stampede/fetch.sh [install [DEST]]  -> creates orig/stampede (or DEST) if absent
#   ae/stampede/fetch.sh base DIR   -> only the unmodified upstream base (for maintainers)
#
# Upstream (all checksum-verified; the URLs/checksums are those of Stampede's own
# scripts/download-contrib, PARSEC excluded because PANDORA does not use it):
#   Stampede      https://github.com/ddn0/stampede, commit 1ccb4e4
#   Galois 2.2.1  $GALOIS_URL (Stampede's original iss.ices.utexas.edu link has moved)
#   TinySTM 1.0.4 https://github.com/patrickmarlier/tinystm, tag v1.0.4 (tmware.org is gone)
#   STAMP 0.9.10  https://github.com/kozyraki/stamp, commit 9222e27
# then applies ae/stampede/{stampede,galois-stm,galois-htm}.diff, which are our
# changes (HTM fallback path, double-precision kmeans, build fixes).
# A Galois mirror can be given with GALOIS_URL=... .
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../.." && pwd)"

STAMPEDE_GIT=https://github.com/ddn0/stampede
STAMPEDE_COMMIT=1ccb4e4e
GALOIS_URL=${GALOIS_URL:-https://iss.oden.utexas.edu/projects/galois/downloads/Galois-2.2.1.tar.gz}
GALOIS_SHA256=d98ffcc59ab433d7ef977c3862b1c73f7a0cdc75610ce5c2df8ee5ba296d5553
TINYSTM_GIT=https://github.com/patrickmarlier/tinystm
TINYSTM_COMMIT=a3b3666c10d806c4fc90daebc81368f44e369bae   # tag v1.0.4
STAMP_GIT=https://github.com/kozyraki/stamp
STAMP_COMMIT=9222e2707e598448d90112991a3a90544469d4b4

fetch() {  # fetch <url> <sha256> <out>
  echo "  downloading $1"
  if command -v curl >/dev/null; then curl -fsSL -o "$3" "$1"; else wget -q -O "$3" "$1"; fi
  echo "$2  $3" | sha256sum -c --quiet - || { echo "checksum mismatch for $1"; exit 1; }
}

make_base() {  # make_base <dir>: upstream Stampede with contrib, as download-contrib leaves it
  for t in git patch curl sha256sum; do
    command -v $t >/dev/null || { echo "$t is required (it is in the Docker image: run this inside it)"; exit 1; }
  done
  local B=$1; rm -rf "$B"; mkdir -p "$B"; B="$(cd "$B" && pwd)"
  echo "== Stampede $STAMPEDE_COMMIT"
  git clone -q "$STAMPEDE_GIT" "$B" && git -C "$B" checkout -q "$STAMPEDE_COMMIT"
  echo "== Galois 2.2.1"
  fetch "$GALOIS_URL" "$GALOIS_SHA256" "$B/galois.tgz"
  (cd "$B" && tar xzf galois.tgz && rm galois.tgz && patch -s -p0 < patches/Galois-2.2.1.patch)
  echo "== TinySTM 1.0.4"
  git clone -q "$TINYSTM_GIT" "$B/.tinystm" && git -C "$B/.tinystm" checkout -q "$TINYSTM_COMMIT"
  (cd "$B/.tinystm" && git archive --prefix=tinySTM-1.0.4/ HEAD) | tar -C "$B" -xf -
  rm -rf "$B/.tinystm"
  (cd "$B" && patch -s -p0 < patches/tinySTM-1.0.4.patch)
  echo "== STAMP 0.9.10"
  (cd "$B/benchmarks" && git clone -q "$STAMP_GIT" stamp-0.9.10 \
     && git -C stamp-0.9.10 checkout -q "$STAMP_COMMIT" && rm -rf stamp-0.9.10/.git \
     && patch -s -p0 < "$B/patches/stamp-0.9.10.patch" \
     && for v in stamp-grun stamp-g; do
          mkdir -p $v && (cd stamp-0.9.10 && tar cf - --exclude=inputs .) | (cd $v && tar xf -)
          patch -s -p0 < "$B/patches/$v.patch"
        done)
  rm -rf "$B/.git"
}

DIFFX=(-x inputs -x .git -x 'Galois-2.2.1*' -x build -x build-ae)

case "${1:-install}" in
  base) make_base "$2" ;;
  install)
    D="${2:-$ROOT/orig/stampede}"
    [[ -e $D ]] && { echo "$D already exists"; exit 0; }
    T="$(dirname "$D")/.stampede-tmp"; make_base "$T"
    echo "== applying PANDORA's changes"
    (cd "$T" && patch -s -p1 < "$HERE/stampede.diff")
    for v in STM HTM; do
      cp -a "$T/Galois-2.2.1" "$T/Galois-2.2.1_$v"
      (cd "$T/Galois-2.2.1_$v" && patch -s -p1 < "$HERE/galois-$(echo $v | tr A-Z a-z).diff")
    done
    rm -rf "$T/Galois-2.2.1"
    while read -r kind f; do                                  # see make_patches.sh
      case $kind in empty) mkdir -p "$T/$(dirname "$f")"; : > "$T/$f" ;; exec) chmod +x "$T/$f" ;; esac
    done < "$HERE/meta.txt"
    for w in kmeans labyrinth yada; do                       # shared inputs
      rm -rf "$T/benchmarks/stamp-0.9.10/$w/inputs"
      ln -s "../../../../../inputs/$w" "$T/benchmarks/stamp-0.9.10/$w/inputs"
    done
    mv "$T" "$D"; echo "orig/stampede ready." ;;
  *) sed -n 2,16p "$0"; exit 1 ;;
esac
