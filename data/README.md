# data/ -- figure-source data

These are the numbers behind the paper's figures and Table 4, tidied from the authors' Excel
workbook: the **exact numbers plotted in the figures**, extracted programmatically from the
workbook's embedded charts, not obtained from a fresh run. They are plain CSV; `ae/plot.py`
draws them next to your own measurements.

All genome points and all TinySTM points (`PANDORA/TM` with switching on and off,
`STAMP/TM`, and every TinySTM value of Table 4) were measured with the code in this
repository: original TinySTM 1.0.5, `backends/tinystm/tm.h` with explicit-descriptor pointer
accesses, and the TCMalloc of `allocator/` (as for all other curves); 5 runs per point. Speedups
are over the sequential-STAMP times used throughout the paper (genome 6.100509 s, intruder
13.695798, kmeans_high 57.7789, kmeans_low 474.261, labyrinth 34.576321, ssca2 3.4235614,
vacation_high 15.206107, vacation_low 11.56033, yada 6.1).

## Files

- `speedup.csv` -- speedup data for Figures 2-11 and for Table 4's switch gain.
- `switch_extra.csv` -- series of the authors' switch-comparison and exploratory charts.
- `loc.csv` -- the LOC table behind Table 4's LOC ratio.
- `table4.csv` -- Table 4.

## `speedup.csv` (Figures 2-11, Table 4's switch gain)

One row per plotted point. Speedup is over sequential STAMP on the same input.

| column | meaning |
|---|---|
| figure | paper figure: `Fig2` genome, `Fig3` kmeans_high, `Fig4` kmeans_low, `Fig5` ssca2, `Fig6` intruder, `Fig7` vacation_high, `Fig8` vacation_low, `Fig9` labyrinth, `Fig10` yada, `Fig11` switch vs non-switch; `Table4` (see below) |
| tm_system | TL2 / TinySTM / NOrec / SwissTM / HTM (the panel) |
| workload | genome, intruder, kmeans, labyrinth, ssca2, vacation, yada |
| level | high / low / na (for kmeans, vacation) |
| series | PANDORA/TM, PANDORA/fine-lock, PANDORA/coarse-lock, Stampede/TM, STAMP/TM |
| switch | on / off / na (transaction switching) |
| cores | core count (measured at 1, 2, 4, 8, 12, ... 40) |
| speedup | speedup over sequential STAMP |

The sequential-STAMP reference line (`STAMP/seq`) is the denominator, so it is 1.0 at every core
by definition; it is omitted here rather than shipped as constant-1 rows.

Rows with `switch = on` or `na` are the curves of Figures 2-10. Some combinations are absent by
design (Stampede's STM results use TinySTM only; yada has no fine-grained lock). Rows with
`switch = off` are the non-switch curves: on all five TM systems for kmeans_high and ssca2
(`Fig11`, drawn there with the switch and fine-lock curves of the same workload), and on TinySTM
for the other seven configurations (`Table4`). Table 4's switch gain is the ratio of the
`switch = on` to the `switch = off` speedup of PANDORA/TM on TinySTM at 40 cores.

The `PANDORA/fine-lock` and `PANDORA/coarse-lock` curves are **independent of the TM backend**
(they use locks, not TM), so for a given workload the same curve is listed under every
`tm_system` -- it is the identical line drawn in each of that workload's panels, not a
per-system re-measurement.

Notes on reconstructed cells (the source workbook's SwissTM charts and a few lock series were
incomplete; these were recovered without new measurements):
(1) `TinySTM / yada / PANDORA/coarse-lock` -- taken from the other systems' identical yada
coarse-lock curve (lock performance is independent of the TM backend; byte-identical across
HTM/NOrec/TL2).
(2) **SwissTM `PANDORA/TM` and `STAMP/TM`** (all workloads) -- read from the authoritative
result cells in the source sheet (the SwissTM charts had been zeroed).
(3) **SwissTM `PANDORA/fine-lock` / `PANDORA/coarse-lock`** -- filled from the other systems'
identical lock curves (TM-independent). A cross-system consistency check found **no** lock-curve
disagreements for any workload.

## `switch_extra.csv`

Series from the switch-comparison and other charts in the authors' workbook, kept with their
original labels and **not** auto-classified; some of these charts are not in the paper. The
figures of the paper are drawn from `speedup.csv`. Columns: `chart`, `chart_title`, `series_label`,
`cores`, `value`. The TinySTM series of these charts (PANDORA switch/non-switch and STAMP; 288
points) follow the TinySTM measurements of `speedup.csv`: each point is scaled by the ratio of
the two measurements of the same point, which keeps each chart's own normalisation (some charts differ from `speedup.csv` by up to +/-13%).

## `LOC` (Table 4, LOC ratio)

Lines of code per backend and the resulting ratios, behind Table 4's `LOC ratio (fine/TM)`
column. Columns: `workload, tinystm_LOC, fine_lock_LOC, coarse_lock_LOC, ratio_fine/tinystm,
ratio_coarse/tinystm`. **LOC excludes the backend `tm.h`** (the Table 4 definition), so the
lock versions differ from the TM version only in application code; `ratio_coarse/tinystm` is 1.0
because the coarse-lock version's application code is identical to the TM version. `yada` has no
deadlock-free fine-grained lock, so its fine ratio is `N/A` (paper Sec. 6.1). `scripts/run_all_cloc.sh`
recomputes these from source.

## Reproducing

You can regenerate the underlying numbers with `scripts/msr.sh` + `Logs/summarize.sh`
(see `../MAPPING.md` Sec. 4); the resulting curves match these figures. Exact values will differ
slightly run to run, as is normal.

## `table4.csv` (Table 4)

The values of Table 4, one row per configuration: `tx_count` (#Tx), `read_set` /
`write_set` (mean |R| / |W| per committed transaction), `tx_time_pct` (share of aggregate core
time spent in transactions), `aborts_stamp` -> `aborts_pandora`, `loc_ratio_fine_tm` (empty
for yada), `seq_ratio` (sequential STAMP time over sequential PANDORA time) and `switch_gain`
(from `speedup.csv`, see above), plus `aborts_pandora_noswitch` (the same instrumented build with switching off,
quoted in Sec. 6.2, not a column of the printed table). Compare with E3 (`results/summary.csv`, series `stamp-tinystm-stats` and
`pandora-tinystm-stats` at 40 cores) and E4.

