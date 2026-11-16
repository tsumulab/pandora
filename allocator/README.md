# allocator/ -- the TCMalloc used for the paper's measurements

- `gperftools-2.13.tar.gz` -- upstream release (github.com/gperftools/gperftools, tag
  `gperftools-2.13`), SHA-256 `4882c5ec...7323e7`, BSD-3-Clause (`COPYING` inside).
- `tcmalloc-batch8.patch` -- the only change: `SizeMap::num_objects_to_move()` returns 8x the
  default, i.e. a thread cache moves objects to and from the central free lists in batches eight
  times larger, which reduces contention on the central lists' locks under allocation-heavy
  parallel workloads.

`ae/build_allocator.sh` (called by `ae/build_all.sh`) builds it into `allocator/lib/`. The
binaries are linked with a RUNPATH to that directory, so they load this library rather than the
system's `libtcmalloc.so.4`; `ldd <binary> | grep tcmalloc` shows which one is used.

The paper's measurements used a gperftools snapshot taken shortly after 2.13; its sources differ
from the 2.13 release only in changes that do not affect Linux (QNX/Windows support, MPI/SLURM
environment handling, an inlining hint), plus the one-line change above.
