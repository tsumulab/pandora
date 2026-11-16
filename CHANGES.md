# Changes

## Unreleased

- Public repository: added `LICENSE` (GPLv3), `CITATION.cff` and `.zenodo.json`; releases are
  archived on Zenodo as new versions of 10.5281/zenodo.23908116.
- README reorganized for general use; the quick start now runs `ae/stampede/fetch.sh` before
  building, since the Stampede baseline is not included.
- `.gitignore` no longer hides `Logs/all_builder.sh` and
  `orig/switch-stamp/common/Defines.common.mk`, which the STAMP baseline needs to build.

## 1.0 (2026-11-09)

The version evaluated in the PPoPP 2027 artifact evaluation
([10.5281/zenodo.23908117](https://doi.org/10.5281/zenodo.23908117)). Compared with the
code used for earlier measurements, it contains these fixes; the paper's numbers were
re-measured with them, and its conclusions did not change.

- **genome:** a `thread_barrier_wait()` after step 1 of the sequencer had been commented out
  in PANDORA (`genome/sequencer.c`). It is restored, matching STAMP at that point
  (`diffs/genome.diff`).
- **TinySTM build:** TinySTM numbers had come from a build that added a global atomic counter
  update on every commit and abort for diagnostics (`-DSTATIC_ID`), which serializes and
  lowers aborts. All TinySTM numbers now come from the stock TinySTM 1.0.5 shipped here.
  Under STAMP the abort counts grew (e.g. kmeans_high 0.88 -> 11.8 billion at 40 cores);
  PANDORA's changed little.
- **TinySTM pointer accesses:** `backends/tinystm/tm.h` passes the transaction descriptor
  explicitly for pointer reads and writes, as for scalars, instead of looking it up in
  thread-local storage on every access.
- **Allocator:** the TCMalloc used throughout (gperftools 2.13 with one tuning change) is
  shipped in `allocator/`.
