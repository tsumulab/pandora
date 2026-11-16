#!/usr/bin/env python3
"""Plot speedup curves from results/summary.csv, overlaid on the paper's data.

    python3 ae/plot.py [--results DIR] [--size large|small] [--no-paper]

For every configuration, one PNG per TM system is written to results/figures/,
mirroring the panels of Figures 2-10 (and Fig. 11 for the
switch/non-switch pairs). Solid lines + markers are your measurements;
dashed lines are the values plotted in the paper (data/speedup.csv). Absolute
values depend on the machine; what should match is the shape and ordering of
the curves (see README, "Claims").
"""
import argparse, csv, os, sys
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit("matplotlib is required (it is installed in the provided Docker image)")

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

PANEL = {  # paper panel name -> series of ours
    "TinySTM": ("pandora-tinystm", "stamp-tinystm", "stampede-tinystm", "pandora-tinystm-noswitch"),
    "TL2":     ("pandora-tl2", "stamp-tl2", None, "pandora-tl2-noswitch"),
    "NOrec":   ("pandora-norec", "stamp-norec", None, "pandora-norec-noswitch"),
    "SwissTM": ("pandora-swisstm", "stamp-swisstm", None, "pandora-swisstm-noswitch"),
    "HTM":     ("pandora-rtm", "stamp-rtm", "stampede-htm", "pandora-rtm-noswitch"),
}
STYLE = {  # label, color, marker
    "PANDORA/TM":          ("tab:red", "o"),
    "PANDORA/TM (no switch)": ("tab:pink", "v"),
    "PANDORA/fine-lock":   ("tab:green", "s"),
    "PANDORA/coarse-lock": ("tab:olive", "D"),
    "STAMP/TM":            ("tab:blue", "^"),
    "Stampede/TM":         ("tab:purple", "x"),
}


def split_cfg(cfg):
    if "_" in cfg:
        w, lvl = cfg.split("_"); return w, lvl
    return cfg, "na"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", default=os.path.join(ROOT, "results"))
    ap.add_argument("--size", default="large")
    ap.add_argument("--no-paper", action="store_true")
    a = ap.parse_args()

    mine = defaultdict(dict)       # (series, config) -> {cores: speedup}
    with open(os.path.join(a.results, "summary.csv")) as f:
        for r in csv.DictReader(f):
            if r["size"] == a.size and r["speedup"]:
                mine[(r["series"], r["config"])][int(r["cores"])] = float(r["speedup"])

    paper = defaultdict(dict)      # (panel, label, workload, level) -> {cores: speedup}
    if not a.no_paper and a.size == "large":
        with open(os.path.join(ROOT, "data", "speedup.csv")) as f:
            for r in csv.DictReader(f):
                label = r["series"] + (" (no switch)" if r["switch"] == "off" else "")
                paper[(r["tm_system"], label, r["workload"], r["level"])][int(r["cores"])] = float(r["speedup"])

    outdir = os.path.join(a.results, "figures"); os.makedirs(outdir, exist_ok=True)
    configs = sorted({c for (_, c) in mine})
    n = 0
    for cfg in configs:
        w, lvl = split_cfg(cfg)
        for panel, (pt, st, sd, ns) in PANEL.items():
            lines = [("PANDORA/TM", pt), ("PANDORA/TM (no switch)", ns),
                     ("PANDORA/fine-lock", "pandora-fine-lock"),
                     ("PANDORA/coarse-lock", "pandora-coarse-lock"),
                     ("STAMP/TM", st), ("Stampede/TM", sd)]
            if not mine.get((pt, cfg)):
                continue     # this TM system was not run for this config
            fig, ax = plt.subplots(figsize=(4.2, 3.2))
            for label, s in lines:
                col, mk = STYLE[label]
                pts = mine.get((s, cfg)) if s else None
                if pts:
                    xs = sorted(pts); ax.plot(xs, [pts[x] for x in xs], marker=mk, color=col, label=label, lw=1.6)
                pp = paper.get((panel, label, w, lvl))
                if pp:
                    xs = sorted(pp); ax.plot(xs, [pp[x] for x in xs], ls="--", color=col, lw=1, alpha=.55,
                                             label=None if pts else label + " (paper)")
            ax.set_title(f"{cfg} -- {panel}" + ("" if a.no_paper or a.size != "large" else "\nsolid: this run, dashed: paper"),
                         fontsize=9)
            ax.set_xlabel("cores"); ax.set_ylabel("speedup over sequential STAMP")
            ax.grid(alpha=.3); ax.legend(fontsize=6.5, loc="upper left")
            fig.tight_layout()
            fig.savefig(os.path.join(outdir, f"{a.size}_{cfg}_{panel}.png"), dpi=130)
            plt.close(fig); n += 1
    print(f"wrote {n} figures to {outdir}")


if __name__ == "__main__":
    main()
