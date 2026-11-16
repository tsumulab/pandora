# PANDORA

A transactional-memory benchmark suite, and the artifact for the PPoPP 2027 paper
*"Measuring Transactional Memory: PANDORA and the Division of Labor."*

PANDORA is a STAMP-derived benchmark suite whose workloads were redesigned so that
conflict detection, version management, and transaction scheduling are left to the
TM runtime rather than to the application. This artifact contains PANDORA, the two
baselines it is compared with (STAMP and Stampede), every TM runtime and lock
version used in the paper, and scripts that rebuild every binary, rerun every
measurement, and redraw every speedup figure next to the paper's own curves.

**Versions.** This repository is where PANDORA continues to be developed. The version
evaluated in the paper's artifact evaluation is archived at Zenodo:
https://doi.org/10.5281/zenodo.23908117. Like that archive, this repository does not contain
the Stampede/Galois baseline, whose license does not permit redistribution; it ships our
patches instead, and `ae/stampede/fetch.sh` downloads the upstream sources and applies them.

**Badges requested:** Artifacts Evaluated -- Reusable, Results Reproduced, Artifacts Available.

**Paper version.** The PDF submitted with this artifact is the camera-ready draft. Changes
relevant to the artifact since submission (details in Sec. 5):
- genome: a missing barrier was restored; all genome numbers were re-measured.
- TinySTM: every TinySTM number (PANDORA with and without switching, STAMP/TM, Table 4,
  the abort counts and profiles of Sec. 6.2) was re-measured with the stock TinySTM 1.0.5
  shipped here; the submitted version had used a build with extra global counters.
- The TCMalloc used throughout (gperftools 2.13 with one tuning change) is now stated in
  Table 3 and shipped in `allocator/`.
The conclusions did not change. `data/` holds the numbers of the camera-ready draft.

| | |
|---|---|
| Getting Started | ~10 min wall clock on our 40-core machine: ~4 min `docker build`, ~5 min build + smoke test (Sec. 2) |
| Key experiments E1, E2, E3, E4 | ~1.5 h on a 40-core machine (Sec. 3) |
| All experiments (`ae/run_all.sh`) | ~14 h on a 40-core machine, mostly E1-full; suited to an overnight run |
| Hardware | x86-64 Linux; the paper's results use 40 cores, >= 8 cores recommended; Intel TSX only for the HTM results (Sec. 4) |
| Software | Docker (or Podman/Apptainer) on a native x86-64 Linux host; network access for `docker build`. Everything else is inside the image. Also runs without containers on Ubuntu 20.04-24.04 (Sec. 2). Not supported: Docker Desktop on macOS or Windows |
| Disk | ~3 GB including the image |

---

## 1. What is in the archive

```
README.md            this file
docker/Dockerfile    build/run environment (Ubuntu 20.04, gcc 9.4 -- the paper's toolchain)
ae/                  artifact-evaluation scripts (start here)
  kick_the_tires.sh    build + small-input smoke test of all nine configurations
  experiments.sh       E1-E5 (Sec. 3)
  run_all.sh           E1 ... E4 back to back, unattended, with timings (Sec. 3)
  build_all.sh         build every binary of every series (~5 min)
  run.sh               run any series x configuration x core count x repetitions
  collect.py           raw logs -> results/runs.csv, results/summary.csv
  plot.py              summary -> results/figures/*.png, overlaid on the paper's curves
  paper_figures.py     the paper's figures, in the paper's layout, from data/ or from your runs
  prepare_inputs.sh    create/verify the large inputs (done automatically on first use)
  clean.sh             remove build products (--dist: back to the shipped state)
  profile.sh           optional perf profiles of Sec. 6.2 (host only)
  common.sh            single source of truth: workload arguments and series definitions
  stampede/            fetch.sh: rebuild orig/stampede from upstream + our patches
<workload>/          PANDORA workloads: genome intruder kmeans labyrinth ssca2 vacation yada
lib/ common/         shared harness (threads, libfibre integration, TM macro layer)
backends/            per-backend tm.h + Makefile (TM runtimes, locks, sequential)
tinySTM-1.0.5/ tl2/ norec/ swisstm/   STM runtimes
ULT/libfibre/        user-level threads (libfibre, GPLv3)
allocator/           the TCMalloc used by PANDORA and STAMP: gperftools 2.13 + one-line patch
orig/switch-stamp/   STAMP baseline: STAMP/TM and sequential STAMP (the speedup denominator)
orig/stampede/       Stampede baseline (Nguyen & Pingali, ASPLOS'17): Stampede/TM
inputs/              all inputs, shared by PANDORA, STAMP and Stampede
scripts/             original build/run/LOC scripts (used by ae/)
data/                the exact numbers plotted in the paper (speedup.csv, loc.csv, ...)
diffs/               STAMP -> PANDORA source diffs, per workload (paper Sec. 5, appendix Sec. A)
MAPPING.md           paper figure/table -> experiment -> output file
NOTICE               licensing index
```

---

## 2. Getting Started (kick the tires)

**Host.** Run on a native x86-64 Linux host and unpack the archive there: the tree contains
file names that differ only in case (e.g. `ULT/libfibre/src/libfibre/{Fibre.h,fibre.h}`),
which a case-insensitive file system (the default on macOS and Windows) silently merges, and
Docker Desktop's virtual machine (or x86 emulation on Arm Macs) distorts the timings the
claims are about. The container uses the host kernel; any recent kernel works. For the HTM
results the host CPU and kernel must also expose Intel TSX (`rtm` in `/proc/cpuinfo`).

```bash
# 0. unpack on a Linux host, then from the archive root (~4 min; downloads Ubuntu 20.04 packages):
docker build -t pandora-ae docker/

# 1. start a container with the archive mounted at /pandora, running as yourself so
#    that everything it writes into the archive stays yours
docker run --rm -it --user "$(id -u):$(id -g)" -e HOME=/tmp -v "$PWD":/pandora pandora-ae bash

# 2. inside the container, in /pandora: build everything and run all nine
#    configurations on small inputs (~5 min)
ae/kick_the_tires.sh
```

Expected: the build prints `ok` for every component, and the output ends with

```
79 runs (expected 79), 0 failed
PASS
```

`kick_the_tires.sh` runs PANDORA (TinySTM and fine-grained locks), STAMP (TinySTM and
sequential) and Stampede (TinySTM) on 1 and 2 cores and checks the programs' own
self-validation (e.g. `Sequence matches gene: yes`, `Final mesh is valid.`). The small
inputs finish in milliseconds, so the timings it reports are meaningless.

**Docker permissions.** Talking to the Docker daemon needs root or membership in the
`docker` group; prefix the `docker` commands with `sudo` if needed. Keep the `--user` option
above either way: without it the container runs as root and the files it writes (`build/`,
`results/`, ...) belong to root.

**Without Docker.**
- *Podman* (rootless, common on shared servers): replace `docker` by `podman` in the commands
  above; with rootless Podman, use `--userns=keep-id` instead of `--user ...`.
- *Apptainer/Singularity*: build the image with Docker or Podman elsewhere, then
  `docker save pandora-ae -o pandora-ae.tar`, `apptainer build pandora-ae.sif docker-archive://pandora-ae.tar`,
  and `apptainer exec --bind "$PWD":/pandora --pwd /pandora pandora-ae.sif ae/kick_the_tires.sh`.
- *Natively*: install the packages of `docker/Dockerfile`, e.g. on Ubuntu
  `sudo apt-get install build-essential cmake libboost-dev libnuma-dev unifdef cloc python3-matplotlib fonts-liberation bc gdb git curl patch`,
  then run the same `ae/` commands from the archive root on the host. We checked this on
  Ubuntu 24.04 with gcc 13.3: every binary builds, and `kick_the_tires.sh` and a small-input
  run of every series pass. The paper's numbers were measured with gcc 9.4 (the image);
  another compiler can shift absolute timings, not the claims' shape.

---

## 3. Step-by-step instructions

All commands run inside the container from `/pandora`. Each experiment writes raw logs
under `results/raw/`, then refreshes `results/summary.csv` and `results/figures/`.
Experiments can be interrupted and re-run; finished runs are kept.
The large kmeans and yada inputs are generated and checksum-verified on first use (~1 min).

By default every experiment uses **core counts 1, N/2 and N** (N = cores of the machine,
capped at 40, the paper's maximum) and **3 repetitions** (the paper uses the mean of at
least 5 runs at 1, 2, 4, 8, 12, ..., 40 cores). Both can be changed:

```bash
CORES="1 2 4 8 12 16 20 24 28 32 36 40" REPS=5 ae/experiments.sh E1   # the paper's sweep
```

| Exp. | Command | Paper | Time (40 cores, defaults) |
|---|---|---|---|
| E1 | `ae/experiments.sh E1` | Figs. 2-4 (genome, kmeans_high, ssca2), TinySTM panel | 53 min |
| E1-full | `ae/experiments.sh E1-full` | Figs. S1-S9 (9 configurations x all TM systems) | 11 h 16 min |
| E2 | `ae/experiments.sh E2` | Fig. 5 (switch vs. non-switch, TinySTM); kmeans at 1 fiber/core | 7 min after E1 (reuses its runs) |
| E2-full | `ae/experiments.sh E2-full` | Figs. S10-S12 | 55 min after E1-full |
| E3 | `ae/experiments.sh E3` | Table 4 (#Tx, \|R\|/\|W\|, Tx time, aborts); abort ratios of Sec. 6.2 | 25 min |
| E4 | `ae/experiments.sh E4` | Table 4, LOC ratio (fine-lock/TM) | < 1 min |
| E5 (optional) | `sudo ae/experiments.sh E5` on the host (not in Docker) | Sec. 6.2 profiling breakdowns (perf) | 1 min (6 profiles; after a build) |

**Where to look.**

- `results/figures/large_<config>_<TM>.png`: this machine's curves (solid) drawn over
  the curves of the paper (dashed, from `data/speedup.csv`), one file per panel.
- `results/paper-figures/`: the paper's figures (Figs. 2-5, S1-S12) in the paper's layout,
  drawn from this machine's results by `python3 ae/paper_figures.py` (after `ae/collect.py`).
  `python3 ae/paper_figures.py --paper-csv data/speedup.csv` draws the same figures from the
  values plotted in the paper, into `results/paper-figures-data/`, for a side-by-side look.
  File names: `speedup_<config>` (Figs. 2-4, S1-S9), `switch_tinystm` (Fig. 5),
  `switch_<config>_all_tm` (S10, S11), `switch_tinystm_all` (S12); `--name-by-figure` prefixes
  the figure number. Panels without data (e.g. HTM without TSX) are left empty.
- `results/summary.csv`: one row per (series, configuration, core count) with mean and
  standard deviation of the execution time, the speedup over sequential STAMP, mean abort
  and commit counts and their ratio, and -- for the `-stats` series of E3 -- mean read/write-set
  sizes. Table 4's columns map as follows: #Tx = `commits_mean`, |R| / |W| =
  `avg_read_set` / `avg_write_set` of `pandora-tinystm-stats`; Aborts (STAMP -> PANDORA) =
  `aborts_mean` of `stamp-tinystm-stats` -> `pandora-tinystm-stats`. Tx time is printed per
  run as `Total Tx Time(second)` (column `tx_time_s` of `results/runs.csv`); divide it by
  cores x execution time for the fraction reported in Table 4.
- E4 prints the LOC table; it is deterministic and must equal `data/loc.csv`.

**Series names** (see `ae/common.sh`):

| Series | Meaning in the paper |
|---|---|
| `pandora-<tm>` | PANDORA/TM, `<tm>` in tinystm, tl2, norec, swisstm, rtm; 4 fibers/core, switching on |
| `pandora-<tm>-noswitch` | same, switching off (Fig. 5, S10-S12) |
| `pandora-tinystm-1fpc` | PANDORA/TM on TinySTM at 1 fiber/core (Sec. 6.2) |
| `pandora-fine-lock`, `pandora-coarse-lock` | PANDORA lock versions, 1 fiber/core |
| `pandora-tinystm[-noswitch]-stats` | instrumented TinySTM build (Table 4, Sec. 6.2 abort counts) |
| `stamp-<tm>`, `stamp-seq` | STAMP/TM; sequential STAMP = speedup denominator |
| `stamp-tinystm-stats` | STAMP/TM on TinySTM with abort counting on (Table 4) |
| `stampede-tinystm`, `stampede-htm` | Stampede/TM (Stampede's `run-tinystm` / `run-intelhtm` builds) |

**Unattended run of everything** (e.g. overnight; finished runs are skipped on restart):

```bash
docker run -d --name pandora-ae-all --user "$(id -u):$(id -g)" -e HOME=/tmp -v "$PWD":/pandora pandora-ae ae/run_all.sh
docker logs -f pandora-ae-all      # progress; results/timings.csv has the time per experiment
```

Any subset can be run directly, e.g.
`ae/run.sh -s "pandora-norec stamp-norec stamp-seq" -c "kmeans_high" -n "1 8 16 40" -r 5`
followed by `python3 ae/collect.py && python3 ae/plot.py`.

---

## 4. Claims

Section and table numbers refer to the camera-ready paper. Absolute speedups depend on
the evaluation machine (ours: single-socket Xeon Platinum 8380, 40 cores, HT and Turbo
off; Table 2); what should be reproduced is the shape and ordering of the curves and the
order of magnitude of the counts. On a machine with fewer cores, compare at its maximum.

**Supported by the artifact.**

- **C1 -- PANDORA measures runtime costs that STAMP's contention hides (genome; Sec. 6.2, Fig. 2, Table 4).**
  On TinySTM at 40 cores, aborts fall from 199,710 under STAMP to 8,179 with the redesign
  alone (switching off) and to 2,903 with switching. Under PANDORA, genome scales on most
  runtimes while NOrec falls behind STAMP beyond 16 cores. *E1, E1-full (NOrec panel), E3.*
- **C2 -- PANDORA tells runtimes apart (kmeans; Sec. 6.2, Fig. 3, Table 4).**
  On TinySTM at 40 cores, aborts fall from about 11.8 billion (STAMP) and ~2.2 billion (Stampede)
  to 481,910 (PANDORA); per committed transaction 92 / 0.92 / 0.026. Four runtimes scale
  toward 30x while NOrec peaks near 16 cores and then declines. *E1, E1-full, E3.*
- **C3 -- Runtime-side switching pays off where contention is high and not where it is
  low (Sec. 6.2, Fig. 5, S10-S12).** kmeans_high on TinySTM at 40 cores: ~31x with switching,
  ~12x without, ~14x at one fiber per core. ssca2: the non-switch configuration is about
  3.5% faster. *E2, E2-full, E3.*
- **C4 -- PANDORA reports where its own redesign does not help (ssca2; Sec. 6.2, Fig. 4).**
  Aborts rise from 7,903 (STAMP) to 107,916 (PANDORA) as transaction boundaries grow; on the
  STMs PANDORA/TM approaches the fine-grained lock except on NOrec, TinySTM leading. *E1, E1-full, E3.*
- **C5 -- TM and locks on equal terms (Sec. 6.1, Sec. 6.3, Figs. S1-S9).** On most workloads and
  runtimes PANDORA/TM exceeds the coarse-grained lock and approaches the fine-grained lock;
  kmeans_low is the exception for coarse locking; intruder and vacation keep measurable
  gaps. *E1-full.*
- **C6 -- Productivity-matched locks (Table 4, LOC ratio).** Fine-grained lock versions are
  0.98-1.12x the TM versions' concurrency-relevant code; yada has none. *E4, exact.*
- **C7 -- Where the time goes (Sec. 6.2, perf profiles at 40 cores).** NOrec spends most of its
  time committing under PANDORA on genome (79.3%) but not under STAMP (12.9%); on ssca2
  TinySTM's costliest operations move from commit/start (STAMP) to write/commit (PANDORA);
  intruder spends 55% on aborts (rollback 18.9% plus the fibre switch each abort triggers,
  35.9%) against 18.9% on transactional reads, and vacation_high 55.6% on transactional reads.
  *E5 (optional): needs `perf` and root on the host, so it cannot run inside Docker. Binaries
  built in the Docker image run on an Ubuntu 20.04 host as they are (install
  `linux-tools-$(uname -r)` and `libgoogle-perftools4`). The shares are machine-dependent;
  their ordering is what should match. Results: `results/profile/*/top-self.txt` (time in the
  function itself) and `top-children.txt` (including its callees). All numbers above are
  "self" shares, except the switch on abort, which is `cfibre_yield` in `top-children.txt`
  (`cfibre_yield` is called only from TinySTM's abort path, `backends/tinystm/tm.h`).*

**Not supported, and why.**

- **HTM (Intel TSX) results** (HTM panels of Figs. 2-4 and S1-S9; the HTM statements in
  C1, C4 and Sec. 6.2, e.g. yada's 29.8% fallback rate). TSX is disabled by microcode on most
  current Intel CPUs, so we do not ask reviewers to reproduce them. Everything is built with
  `ae/build_all.sh --htm`, and `experiments.sh` includes the `rtm`/`stampede-htm` series
  automatically when `/proc/cpuinfo` reports `rtm`.
- **Absolute speedups and abort counts** depend on the machine and on timing.

---

## 5. Notes for reviewers

- **Machine setup.** For performance runs, disable Hyper-Threading and Turbo Boost if
  possible and leave the machine otherwise idle. PANDORA runs on libfibre (user-level
  threads): `<cores>` OS workers, each multiplexing 4 fibers (TM) or 1 fiber (locks). STAMP
  and Stampede use one pthread per core (Table 3). As for the paper's measurements, no
  thread-to-core binding is applied (`ENABLE_BIND` stays off in `common/env.mk`).
- **Containers.** Running inside Docker is fine for the measurements. Stampede's Galois
  runtime disables its hardware-topology thread mapping when `/dev/cpuset` is absent and
  prints a warning while building. The paper's machine has no `/dev/cpuset` either, so the
  paper's Stampede runs had the mapping disabled too: Docker and host runs match the paper here.
- **TinySTM statistics.** As in the paper, all TinySTM builds keep TinySTM's built-in
  commit/abort counters (`TM_STATISTICS`, `TM_STATISTICS2`); they only increment
  per-transaction counters on commit/abort. Performance runs of PANDORA and STAMP do not set
  `STM_STATS`, so TinySTM's statistics module is not loaded and no abort counts are printed;
  Stampede's own run script always sets `STM_STATS=1`, which we keep. The instrumented build
  for Table 4 (`-DTINYSTM_STATS`, series `pandora-tinystm-stats`) adds per-transaction
  read/write-set and time measurement and is never used for performance figures.
- **Allocator.** PANDORA and STAMP (TM, locks and sequential) load the TCMalloc in
  `allocator/`: gperftools 2.13 with the batch of objects moved between thread caches and the
  central free lists enlarged eightfold (`allocator/README.md`). `ae/build_all.sh` builds it, and
  the binaries find it through their RUNPATH, not the system's `libtcmalloc.so.4`
  (check with `ldd <binary> | grep tcmalloc`). Stampede uses its own allocators.
  The choice matters most under HTM: with the system's TCMalloc (gperftools 2.7 on Ubuntu 20.04)
  vacation on HTM at 40 cores runs at about half the speedup, under both PANDORA and STAMP.
- **TinySTM pointer accesses.** In `backends/tinystm/tm.h`, `TM_SHARED_READ_P` /
  `TM_SHARED_WRITE_P` pass the transaction descriptor explicitly (`stm_load_tx` /
  `stm_store_tx`, TinySTM's stock API), like the scalar accesses. The `stm_load_ptr` /
  `stm_store_ptr` wrappers instead look the descriptor up in thread-local storage on every
  access, which under libfibre is a `pthread_getspecific` call per pointer read; this
  slowed pointer-heavy workloads (vacation) and was not the configuration the paper's
  TinySTM numbers were meant to reflect. All TinySTM numbers in the camera-ready paper were
  measured with this artifact's `tm.h`.
- **TinySTM re-measurement.** The submitted version's TinySTM abort counts (and some STAMP/TM
  speedups) came from a TinySTM build that added, for diagnostics, a global atomic counter
  update on every commit and abort (`-DSTATIC_ID`); that serializing update lowered aborts.
  The camera-ready draft replaces every TinySTM number with measurements of the stock
  TinySTM 1.0.5 shipped here (5 runs, 12 core counts). Under STAMP the abort counts grew
  (e.g. kmeans_high 0.88 -> 11.8 billion at 40 cores); PANDORA's counts changed little, so
  the reductions reported in Sec. 6.2 widened. One sentence of the submitted Sec. 6.2 (aborts
  "rise 36-fold" without switching on kmeans) did not survive and was removed.
- **genome correction.** After submission we found that a `thread_barrier_wait()` after
  step 1 of genome's sequencer had been commented out in PANDORA (`genome/sequencer.c`).
  This artifact contains the corrected code, which matches STAMP at that point
  (`diffs/genome.diff`); all genome numbers in the camera-ready paper and in `data/` were
  re-measured with it. The conclusions did not change.
- **TinySTM in the STAMP baseline.** `orig/switch-stamp/tinySTM-1.0.5` differs from the copy
  used by PANDORA only in debugging aids that are compiled out and in summing the
  per-thread commit/abort counters into global ones when a thread exits, which it prints at
  the end of every run; the transactional code paths are identical. This is why STAMP runs
  always report abort counts while PANDORA runs report them only in the `-stats` series.
  The two `backends/tinystm/tm.h` also differ in two macros, as in the paper's runs: STAMP's
  `TM_BEGIN` starts a transaction with `stm_start_id` (one thread-local lookup of the
  descriptor per transaction, a `pthread_getspecific` under STAMP's pthreads), PANDORA's with
  `stm_start_tx` on the descriptor it holds; and STAMP writes pointers with
  `stm_store_ptr_tx`, PANDORA with `stm_store_tx`, which perform the same store. Every
  transactional read and write passes the descriptor explicitly in both suites, except
  float/double accesses, which look it up in both.
  Replacing STAMP's `TM_BEGIN` with PANDORA's changed STAMP/TinySTM's times at 40 cores by
  less than their run-to-run variation (kmeans_high, ssca2, genome; five runs each).
- **ssca2 timer.** ssca2 prints several timers; as in the paper we use
  `Time taken for kernel 1` (the transactional kernel), see `SSCA2_TIMER` in `ae/collect.py`.
- **kmeans precision.** kmeans uses double precision in all three suites and in the
  transactional read paths of TL2 and NOrec (Sec. 5.1).
- **Large inputs.** `inputs/kmeans/random-n1048576-d32-c16.txt` (511 MB) is regenerated with
  STAMP's generator (fixed seed); `inputs/kmeans/generate3.py` is a Python 3 port of STAMP's
  Python 2 `generate.py` that produces the identical file. `inputs/yada/ttimeu1000000.2.*` is shipped
  compressed as in the STAMP distribution. `ae/prepare_inputs.sh` checks both against the
  SHA-256 of the files used in the paper.
- **Stampede kmeans fix.** Upstream Stampede's kmeans prints a `SUM SQUARED ERROR` after the
  timed region from an array that is never filled; with small inputs this reads garbage
  indices and can crash depending on the machine. We fill the array (`cluster.c`, outside
  the timed region); timings are unaffected.
- **Rare verification failure of labyrinth on TL2.** With the small input on many threads,
  labyrinth on TL2 occasionally fails its own path check (`Assertion 'status == TRUE'` in
  `labyrinth.c`), in PANDORA and in STAMP alike, with either TCMalloc. It does not occur on
  the other TM systems or the locks, nor with TL2 built for eager write-back (`-DTL2_EAGER`),
  so it lies in TL2's default commit-time (lazy) write-back path, which is the configuration
  the paper evaluates (Table 1). We have not seen it with the large input. `ae/run.sh` marks
  such a run FAILED; running the same command again repeats only the failed runs.
- **Stampede's HTM fallback.** Stampede has no fallback path for Intel TSX; we added one
  based on Nakaike et al. (`orig/stampede/benchmarks/htm/`, Sec. 6.1).
- **Starting over.** `ae/clean.sh` removes all build products; `ae/clean.sh --dist` also
  removes results and generated inputs, returning the tree to its shipped state.

---

## 6. Reusing PANDORA

- **Other core counts / repetitions:** `CORES=... REPS=...` (Sec. 3), or `ae/run.sh` directly.
- **Another TM runtime:** add `backends/<name>/{tm.h,Makefile}` (start from
  `backends/tinystm` and map the `TM_*` macros to your runtime), add `<name>` to `STMS` in
  `ae/common.sh`, and rebuild with `ae/build_all.sh`. The workloads need no change: keeping
  conflict detection, version management and scheduling below the `TM_*` interface is the
  point of the suite (Sec. 4, Sec. 5.1).
- **Other inputs:** every configuration's arguments are in `args_of` in `ae/common.sh`
  (`large` = the paper's, `small` = smoke-test inputs).
- **Without libfibre** (non-x86/Arm): comment out `USE_ULT` in `common/env.mk`.
- **Programming-effort analysis of new code:** `scripts/run_cloc_analysis.sh <backend> <workload>`.

---

## 7. Licensing

See `LICENSE` (GPLv3) and `NOTICE`. PANDORA derives from STAMP (BSD-style). Bundled runtimes keep their own
licenses; libfibre is GPLv3, so redistribution of the archive as a whole is subject to the
GPLv3. Stampede and Galois 2.2.1 are third-party code under their own terms; the public
(Zenodo) version of this artifact therefore ships only our patches for them, and
`ae/stampede/fetch.sh` downloads the upstream sources and rebuilds `orig/stampede`.
