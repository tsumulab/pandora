# Changes

## Unreleased

- Public repository: added `LICENSE` (GPLv3), `CITATION.cff` and `.zenodo.json`; releases are
  archived on Zenodo as new versions of 10.5281/zenodo.23908116.
- README reorganized for general use; the quick start now runs `ae/stampede/fetch.sh` before
  building, since the Stampede baseline is not included.
- Figure and table numbers follow the published paper, in which the former supplementary
  figures are part of the main text: speedups are Figs. 2-10, switching is Fig. 11, and Tables
  3-4 hold the fiber sweep and the per-configuration switch gain. `data/speedup.csv`'s `figure`
  column uses the new numbers; `data/table4.csv` gains `seq_ratio` and `switch_gain`.
- `ae/paper_figures.py` draws Fig. 11 (`switch_all_tm`) and uses the paper's colors and marker
  sizes.
- `.gitignore` no longer hides `Logs/all_builder.sh` and
  `orig/switch-stamp/common/Defines.common.mk`, which the STAMP baseline needs to build.

## 1.0 (2026-11-09)

The version evaluated in the PPoPP 2027 artifact evaluation
([10.5281/zenodo.23908117](https://doi.org/10.5281/zenodo.23908117)).
