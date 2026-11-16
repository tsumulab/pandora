# STAMP baseline (STAMP/TM, sequential STAMP)

STAMP 0.9.10 on the same build harness as PANDORA (TM back-ends, allocator), configured as
in the paper for STAMP: pthreads, 1 thread per core, no transaction switching
(`common/env.mk`). It provides the STAMP/TM curves and the sequential STAMP time used as
the speedup denominator of every figure.

**Use the top-level `ae/` scripts** (`ae/build_all.sh`, `ae/run.sh -s stamp-<tm>|stamp-seq`).
The scripts in this directory (`sbuilder.sh`, `run.sh`, ...) are the ones used during the
project and still work; `inputs/` is a link to the top-level `inputs/`.
`README.orig-ja.md` is the original (Japanese) developer note.
