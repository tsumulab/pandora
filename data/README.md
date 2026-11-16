# data/ -- figure-source data

These are the numbers behind the paper's figures and Table 4, tidied from the authors' Excel
workbook. The values are the **exact numbers plotted in the figures** (submitted version; genome: camera-ready, see below) -- they were
extracted programmatically from the workbook's embedded charts, not obtained from a fresh run.
The data is provided as plain CSV; `ae/plot.py` draws it next to a reviewer's measurements.

**genome (camera-ready).** All PANDORA genome points (PANDORA/TM on all five TM systems with
switching, TinySTM without switching, and both lock versions) were re-measured after the
barrier correction described in the top-level README (Sec. 5) and replaced here: means of the
re-measured runs, divided by the unchanged sequential-STAMP time of 6.100509 s. The
STAMP/TM and Stampede/TM genome points are unaffected by the correction and unchanged.

**TinySTM (camera-ready).** All TinySTM `PANDORA/TM` (switch on and off) and `STAMP/TM`
points, and every TinySTM value of Table 4, were re-measured with this artifact's build
(original TinySTM 1.0.5; `backends/tinystm/tm.h` with explicit-descriptor pointer accesses,
see the top-level README; the TCMalloc of `allocator/`, as for all other curves), 5 runs per point, and divided by the paper's sequential-STAMP
times (genome 6.100509 s, intruder 13.695798, kmeans_high 57.7789, kmeans_low 474.261,
labyrinth 34.576321, ssca2 3.4235614, vacation_high 15.206107, vacation_low 11.56033,
yada 6.1). The submitted version's abort counts had come from a TinySTM build with extra
global commit/abort counters (`STATIC_ID`), which lowered them.

## Files

- `speedup.csv` -- speedup data for Figures 2-4 and S1-S12 (see the figure notes below).
- `switch_extra.csv` -- series from the switch-comparison and exploratory charts (Fig 5, S10-S12,
  and some non-paper charts).
- `loc.csv` -- the LOC table behind Table 4's LOC ratio.

## `speedup` (Figures 2-4 and S1-S9)

One row per plotted point. Speedup is over sequential STAMP on the same input.

| column | meaning |
|---|---|
| figure | which paper figure (e.g. `S1(=Fig2)`) |
| tm_system | TL2 / TinySTM / NOrec / SwissTM / HTM (the panel) |
| workload | genome, intruder, kmeans, labyrinth, ssca2, vacation, yada |
| level | high / low / na (for kmeans, vacation) |
| series | PANDORA/TM, PANDORA/fine-lock, PANDORA/coarse-lock, Stampede/TM, STAMP/TM |
| switch | on / off / na (transaction switching) |
| cores | core count (measured at 1, 2, 4, 8, 12, ... 40) |
| speedup | speedup over sequential STAMP |

The sequential-STAMP reference line (`STAMP/seq`) is the denominator, so it is 1.0 at every core
by definition; it is omitted here rather than shipped as constant-1 rows.

Figure map (switch = on / na rows): S1=Fig 2 genome, S2=Fig 3 kmeans_high, S3 kmeans_low,
S4=Fig 4 ssca2, S5 intruder, S6 yada, S7 vacation_high, S8 vacation_low, S9 labyrinth. Some
combinations are absent by design (Stampede's STM results use TinySTM only; yada has no
fine-grained lock).

The **nonswitch** rows (`switch = off`) belong to the switch-analysis figures, not the S1-S9
panels: `S10` = kmeans_high across all 5 TM systems, `S11` = ssca2 across all 5 TM systems,
`S12` = TinySTM across the remaining workloads. (kmeans_high/ssca2 on TinySTM also appear in
S12 in the paper; here they are tagged S10/S11.)

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

## `switch_extra` (Figure 5, S10-S12, and exploratory charts)

Series from the switch-comparison and other charts in the workbook, kept with their original
labels and **not** auto-classified. This includes the material behind Figure 5 and S10-S12 as
well as some charts not used in the paper. Columns: `chart`, `chart_title`, `series_label`,
`cores`, `value`. The TinySTM series of these charts (PANDORA switch/non-switch and STAMP; 288
points) were updated with the camera-ready re-measurement: each point is scaled by the ratio of
the new to the old value of the same point in `speedup.csv`, which keeps each chart's own
normalisation (some charts differ from `speedup.csv` by up to +/-13%).

## `LOC` (Table 4, LOC ratio)

Lines of code per backend and the resulting ratios, behind Table 4's `LOC ratio (fine/TM)`
column. Columns: `workload, tinystm_LOC, fine_lock_LOC, coarse_lock_LOC, ratio_fine/tinystm,
ratio_coarse/tinystm`. **LOC excludes the backend `tm.h`** (the Table 4 definition), so the
lock versions differ from the TM version only in application code; `ratio_coarse/tinystm` is 1.0
because the coarse-lock version's application code is identical to the TM version. `yada` has no
deadlock-free fine-grained lock, so its fine ratio is `N/A` (paper Sec. 6.1). `scripts/run_all_cloc.sh`
recomputes these from source.

## Reproducing

A reviewer can regenerate the underlying numbers with `scripts/msr.sh` + `Logs/summarize.sh`
(see `../MAPPING.md` Sec. 4); the resulting curves match these figures. Exact values will differ
slightly run to run, as is normal.

## `table4.csv` (Table 4)

The camera-ready values of Table 4, one row per configuration: `tx_count` (#Tx), `read_set` /
`write_set` (mean |R| / |W| per committed transaction), `tx_time_pct` (share of aggregate core
time spent in transactions), `aborts_stamp` -> `aborts_pandora`, and `loc_ratio_fine_tm` (empty
for yada), plus `aborts_pandora_noswitch` (the same instrumented build with switching off,
quoted in Sec. 6.2, not a column of the printed table). Compare with E3 (`results/summary.csv`, series `stamp-tinystm-stats` and
`pandora-tinystm-stats` at 40 cores) and E4.

