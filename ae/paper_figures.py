#!/usr/bin/env python3
"""Draw the paper's speedup figures, in the paper's style, from AE results.

    python3 ae/paper_figures.py [--results DIR] [--out DIR] [--size large]
                                [--format pdf,svg,png] [--legend paper|all|none]
                                [--width-in 7.0] [--ymax genome=25 ...]

Input:  <results>/summary.csv written by ae/collect.py
        (columns used: size, series, config, cores, speedup, time_mean),
        or, with --paper-csv, the artifact's data/speedup.csv (the plotted values).
Overlapping curves (one hidden under another) are listed on the terminal,
so that the caption can say so.

Examples (from the artifact root, also inside Docker):
    python3 ae/paper_figures.py --paper-csv data/speedup.csv   # the paper's figures,
                                         # from the plotted values -> results/paper-figures-data/
    python3 ae/paper_figures.py                                # the same figures from your
                                         # runs (results/summary.csv) -> results/paper-figures/

Output: <out>/speedup_<config>.<fmt>   Figs. 2-4 and S1-S9 (five TM panels)
        <out>/switch_tinystm.<fmt>     Fig. 5 (kmeans_high, ssca2; single column)
        <out>/switch_tinystm_all.<fmt> Fig. S12 (all nine configurations)
        <out>/switch_<config>_all_tm.<fmt>  Figs. S10/S11 (kmeans_high, ssca2;
                                       switch vs non-switch on all five TM systems)
        <out>/legend_speedup.<fmt>     the shared legend on its own

The look reproduces the Excel/Illustrator figures of the submitted paper:
same series, colours, markers, dash patterns, panel order, horizontal grid
only, and Excel-like automatic y scaling (override with --ymax).
Requires matplotlib (installed in the Docker image).
"""
import argparse, csv, math, os, sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

# --------------------------------------------------------------------------
# Style (taken from the submitted figures)
# --------------------------------------------------------------------------
FONT = ["Arial", "Liberation Sans", "Helvetica", "DejaVu Sans"]
FS_TICK, FS_TITLE, FS_LEGEND = 8, 9, 8
LW, MS = 1.4, 3.6
DASH = (0, (2.0, 1.1))
GRID, AXIS, TEXT = "#D9D9D9", "#BFBFBF", "#404040"

RED, LIGHTBLUE, NAVY = "#E8262D", "#2E9BD6", "#1F4E79"
ORANGE, PURPLE = "#F5A623", "#7B3F9E"

# label: (color, linestyle, marker)
STYLE = {
    "PANDORA/TM":                  (RED,       "-",  "D"),
    "PANDORA/fine-grained lock":   (LIGHTBLUE, DASH, "D"),
    "PANDORA/coarse-grained lock": (NAVY,      DASH, "D"),
    "Stampede/TM":                 (ORANGE,    "-",  "^"),
    "STAMP/TM":                    (PURPLE,    "-",  "o"),
    "switch":                      (RED,       "-",  "D"),
    "non-switch":                  (RED,       DASH, "D"),
    "fine lock":                   (LIGHTBLUE, DASH, "D"),
}
# Lock series get hollow markers drawn above the TM lines, so that a lock curve
# lying on top of a TM curve (e.g. HTM vs coarse lock) stays visible: the filled
# TM marker shows through the hollow lock marker. Off by default (as in the
# paper); --hollow-locks turns it on.
LOCKS = {"PANDORA/fine-grained lock", "PANDORA/coarse-grained lock", "fine lock"}
HOLLOW_LOCKS = False
HOLLOW_SCALE, HOLLOW_EDGE = 1.35, 1.0

SPEEDUP_ORDER = ["PANDORA/TM", "PANDORA/fine-grained lock", "PANDORA/coarse-grained lock",
                 "Stampede/TM", "STAMP/TM"]
SWITCH_ORDER = ["switch", "non-switch", "fine lock"]

# Panels of Figs. 2-4 / S1-S9: (title, AE suffix of pandora-/stamp-, Stampede series)
TM_PANELS = [
    ("TL2",       "tl2",     None),
    ("TinySTM",   "tinystm", "stampede-tinystm"),
    ("NOrec",     "norec",   None),
    ("SwissTM",   "swisstm", None),
    ("HTM (TSX)", "rtm",     "stampede-htm"),
]
CONFIGS = ["genome", "intruder", "kmeans_high", "kmeans_low", "labyrinth",
           "ssca2", "vacation_high", "vacation_low", "yada"]
LEGEND_ON = {"genome"}          # paper: legend drawn on Fig. 2 (genome) only


def series_for_panel(tm, stampede):
    """AE series name -> legend label, for one TM panel."""
    m = {f"pandora-{tm}": "PANDORA/TM",
         "pandora-fine-lock": "PANDORA/fine-grained lock",
         "pandora-coarse-lock": "PANDORA/coarse-grained lock",
         f"stamp-{tm}": "STAMP/TM"}
    if stampede:
        m[stampede] = "Stampede/TM"
    return m


def series_for_switch(tm):
    return {f"pandora-{tm}": "switch", f"pandora-{tm}-noswitch": "non-switch",
            "pandora-fine-lock": "fine lock"}


# --------------------------------------------------------------------------
# Data
# --------------------------------------------------------------------------
PAPER_TM = {"TL2": "tl2", "TinySTM": "tinystm", "NOrec": "norec",
            "SwissTM": "swisstm", "HTM": "rtm"}


def load_paper_csv(path):
    """data/speedup.csv of the artifact (figure,tm_system,workload,level,series,
    switch,cores,speedup) -> the same dict as load()."""
    d = {}
    for r in csv.DictReader(open(path)):
        tm = PAPER_TM[r["tm_system"]]
        cfg = r["workload"] + ("" if r["level"] == "na" else "_" + r["level"])
        ser = r["series"]
        if ser == "PANDORA/TM":
            name = f"pandora-{tm}" + ("-noswitch" if r["switch"] == "off" else "")
        elif ser == "PANDORA/fine-lock":
            name = "pandora-fine-lock"
        elif ser == "PANDORA/coarse-lock":
            name = "pandora-coarse-lock"
        elif ser == "STAMP/TM":
            name = f"stamp-{tm}"
        elif ser == "Stampede/TM":
            name = {"tinystm": "stampede-tinystm", "rtm": "stampede-htm"}.get(tm)
        else:
            name = None
        if not name:
            continue
        # lock rows are repeated under every tm_system; keep one copy
        d.setdefault((name, cfg), {})[int(r["cores"])] = float(r["speedup"])
    return {k: sorted(v.items()) for k, v in d.items()}


def load(path, size):
    """{(series, config): [(cores, speedup), ...]} sorted by cores."""
    rows = [r for r in csv.DictReader(open(path)) if r.get("size", size) == size]
    seq = {r["config"]: float(r["time_mean"]) for r in rows
           if r["series"] == "stamp-seq" and r.get("time_mean")}
    d = defaultdict(list)
    for r in rows:
        if r["series"] == "stamp-seq":
            continue
        s = r.get("speedup", "")
        if s:
            v = float(s)
        elif r.get("time_mean") and r["config"] in seq:
            v = seq[r["config"]] / float(r["time_mean"])
        else:
            continue
        d[(r["series"], r["config"])].append((int(r["cores"]), v))
    for k in d:
        d[k].sort()
    return d


# --------------------------------------------------------------------------
# Axes helpers
# --------------------------------------------------------------------------
def nice_ymax(vmax):
    """Excel-like automatic maximum: 5% headroom, step in {1,2,5}x10^k, <= 7 steps."""
    if vmax <= 0:
        return 1.0, 0.2
    target = vmax * 1.05
    k = math.floor(math.log10(target)) - 1
    while True:
        for m in (1, 2, 5):
            step = m * 10 ** k
            n = math.ceil(target / step - 1e-9)
            if n <= 7:
                return n * step, step
        k += 1


def step_for(ymax):
    """Tick step for a user-fixed y maximum: divides it into 3-7 intervals,
    preferring 1/2/5 x 10^k, then 4 and 2.5 (e.g. 16 -> 4, 12 -> 2, 40 -> 10)."""
    k0 = math.floor(math.log10(ymax)) - 2
    for m in (1, 2, 5, 4, 2.5):
        for k in range(k0, k0 + 4):
            st = m * 10 ** k
            n = ymax / st
            if abs(n - round(n)) < 1e-9 and 3 <= round(n) <= 7:
                return st
    return nice_ymax(ymax)[1]


def style_axes(ax, title, ymax, ystep, xmax):
    ax.set_title(title, fontsize=FS_TITLE, color=TEXT, pad=3)
    ax.set_xlim(0, xmax)
    ax.set_ylim(0, ymax)
    xstep = 8 if xmax <= 48 else 16
    ax.set_xticks(range(0, int(xmax) + 1, xstep))
    nt = int(round(ymax / ystep))
    ax.set_yticks([i * ystep for i in range(nt + 1)])
    ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(
        lambda v, _: f"{v:g}"))
    ax.tick_params(length=0, labelsize=FS_TICK, colors=TEXT, pad=2)
    ax.grid(axis="y", color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)
    for s in ("top", "right", "left"):
        ax.spines[s].set_visible(False)
    ax.spines["bottom"].set_color(AXIS)
    ax.spines["bottom"].set_linewidth(0.8)


def marker_kw(label, legend=False):
    c, ls, mk = STYLE[label]
    size = MS * (1.2 if mk == "^" else 1.0) + (1.5 if legend else 0)
    if HOLLOW_LOCKS and label in LOCKS:
        return dict(marker=mk, markersize=size * HOLLOW_SCALE, markerfacecolor="none",
                    markeredgecolor=c, markeredgewidth=HOLLOW_EDGE)
    return dict(marker=mk, markersize=size, markerfacecolor=c, markeredgewidth=0)


def draw(ax, pts, label):
    c, ls, mk = STYLE[label]
    xs, ys = zip(*pts)
    z = 4 if (HOLLOW_LOCKS and label in LOCKS) else 3
    ax.plot(xs, ys, color=c, linestyle=ls, linewidth=LW, label=label,
            clip_on=False, zorder=z, **marker_kw(label))


def handle(label):
    c, ls, mk = STYLE[label]
    return Line2D([], [], color=c, linestyle=ls, linewidth=LW + 0.6,
                  **marker_kw(label, legend=True))


def row_major(labels, ncol):
    """matplotlib fills legends column-first; reorder so rows read left-to-right."""
    nrow = math.ceil(len(labels) / ncol)
    grid = [labels[r * ncol:(r + 1) * ncol] for r in range(nrow)]
    return [grid[r][c] for c in range(ncol) for r in range(nrow) if c < len(grid[r])]


def add_legend(fig, labels, ncol, where="above", handlelength=4.5):
    """Legend outside the panels: above the top row or below the bottom row."""
    order = row_major(labels, ncol)
    top = max(ax.get_position().y1 for ax in fig.axes)
    bot = min(ax.get_position().y0 for ax in fig.axes)
    if where == "above":
        loc, y = "lower center", top + 0.13 / fig.get_figheight() + 0.02
    else:
        loc, y = "upper center", bot - 0.22 / fig.get_figheight()
    leg = fig.legend([handle(l) for l in order], order, loc=loc,
                     bbox_to_anchor=(0.5, y), ncol=ncol, frameon=False,
                     fontsize=FS_LEGEND, handlelength=handlelength,
                     columnspacing=2.0, handletextpad=0.6)
    for t in leg.get_texts():       # (labelcolor= needs matplotlib >= 3.5; the image has 3.1)
        t.set_color(TEXT)
    leg._ae_anchor_y = y            # figure fraction; fit_canvas() moves it


# figure numbers in the paper (Fig.) and in the supplementary material (S)
FIG_IDS = {"speedup_genome": "Fig2_S1", "speedup_kmeans_high": "Fig3_S2",
           "speedup_kmeans_low": "S3", "speedup_ssca2": "Fig4_S4",
           "speedup_intruder": "S5", "speedup_yada": "S6",
           "speedup_vacation_high": "S7", "speedup_vacation_low": "S8",
           "speedup_labyrinth": "S9", "switch_kmeans_high_all_tm": "S10",
           "switch_ssca2_all_tm": "S11", "switch_tinystm_all": "S12",
           "switch_tinystm": "Fig5"}
NAME_BY_FIGURE = False


def fit_canvas(fig):
    """Grow the figure so that legends placed outside the panels lie on the canvas.
    matplotlib 3.1 (the Docker image) clips anything off the canvas even with a
    bounding box that includes it; newer versions do not need this."""
    from matplotlib.transforms import Bbox
    fig.canvas.draw()               # legend extents are only valid after a draw
    r = fig.canvas.get_renderer()
    W, H = fig.get_size_inches()
    boxes = [l.get_window_extent(r).transformed(fig.dpi_scale_trans.inverted())
             for l in fig.legends]
    if not boxes:
        return
    bb = Bbox.union(boxes)
    db, dt = max(0.0, -bb.y0 + 0.1) + 0.02, max(0.0, bb.y1 + 0.1 - H) + 0.02
    if db <= 0.02 and dt <= 0.02:
        return
    nH = H + db + dt
    for ax in fig.axes:
        p = ax.get_position()
        ax.set_position([p.x0, (p.y0 * H + db) / nH, p.width, p.height * H / nH])
    fig.set_size_inches(W, nH)
    for l in fig.legends:
        y = getattr(l, "_ae_anchor_y", None)
        if y is not None:
            l.set_bbox_to_anchor((0.5, (y * H + db) / nH), transform=fig.transFigure)


def tight_bbox(fig, pad=0.01):
    """Bounding box (inches) of everything drawn, figure legends included: matplotlib
    3.1 (Ubuntu 20.04, the Docker image) leaves fig.legend() out of bbox_inches="tight"
    and clips it."""
    from matplotlib.transforms import Bbox
    fig.canvas.draw()
    r = fig.canvas.get_renderer()
    # matplotlib 3.1 under-reports a figure legend's extent by about one text line
    # when saving at a different dpi; pad legends generously
    boxes = [fig.get_tightbbox(r)] + [
        l.get_window_extent(r).transformed(fig.dpi_scale_trans.inverted()).padded(0.1)
        for l in fig.legends]
    return Bbox.union(boxes).padded(pad)


def save(fig, out, name, fmts):
    if NAME_BY_FIGURE and name in FIG_IDS:
        name = f"{FIG_IDS[name]}_{name}"
    fit_canvas(fig)
    for f in fmts:
        p = os.path.join(out, f"{name}.{f}")
        fig.savefig(p, bbox_inches=tight_bbox(fig), dpi=300)
    plt.close(fig)
    print("wrote", os.path.join(out, name) + "." + "|".join(fmts))


# --------------------------------------------------------------------------
# Figures
# --------------------------------------------------------------------------
def report_overlaps(name, title, lines, ymax, tol=0.03):
    """Print series that lie on top of each other (every common point within
    tol*ymax), i.e. curves a reader cannot see, for writing the caption."""
    for i, (la, pa) in enumerate(lines):
        for lb, pb in lines[i + 1:]:
            a, b = dict(pa), dict(pb)
            xs = sorted(set(a) & set(b))
            if xs and all(abs(a[x] - b[x]) <= tol * ymax for x in xs):
                print(f"  overlap  {name} / {title}: {la} ~ {lb} "
                      f"(max diff {max(abs(a[x]-b[x]) for x in xs):.2f})")


def fig_speedup(d, cfg, legend, width, ymax_override, xmax):
    panels = []
    for title, tm, stp in TM_PANELS:
        lines = [(lab, d[(s, cfg)]) for s, lab in series_for_panel(tm, stp).items()
                 if d.get((s, cfg))]
        lines.sort(key=lambda t: SPEEDUP_ORDER.index(t[0]))
        panels.append((title, lines))
    if not any(l for _, l in panels):
        print(f"skip speedup_{cfg}: no data", file=sys.stderr)
        return None
    vmax = max(v for _, ls in panels for _, pts in ls for _, v in pts)
    ymax, ystep = (ymax_override, step_for(ymax_override)) if ymax_override \
        else nice_ymax(vmax)
    h = 1.45 + (0.42 if legend else 0)
    fig, axes = plt.subplots(1, 5, figsize=(width, h))
    fig.subplots_adjust(wspace=0.32, left=0.03, right=0.995, bottom=0.12,
                        top=(1.45 - 0.22) / h)
    for ax, (title, lines) in zip(axes, panels):
        # draw back-to-front so PANDORA/TM is on top, as in the paper
        for lab, pts in reversed(lines):
            draw(ax, pts, lab)
        style_axes(ax, title, ymax, ystep, xmax)
        report_overlaps(f"speedup_{cfg}", title, lines, ymax)
        missing = [t for t in ("PANDORA/TM", "STAMP/TM") if t not in dict(lines)]
        if missing:
            print(f"  speedup_{cfg} / {title}: no data for {', '.join(missing)}",
                  file=sys.stderr)
    if legend:
        add_legend(fig, SPEEDUP_ORDER, 3, "above")
    return fig


def fig_switch(d, rows, tm, ncols, width, panel_h, ymax_override, xmax,
               legend_where="above", handlelength=4.5):
    """rows: list of rows, each a list of configurations (left-aligned)."""
    m = series_for_switch(tm)
    nrows = len(rows)
    leg_h = 0.3
    h = panel_h * nrows + leg_h
    fig = plt.figure(figsize=(width, h))
    gs = fig.add_gridspec(nrows, ncols, wspace=0.38, hspace=0.55,
                          left=0.04, right=0.995, bottom=0.08 / nrows,
                          top=(h - leg_h) / h)
    any_data = False
    for ri, row in enumerate(rows):
        for ci, cfg in enumerate(row):
            ax = fig.add_subplot(gs[ri, ci])
            lines = [(lab, d[(s, cfg)]) for s, lab in m.items() if d.get((s, cfg))]
            lines.sort(key=lambda t: SWITCH_ORDER.index(t[0]))
            any_data |= bool(lines)
            vmax = max((v for _, pts in lines for _, v in pts), default=1)
            ov = ymax_override.get(cfg)
            ymax, ystep = (ov, step_for(ov)) if ov else nice_ymax(vmax)
            for lab, pts in reversed(lines):
                draw(ax, pts, lab)
            style_axes(ax, cfg, ymax, ystep, xmax)
            report_overlaps(f"switch_{tm}", cfg, lines, ymax)
    if not any_data:
        plt.close(fig)
        print(f"skip switch figure ({tm}): no data", file=sys.stderr)
        return None
    add_legend(fig, SWITCH_ORDER, 3, legend_where, handlelength)
    return fig


def fig_switch_tms(d, cfg, width, ymax_override, xmax):
    """Figs. S10/S11: switch vs non-switch on every TM system, one workload."""
    panels = []
    for title, tm, _ in TM_PANELS:
        lines = [(lab, d[(s, cfg)]) for s, lab in series_for_switch(tm).items()
                 if d.get((s, cfg))]
        lines.sort(key=lambda t: SWITCH_ORDER.index(t[0]))
        panels.append((title, lines))
    if not any(any(l == "non-switch" for l, _ in ls) for _, ls in panels):
        return None
    vmax = max(v for _, ls in panels for _, pts in ls for _, v in pts)
    ymax, ystep = (ymax_override, step_for(ymax_override)) if ymax_override \
        else nice_ymax(vmax)
    h = 1.45 + 0.3
    fig, axes = plt.subplots(1, 5, figsize=(width, h))
    fig.subplots_adjust(wspace=0.32, left=0.03, right=0.995, bottom=0.12,
                        top=(1.45 - 0.22) / h)
    for ax, (title, lines) in zip(axes, panels):
        for lab, pts in reversed(lines):
            draw(ax, pts, lab)
        style_axes(ax, title, ymax, ystep, xmax)
        report_overlaps(f"switch_{cfg}_all_tm", title, lines, ymax)
    add_legend(fig, SWITCH_ORDER, 3, "above")
    return fig


def fig_legend_only(width):
    fig = plt.figure(figsize=(width, 0.4))
    ax = fig.add_axes([0, 0, 1, 0.01]); ax.set_axis_off()
    add_legend(fig, SPEEDUP_ORDER, 3, "above")
    return fig


# --------------------------------------------------------------------------
def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--results", default=os.path.join(here, "..", "results"))
    ap.add_argument("--paper-csv", default=None, metavar="speedup.csv",
                    help="read the artifact's data/speedup.csv instead of summary.csv")
    ap.add_argument("--out", default=None, help="default: <results>/paper-figures")
    ap.add_argument("--size", default="large", choices=["large", "small"])
    ap.add_argument("--format", default="pdf,svg",
                    help="comma list of pdf, svg, png (default pdf,svg)")
    ap.add_argument("--legend", default="paper", choices=["paper", "all", "none"],
                    help="paper: legend on genome and switch figures only")
    ap.add_argument("--width-in", type=float, default=7.0,
                    help="two-column figure width in inches (set to \\textwidth)")
    ap.add_argument("--column-in", type=float, default=3.33,
                    help="one-column figure width in inches (set to \\columnwidth)")
    ap.add_argument("--xmax", type=int, default=None,
                    help="x-axis maximum (default: largest core count, at least 40)")
    ap.add_argument("--switch-tm", default="tinystm")
    ap.add_argument("--ymax", nargs="*", default=[], metavar="CONFIG=VALUE",
                    help="fix the y maximum, e.g. genome=25 kmeans_high=40")
    ap.add_argument("--hollow-locks", action="store_true",
                    help="hollow lock markers drawn above TM lines")
    ap.add_argument("--name-by-figure", action="store_true",
                    help="prefix file names with the figure number (Fig2_S1_..., S3_..., S10_...)")
    ap.add_argument("--only", nargs="*", default=None, metavar="CONFIG",
                    help="restrict speedup figures to these configurations")
    a = ap.parse_args()

    global HOLLOW_LOCKS, NAME_BY_FIGURE
    HOLLOW_LOCKS = a.hollow_locks
    NAME_BY_FIGURE = a.name_by_figure
    matplotlib.rcParams.update({
        "font.family": "sans-serif", "font.sans-serif": FONT,
        "pdf.fonttype": 42, "ps.fonttype": 42, "svg.fonttype": "none",
    })
    if a.paper_csv:
        d = load_paper_csv(a.paper_csv)
        out = a.out or os.path.normpath(os.path.join(here, "..", "results", "paper-figures-data"))
    else:
        path = os.path.join(a.results, "summary.csv")
        if not os.path.exists(path):
            sys.exit(f"{path} not found; run ae/collect.py first")
        d = load(path, a.size)
        if not d:
            sys.exit(f"no '{a.size}' rows in {path}")
        out = a.out or os.path.join(a.results, "paper-figures")
    os.makedirs(out, exist_ok=True)
    fmts = [f.strip() for f in a.format.split(",") if f.strip()]
    yov = {k: float(v) for k, v in (x.split("=", 1) for x in a.ymax)}
    xmax = a.xmax or max(40, max(c for pts in d.values() for c, _ in pts))

    for cfg in (a.only or CONFIGS):
        leg = a.legend == "all" or (a.legend == "paper" and cfg in LEGEND_ON)
        fig = fig_speedup(d, cfg, leg, a.width_in, yov.get(cfg), xmax)
        if fig:
            save(fig, out, f"speedup_{cfg}", fmts)
    save(fig_legend_only(a.width_in), out, "legend_speedup", fmts)

    tm = a.switch_tm
    fig = fig_switch(d, [["kmeans_high", "ssca2"]], tm, 2, a.column_in, 1.4, yov, xmax,
                     legend_where="below", handlelength=3.0)
    if fig:
        save(fig, out, f"switch_{tm}", fmts)
    for cfg in ("kmeans_high", "ssca2"):
        fig = fig_switch_tms(d, cfg, a.width_in, yov.get(cfg), xmax)
        if fig:
            save(fig, out, f"switch_{cfg}_all_tm", fmts)
    s12 = [["genome", "kmeans_high", "kmeans_low", "ssca2"],
           ["intruder", "yada", "vacation_high", "vacation_low", "labyrinth"]]
    fig = fig_switch(d, s12, tm, 5, a.width_in, 1.45, yov, xmax)
    if fig:
        save(fig, out, f"switch_{tm}_all", fmts)


if __name__ == "__main__":
    main()
