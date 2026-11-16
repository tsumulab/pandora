# PANDORA

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23908116.svg)](https://doi.org/10.5281/zenodo.23908116)

PANDORA is a transactional-memory (TM) benchmark suite derived from STAMP. Its workloads
were redesigned so that conflict detection, version management, and transaction scheduling
are left to the TM runtime rather than to the application, so that what it measures is the
runtime, not how the benchmark was programmed around it.

This repository contains the seven PANDORA workloads (nine configurations), their
productivity-matched fine- and coarse-grained lock versions, back-ends for four STM runtimes
(TL2, TinySTM, NOrec, SwissTM) and Intel TSX, the libfibre user-level threads PANDORA runs
on, the STAMP and Stampede baselines, and scripts that build everything, run the
measurements, and draw the figures of the paper.

PANDORA is described in

> Hiroki Sato, Toru Koizumi, Ryota Shioya, Masahiro Goshima, and Tomoaki Tsumura.
> *Measuring Transactional Memory: PANDORA and the Division of Labor.*
> PPoPP 2027.

If you use PANDORA, please cite the paper (see `CITATION.cff`).

**Versions.** Releases of this repository are archived on Zenodo under
[10.5281/zenodo.23908116](https://doi.org/10.5281/zenodo.23908116), which always resolves to
the latest release. Version 1.0 ([10.5281/zenodo.23908117](https://doi.org/10.5281/zenodo.23908117))
is the one evaluated in the paper's artifact evaluation. See `CHANGES.md` for what changed.

**Stampede.** The Stampede baseline (Stampede, its bundled Galois 2.2.1 and TinySTM 1.0.4) may
not be redistributed, so this repository ships only our patches to it.
`ae/stampede/fetch.sh` downloads the upstream sources, checks their checksums, and applies the
patches; run it once before building (Quick start, step 1).

| | |
|---|---|
| Hardware | x86-64 Linux; the paper uses 40 cores, >= 8 cores recommended; Intel TSX only for the HTM results |
| Software | Docker (or Podman/Apptainer) on a native x86-64 Linux host, or Ubuntu 20.04-24.04 without containers. Not supported: Docker Desktop on macOS or Windows |
| Disk | ~3 GB including the image |
| Time | ~10 min to build and smoke-test; ~1.5 h for the key experiments on 40 cores; ~14 h for all |

---

## Quick start

**Host.** Run on a native x86-64 Linux host and clone or unpack the tree there: the tree contains
file names that differ only in case (e.g. `ULT/libfibre/src/libfibre/{Fibre.h,fibre.h}`),
which a case-insensitive file system (the default on macOS and Windows) silently merges, and
Docker Desktop's virtual machine (or x86 emulation on Arm Macs) distorts the timings the
claims are about. The container uses the host kernel; any recent kernel works. For the HTM
results the host CPU and kernel must also expose Intel TSX (`rtm` in `/proc/cpuinfo`).

```bash
# 0. clone (or unpack a release), then from the repository root (~4 min):
docker build -t pandora-ae docker/

# 1. download and patch the Stampede baseline into orig/stampede (needs network; once)
ae/stampede/fetch.sh

# 2. start a container with the tree mounted at /pandora, running as yourself so that
#    everything it writes stays yours
docker run --rm -it --user "$(id -u):$(id -g)" -e HOME=/tmp -v "$PWD":/pandora pandora-ae bash

# 3. inside the container, in /pandora: build everything and run all nine
#    configurations on small inputs (~5 min)
ae/kick_the_tires.sh
```

Step 1 needs `git`, `curl` and `patch` on the host; alternatively run it inside the
container (`docker run ... pandora-ae ae/stampede/fetch.sh`). `ae/kick_the_tires.sh` and a
plain `ae/build_all.sh` expect the Stampede baseline. Without it, build only what you need
(`ae/build_all.sh --only pandora`, `--only stamp`) and run it with `ae/run.sh`.

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
  then run the same `ae/` commands from the repository root on the host. We checked this on
  Ubuntu 24.04 with gcc 13.3: every binary builds, and `kick_the_tires.sh` and a small-input
  run of every series pass. The paper's numbers were measured with gcc 9.4 (the image);
  another compiler can shift absolute timings, not the claims' shape.

---

## Using PANDORA

- **Other core counts / repetitions:** `CORES=... REPS=...` (see Running the experiments), or `ae/run.sh` directly.
- **Another TM runtime:** add `backends/<name>/{tm.h,Makefile}` (start from
  `backends/tinystm` and map the `TM_*` macros to your runtime), add `<name>` to `STMS` in
  `ae/common.sh`, and rebuild with `ae/build_all.sh`. The workloads need no change: keeping
  conflict detection, version management and scheduling below the `TM_*` interface is the
  point of the suite (Sec. 4, Sec. 5.1).
- **Other inputs:** every configuration's arguments are in `args_of` in `ae/common.sh`
  (`large` = the paper's, `small` = smoke-test inputs).
- **Without libfibre** (non-x86/Arm): comment out `USE_ULT` in `common/env.mk`.
- **Programming-effort analysis of new code:** `scripts/run_cloc_analysis.sh <backend> <workload>`.


The workloads talk to the TM runtime only through the `TM_*` macros of `lib/tm.h`, which each
back-end in `backends/` defines; `backends/tinystm/tm.h` is the reference.

---

## Reproducing the paper

Section, figure and table numbers below refer to the PPoPP 2027 paper. `data/` holds the
numbers plotted in the paper, and `MAPPING.md` lists which experiment produces which figure
or table.

### Running the experiments

All commands run inside the container from `/pandora`. Each experiment writes raw logs
under `results/raw/`, then refreshes `results/summary.csv` and `results/figures/`.
Experiments can be interrupted and re-run; finished runs are kept.
The large kmeans and yada inputs are generated and checksum-verified on first use (~1 min).

By default every experiment uses **core counts 1, N/2 and N** (N = cores of the machine,
capped at 40, the paper's maximum) and **3 repetitions** (the paper uses the mean of 5 runs at 1, 2, 4, 8, 12, ..., 40 cores). Both can be changed:

```bash
CORES="1 2 4 8 12 16 20 24 28 32 36 40" REPS=5 ae/experiments.sh E1   # the paper's sweep
```

| Exp. | Command | Paper | Time (40 cores, defaults) |
|---|---|---|---|
| E1 | `ae/experiments.sh E1` | Figs. 2, 3, 5 (genome, kmeans_high, ssca2), TinySTM panel | 53 min |
| E1-full | `ae/experiments.sh E1-full` | Figs. 2-10 (9 configurations x all TM systems) | 11 h 16 min |
| E2 | `ae/experiments.sh E2` | Fig. 11, TinySTM panels (switch vs. non-switch); kmeans_high at 1 fiber/core (Table 3) | 7 min after E1 (reuses its runs) |
| E2-full | `ae/experiments.sh E2-full` | Fig. 11, all TM systems; Table 4, switch gain | 55 min after E1-full |
| E3 | `ae/experiments.sh E3` | Table 4 (#Tx, \|R\|/\|W\|, Tx time, aborts); abort ratios of Sec. 6.2 | 25 min |
| E4 | `ae/experiments.sh E4` | Table 4, LOC ratio (fine-lock/TM) | < 1 min |
| E5 (optional) | `sudo ae/experiments.sh E5` on the host (not in Docker) | Sec. 6.2 profiling breakdowns (perf) | 1 min (6 profiles; after a build) |

**Where to look.**

- `results/figures/large_<config>_<TM>.png`: this machine's curves (solid) drawn over
  the curves of the paper (dashed, from `data/speedup.csv`), one file per panel.
- `results/paper-figures/`: the paper's figures (Figs. 2-11) in the paper's layout,
  drawn from this machine's results by `python3 ae/paper_figures.py` (after `ae/collect.py`).
  `python3 ae/paper_figures.py --paper-csv data/speedup.csv` draws the same figures from the
  values plotted in the paper, into `results/paper-figures-data/`, for a side-by-side look.
  File names: `speedup_<config>` (Figs. 2-10), `switch_all_tm` (Fig. 11), plus views that
  are not figures of the paper (`switch_tinystm`, `switch_<config>_all_tm`,
  `switch_tinystm_all`); `--name-by-figure` prefixes the figure number. Panels without data (e.g. HTM without TSX) are left empty.
- `results/summary.csv`: one row per (series, configuration, core count) with mean and
  standard deviation of the execution time, the speedup over sequential STAMP, mean abort
  and commit counts and their ratio, and -- for the `-stats` series of E3 -- mean read/write-set
  sizes. Table 4's columns map as follows: #Tx = `commits_mean`, |R| / |W| =
  `avg_read_set` / `avg_write_set` of `pandora-tinystm-stats`; Aborts (STAMP -> PANDORA) =
  `aborts_mean` of `stamp-tinystm-stats` -> `pandora-tinystm-stats`. Tx time is printed per
  run as `Total Tx Time(second)` (column `tx_time_s` of `results/runs.csv`); divide it by
  cores x execution time for the fraction reported in Table 4. Switch gain = `speedup` of
  `pandora-tinystm` over `pandora-tinystm-noswitch` at 40 cores (E2-full).
- E4 prints the LOC table; it is deterministic and must equal `data/loc.csv`.

**Series names** (see `ae/common.sh`):

| Series | Meaning in the paper |
|---|---|
| `pandora-<tm>` | PANDORA/TM, `<tm>` in tinystm, tl2, norec, swisstm, rtm; 4 fibers/core, switching on |
| `pandora-<tm>-noswitch` | same, switching off (Fig. 11, Table 4) |
| `pandora-tinystm-1fpc` | PANDORA/TM on TinySTM at 1 fiber/core (Table 3, Sec. 6.2) |
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

### Claims

Absolute speedups depend on the machine (ours: single-socket Xeon Platinum 8380, 40 cores,
HT and Turbo off; Sec. 6.1); what should be reproduced is the shape and ordering of the
curves and the order of magnitude of the counts. On a machine with fewer cores, compare at
its maximum.

**Reproducible with the scripts.**

- **C1 -- PANDORA measures runtime costs that STAMP's contention hides (genome; Sec. 6.2, Fig. 2, Table 4).**
  On TinySTM at 40 cores, aborts fall from 199,710 under STAMP to 8,179 with the redesign
  alone (switching off) and to 2,903 with switching. Under PANDORA, genome scales on most
  runtimes while NOrec falls behind STAMP from 16 cores on. *E1, E1-full (NOrec panel), E3.*
- **C2 -- PANDORA tells runtimes apart (kmeans; Sec. 6.2, Fig. 3, Table 4).**
  On TinySTM at 40 cores, aborts fall from about 11.8 billion (STAMP) and ~2.2 billion (Stampede)
  to 481,910 (PANDORA); per committed transaction 92 / 0.92 / 0.026. Four runtimes scale
  toward 30x while NOrec peaks near 16 cores and then declines. *E1, E1-full, E3.*
- **C3 -- Runtime-side switching pays off where contention is high and not where it is
  low (Sec. 6.2, Fig. 11, Tables 3-4).** kmeans_high on TinySTM at 40 cores: ~31x with switching,
  ~12x without, ~14x at one fiber per core. ssca2: the non-switch configuration is about
  3.5% faster. *E2, E2-full, E3.*
- **C4 -- PANDORA reports where its own redesign does not help (ssca2; Sec. 6.2, Fig. 5).**
  Aborts rise from 7,903 (STAMP) to 107,916 (PANDORA) as transaction boundaries grow; on the
  STMs PANDORA/TM approaches the fine-grained lock except on NOrec, TinySTM leading. *E1, E1-full, E3.*
- **C5 -- TM and locks on equal terms (Sec. 6.1, Sec. 6.3, Figs. 2-10).** On most workloads and
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

**Not covered.**

- **HTM (Intel TSX) results** (HTM panels of Figs. 2-11; the HTM statements in
  C1, C4 and Sec. 6.2, e.g. yada's 29.8% fallback rate). TSX is disabled by microcode on most
  current Intel CPUs, so they are not part of the claims above. Everything is built with
  `ae/build_all.sh --htm`, and `experiments.sh` includes the `rtm`/`stampede-htm` series
  automatically when `/proc/cpuinfo` reports `rtm`.
- **Absolute speedups and abort counts** depend on the machine and on timing.
- **Table 3 beyond the series above** (TinySTM at 2 and 8 fibers per core, at 1 fiber on genome
  and ssca2; the fine-grained lock at 2, 4 and 8) and **Table 4's Seq. ratio** (sequential
  PANDORA) have no series in `ae/common.sh` yet; `data/table4.csv` holds the paper's values.

---

### Notes on the measurement setup

- **Machine setup.** For performance runs, disable Hyper-Threading and Turbo Boost if
  possible and leave the machine otherwise idle. PANDORA runs on libfibre (user-level
  threads): `<cores>` OS workers, each multiplexing 4 fibers (TM) or 1 fiber (locks). STAMP
  and Stampede use one pthread per core (Table 2). As for the paper's measurements, no
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
  TinySTM numbers were meant to reflect. All TinySTM numbers in the paper were measured
  with this `tm.h`.
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

## Repository layout

```
README.md            this file
CHANGES.md           version history
CITATION.cff         how to cite
docker/Dockerfile    build/run environment (Ubuntu 20.04, gcc 9.4 -- the paper's toolchain)
ae/                  build / run / plot scripts (start here)
  kick_the_tires.sh    build + small-input smoke test of all nine configurations
  experiments.sh       E1-E5 (Running the experiments)
  run_all.sh           E1 ... E4 back to back, unattended, with timings
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
orig/stampede/       Stampede baseline (Nguyen & Pingali, ASPLOS'17), created by ae/stampede/fetch.sh
inputs/              all inputs, shared by PANDORA, STAMP and Stampede
scripts/             original build/run/LOC scripts (used by ae/)
data/                the exact numbers plotted in the paper (speedup.csv, loc.csv, ...)
diffs/               STAMP -> PANDORA source diffs, per workload (paper Sec. 5.2)
MAPPING.md           paper figure/table -> experiment -> output file
NOTICE               licensing index
```

---

## Licensing

PANDORA's own code is licensed under the BSD-3-Clause-Clear license (`LICENSE`). It derives
from STAMP (BSD-style), whose notices are kept. Bundled third-party components keep their own
licenses, listed in `NOTICE`; libfibre, the user-level threads PANDORA runs on by default, is
GPLv3, which applies to binaries linked with it. Stampede and Galois 2.2.1 are third-party code under their own terms
and are not included; `ae/stampede/fetch.sh` downloads them and applies our patches.
