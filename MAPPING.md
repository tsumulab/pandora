# MAPPING -- paper <-> artifact

Paper: *"Measuring Transactional Memory: PANDORA and the Division of Labor"* (PPoPP 2027).
For how to run things, see `README.md`; this file says which experiment produces which
figure or table, and where the paper's own numbers are.

## 1. Measurement environment (paper Sec. 6.1, Tables 2-3)

| Item | Paper | Artifact |
|---|---|---|
| CPU | Intel Xeon Platinum 8380 (Ice Lake), 1 socket, 40 cores, 2.30 GHz; HT off, Turbo off | your machine |
| Memory / caches | 64 GB; L1D 1.9 MiB / L2 50 MiB / L3 60 MiB | -- |
| OS / compiler | Ubuntu 20.04, gcc/g++ 9.4.0, `-O2` | `docker/Dockerfile` (same) |
| Allocator | TCMalloc | same (`common/env.mk`) |
| Threads | PANDORA TM: 4 fibers/core (libfibre); PANDORA locks: 1 fiber/core; STAMP and Stampede: 1 pthread/core | same (`ae/common.sh`: `series_tpc`) |
| Repetitions | mean of 5 runs; cores 1, 2, 4, 8, 12, ..., 40 | default 3 runs at 1, N/2, N; `REPS`, `CORES` to change |
| Speedup | over sequential STAMP on the same input | same (`ae/collect.py`) |

**libfibre provenance.** libfibre ("Fred", M. Karsten, U. Waterloo) is bundled under
`ULT/libfibre/` from `https://git.uwaterloo.ca/mkarsten/libfibre.git`, commit
`8a4ef356b0828fe2cca504a5d6661814cd3d0dc8` (upstream inactive since 2023-06). Local changes:
two disabled build-time options in `testoptions.h.default`, and a fix in
`src/runtime/Stack.S` that makes a new fibre's stack follow the x86-64 ABI alignment
(`diffs/libfibre.diff`).

**Baselines.** `orig/switch-stamp/` is STAMP 0.9.10 on the same build harness (allocator,
TM back-ends) as PANDORA, configured as in the paper for STAMP (pthreads, 1 thread/core,
no switching); it produces STAMP/TM and the sequential STAMP denominator.
`orig/stampede/` is Stampede (github.com/ddn0/stampede) with its bundled, patched Galois
2.2.1 and TinySTM 1.0.4; Stampede/TM is its `run-tinystm` build and the HTM panel uses its
`run-intelhtm` build.

## 2. Systems under test

| Paper name | Series (`ae/`) | Backend dir | Notes (Table 1) |
|---|---|---|---|
| TL2 | `pandora-tl2`, `stamp-tl2` | `backends/tl2` | lazy CD, per-location version locks + global clock |
| TinySTM | `pandora-tinystm`, `stamp-tinystm`, `stampede-tinystm` | `backends/tinystm` | eager CD, per-location version locks + global clock |
| NOrec | `pandora-norec`, `stamp-norec` | `backends/norec` | lazy CD, single global sequence lock |
| SwissTM | `pandora-swisstm`, `stamp-swisstm` | `backends/swisstm` | eager W/W, lazy R/W, per-location + commit timestamp |
| HTM (Intel TSX) | `pandora-rtm`, `stamp-rtm`, `stampede-htm` | `backends/rtm` | needs a CPU with TSX enabled |
| fine-/coarse-grained locks | `pandora-fine-lock`, `pandora-coarse-lock` | `backends/{fine,coarse}-lock` | yada has no fine-grained version (Sec. 6.1) |

`backends/powerhtm` (IBM POWER) and `bayes` are present but not part of the paper's
evaluation (Sec. 6.4; Sec. 5 footnote 1).

## 3. Figures and tables

| Item | Content | Experiment | Paper's numbers |
|---|---|---|---|
| Table 1 | STM design properties | descriptive (`backends/*/tm.h`, runtime sources) | -- |
| Tables 2-3 | platform / execution environment | Sec. 1 above | -- |
| Table 4 | #Tx, \|R\|/\|W\|, Tx time, aborts STAMP->PANDORA (TinySTM @ 40 cores) | E3 | `data/table4.csv` |
| Table 4 | LOC ratio (fine/TM) | E4 | `data/loc.csv` |
| Fig. 1 | division-of-labor diagram | conceptual | -- |
| Figs. 2-4 | speedup: genome, kmeans_high, ssca2 | E1 | `data/speedup.csv` (`S1(=Fig2)`, `S2(=Fig3)`, `S4(=Fig4)`) |
| Fig. 5 | switch vs. non-switch, TinySTM, kmeans_high & ssca2 | E2 | `data/speedup.csv` (`switch=off` rows and the fine-lock curve) |
| Figs. S1-S9 | speedup, all nine configurations x all TM systems | E1-full | `data/speedup.csv` |
| Figs. S10-S11 | switch/non-switch, all TM systems, kmeans_high / ssca2 | E2-full | `data/speedup.csv` |
| Fig. S12 | switch/non-switch, TinySTM, all configurations | E2-full | `data/speedup.csv` |

`ae/paper_figures.py` draws Figs. 2-5 and S1-S12 in the paper's layout, from
`data/speedup.csv` (`--paper-csv`) or from your `results/summary.csv`.
`ae/plot.py` draws each measured panel together with the paper's curve from
`data/speedup.csv`. See `data/README.md` for that file's schema.

## 4. Source diffs

`diffs/` holds unified diffs from pristine STAMP 0.9.10 (and TinySTM 1.0.5, libfibre) to
the versions shipped here; see `diffs/README.md`.

| File | Shows | Paper |
|---|---|---|
| `diffs/{genome,intruder,kmeans,labyrinth,ssca2,vacation,yada}.diff` | per-workload redesign | Sec. 5.2, appendix Sec. A |
| `diffs/lib.diff` | shared harness: threads, libfibre integration, TM macros, Table 4 counters | Sec. 5.1 |
| `diffs/tinystm.diff` | TinySTM build configuration only (statistics, POSIX TLS, FORTIFY off) | Table 4 |
| `diffs/libfibre.diff` | two libfibre build options | Sec. 5.1 |
