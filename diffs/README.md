# diffs -- what PANDORA changed, against pristine baselines

Each file here is a unified diff (`diff -ruN`) from an unmodified upstream baseline to the
version shipped in this archive. Added lines (`+`) are PANDORA's; removed lines (`-`) are the
baseline's. They exist so a reviewer can see exactly what was changed and why, without diffing
trees by hand.

## Baselines

- **STAMP** -- the original Stanford STAMP distribution (the `*.diff` workload files and
  `lib.diff` are taken against it).
- **TinySTM 1.0.5** -- upstream TinySTM (`tinystm.diff`).

## Workload redesigns (paper Sec. 5.2 and appendix Sec. A)

STAMP -> PANDORA source changes, one file per workload:

| File | Workload |
|---|---|
| `genome.diff` | genome |
| `intruder.diff` | intruder |
| `kmeans.diff` | kmeans |
| `labyrinth.diff` | labyrinth |
| `ssca2.diff` | ssca2 |
| `vacation.diff` | vacation |
| `yada.diff` | yada |

These show the access-dispersal and transaction-boundary changes described in the paper (e.g.
genome's tail-append instead of sorted-chain insertion, kmeans' per-thread counters and
accumulators, ssca2's enlarged transaction boundary). Build files (`Makefile*`, `*.mk`) are
excluded: PANDORA replaces STAMP's per-flavor Makefiles with a unified build
(`backends/` + `common/`), which is a packaging change, not an algorithm change.

## Shared harness (paper Sec. 5.1)

- `lib.diff` -- changes to the shared library/harness (`lib/`): thread management for
  multicore-multithread execution, the libfibre integration and pthread-compatible path
  (`thread.c`/`thread.h`), the transactional-memory macro layer (`tm.h`), and memory macros.
  It also carries the `TINYSTM_STATS`-gated **measurement counters** in `thread.c`
  (read/write-set sizes and per-core transaction time) that produce Table 4's |R|/|W| and
  Tx-time columns.

## Runtime & measurement instrumentation

- `tinystm.diff` -- **build configuration only; TinySTM's sources are unmodified (stock
  upstream).** `Makefile` enables TinySTM's built-in statistics (`-DTM_STATISTICS
  -DTM_STATISTICS2`, i.e. the commit/abort counts) and selects POSIX TLS (`-DTLS_POSIX`
  instead of `-DTLS_COMPILER`, so thread-local TM state follows libfibre's fibres).
  `Makefile.common` disables FORTIFY (`-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0`; see INSTALL.md
  troubleshooting: libfibre + `siglongjmp`). Architecture/vendor subtrees (`asf/`,
  `atomic_ops/`, `abi/`, `test/`, `doc/`) are excluded from this diff.

- The per-transaction **measurement** instrumentation for Table 4 (read/write-set sizes and
  per-core transaction time) lives in the shared harness, not in TinySTM -- see `thread.c` in
  `lib.diff`, gated by `#if defined(TINYSTM_STATS)`. Commit/abort counts come from TinySTM's
  built-in statistics (above). A default build (no `TINYSTM_STATS`, no stats defines) yields the
  stock runtime used for the **performance** figures; a stats build yields the instrumented
  runtime used for **Table 4** -- matching the paper's note that "abort and set-size figures use
  an instrumented build; performance figures do not."

## libfibre

- `libfibre.diff` -- libfibre is bundled from upstream (provenance/commit in `MAPPING.md` Sec. 1)
  with a single, minimal **build-time configuration** change in `src/*/testoptions.h.default`:
  two compile-time options are disabled -- `TESTING_LOADBALANCING` (worker load balancing via
  ISRS) and `TESTING_CLUSTER_POLLER_FIBRE` (per-cluster poller runs on a pthread rather than a
  fibre) -- and one bug fix in `src/runtime/Stack.S`: a new fibre's initial stack was 8 bytes
  off the x86-64 ABI alignment, which crashes code that relies on it (e.g. SSE spills in some
  TCMalloc builds); one padding slot fixes it. libfibre's `.cc`/`.h` sources are unmodified.
  This is the libfibre used for all reported measurements; the bundled tree reproduces it.

## Regenerating these diffs

With pristine baselines in `master/stamp` and `master/tinystm-1.0.5` (siblings of the
supplement), from the directory that contains both trees:

```bash
# workloads (+ lib): exclude build artifacts and build files
printf '%s\n' '*.o' '*.a' '*.so' '.git' 'build' 'Makefile*' '*.mk' > /tmp/exc
for w in genome intruder kmeans labyrinth ssca2 vacation yada lib; do
  diff -ruN -X /tmp/exc master/stamp/$w pandora-supplement/$w > pandora-supplement/diffs/$w.diff
done

# tinystm: keep Makefiles, drop arch/vendor/build subtrees
printf '%s\n' '*.o' '*.a' '*.so' '.git' 'doc' 'test' 'test-asf' 'asf' 'atomic_ops' 'abi' 'lib' \
  > /tmp/exc_rt
diff -ruN -X /tmp/exc_rt master/tinystm-1.0.5 pandora-supplement/tinySTM-1.0.5 \
  > pandora-supplement/diffs/tinystm.diff
```
