#!/bin/bash
# Create the large inputs that are not shipped verbatim.
#
#   kmeans: inputs/kmeans/random-n1048576-d32-c16.txt (511 MB) is regenerated
#           deterministically (fixed seed) by inputs/kmeans/generate3.py, a Python 3
#           port of STAMP's generate.py (Python 2) that reproduces its output byte for
#           byte. The result is checked against the SHA-256 of the file used in the paper.
#   yada:   inputs/yada/ttimeu1000000.2.* is shipped compressed (.gz, as in the
#           STAMP distribution, commit 9222e27 of github.com/kozyraki/stamp)
#           and is decompressed and checksum-verified here.
set -euo pipefail
source "$(dirname "$0")/common.sh"

KM="$INPUTS/kmeans/random-n1048576-d32-c16.txt"
KM_SHA256=2cbd87d4e6c4a7f9624e1b6545b97a192cc68aac2960ef0d88fe0c3c288e8147

if [[ ! -s $KM ]]; then
  echo "generating $KM (~20 s) ..."
  python3 "$INPUTS/kmeans/generate3.py" 1048576 32 16 > "$KM.tmp" && mv "$KM.tmp" "$KM"
fi
echo "$KM_SHA256  $KM" | sha256sum -c - || { echo "kmeans input differs from the paper's"; exit 1; }

Y="$INPUTS/yada/ttimeu1000000.2"
for ext in ele node poly; do
  [[ -s $Y.$ext ]] || { echo "decompressing $Y.$ext.gz"; gunzip -k "$Y.$ext.gz"; }
done
sha256sum -c - <<SUMS || { echo "yada input differs from the paper's"; exit 1; }
3e8befca496fb3b59ff7c858c4ca88cb769fdb102579a69008c405b5d0911292  $Y.ele
5258436e17b9618c9455f11ab5358ad326d16eafa3b3decb956d5ce62cd3aa0d  $Y.node
729e4895e408bac10ec1d8cf5447923bd40ae08652e24136982f2a9340411e9c  $Y.poly
SUMS
echo "inputs ready."
