#!/usr/bin/env python3
"""Python 3 port of STAMP's inputs/kmeans/generate.py that reproduces its Python 2 output
byte for byte (same seed and Mersenne Twister; random.choice and float printing emulated
as in Python 2). Used by ae/prepare_inputs.sh when python2 is not available."""
import random, sys

if len(sys.argv) != 4:
    print("Usage: generate3.py <rows> <dimensions> <centers>"); sys.exit(1)
numRow, numDim, numCenter = (int(a) for a in sys.argv[1:4])
random.seed(0)
centers = [[random.random() for _ in range(numDim)] for _ in range(numCenter)]
sigma = (1. / numCenter) ** 3
out = sys.stdout.write
for row in range(1, numRow + 1):
    center = centers[int(random.random() * len(centers))]   # Python 2 random.choice
    vals = ["%.12g" % (center[d] + random.gauss(0, sigma)) for d in range(numDim)]  # Python 2 str(float)
    out(str(row) + " " + " ".join(vals) + "\n")
