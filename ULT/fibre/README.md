# ULT/fibre -- pthread-to-libfibre wrapper

These files map the pthread API onto libfibre fibres, so that the benchmarks run on
libfibre without source changes (`common/env.mk`, `USE_ULT`).

They are derived from the pthread wrapper of MassiveThreads
(https://github.com/massivethreads/massivethreads: `src/myth_wrap_pthread.c`,
`src/myth_wrap_util_func.h`, `src/myth_real.h`), adapted to call libfibre instead of
MassiveThreads. MassiveThreads is distributed under the BSD-style license reproduced in
`LICENSE.MassiveThreads`, which applies to these files.
