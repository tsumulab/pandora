#!/bin/bash
# Maintainers only: regenerate ae/stampede/*.diff from the full orig/stampede tree.
#   ae/stampede/make_patches.sh
# Needs network access to the upstream locations listed in fetch.sh.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../.." && pwd)"
O="$ROOT/orig/stampede"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
"$HERE/fetch.sh" base "$T/a"
X=(-x inputs -x .git -x build -x build-ae -x doc -x 'Galois-2.2.1*' -x '*.o' -x '*.a' -x '*.so')   # doc: generated Doxygen HTML
( cd "$T" && ln -s "$O" b && diff -ruN "${X[@]}" a b ) > "$HERE/stampede.diff" || true
for v in STM HTM; do
  ( cd "$T" && ln -sfn "$T/a/Galois-2.2.1" ga && ln -sfn "$O/Galois-2.2.1_$v" gb \
      && diff -ruN -x build ga/ gb/ ) > "$HERE/galois-$(echo $v | tr A-Z a-z).diff" || true
done
# what diff/patch cannot carry: empty files and executable bits
( cd "$O" && find . \( -path ./build-ae -o -path ./build -o -name doc -o -name inputs -o -name .git \) -prune -o \
    -type f \( -empty -printf 'empty %P\n' -o -perm -u+x -printf 'exec %P\n' \) | grep -v '^[a-z]* Galois-2.2.1/' | sort ) \
  > "$HERE/meta.txt"
ls -l "$HERE"/*.diff "$HERE/meta.txt"
echo "== verifying: rebuilding orig/stampede from upstream + patches"
"$HERE/fetch.sh" install "$T/verify/stampede" >/dev/null
if diff -r --no-dereference "${X[@]}" "$T/verify/stampede" "$O" \
   && for v in STM HTM; do diff -r -x build -x doc "$T/verify/stampede/Galois-2.2.1_$v" "$O/Galois-2.2.1_$v" || exit 1; done; then
  echo "OK: ae/stampede/fetch.sh reproduces orig/stampede"
else
  echo "MISMATCH (differences listed above; rebuilt tree kept in $T/verify)"; trap - EXIT; exit 1
fi
