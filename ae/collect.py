#!/usr/bin/env python3
"""Reduce raw run logs (results/raw/...) to CSV tables.

    python3 ae/collect.py [--results DIR]

Writes
  results/runs.csv     one row per run: time, aborts, commits, set sizes, validation
  results/summary.csv  one row per (size, series, config, cores): mean/stdev of
                       time and aborts, and speedup over sequential STAMP
                       (mean stamp-seq time for the same config and input size),
                       i.e. the same definition as the paper's figures.

Only the Python 3 standard library is used.
"""
import argparse, csv, os, re, statistics, sys
from collections import defaultdict

# Which time line is the "execution time" of each benchmark.
# ssca2 prints several timers; SSCA2_TIMER selects one (see README, Claims).
SSCA2_TIMER = os.environ.get("SSCA2_TIMER", "kernel1")   # kernel1 | all

TIME_PATTERNS = [
    re.compile(r"^Time\s*=\s*([0-9.eE+-]+)"),              # genome, intruder, vacation
    re.compile(r"^Time:\s*([0-9.eE+-]+)\s*seconds"),        # kmeans
    re.compile(r"^Elapsed time\s*=\s*([0-9.eE+-]+)"),       # labyrinth, yada (Stampede intruder)
]
SSCA2_PATTERNS = {
    "kernel1": re.compile(r"^Time taken for kernel 1 is\s+([0-9.]+)"),
    "all":     re.compile(r"^Time taken for all is\s+([0-9.]+)"),
}
FIRST_INT = {
    "commits": re.compile(r"^#commits\s*:\s*(\d+)"),
    "aborts":  re.compile(r"^#aborts\s*:\s*(\d+)"),
    "htm_commits": re.compile(r"^\s*# HTMEnd_Tx\s*:\s*(\d+)"),
    "htm_fallback": re.compile(r"^\s*# HTMEnd_LOCK\s*:\s*(\d+)"),
    "htm_conflict": re.compile(r"^\s*# HTM(?:Tx)?Conflict\s*:\s*(\d+)"),
    "htm_capacity": re.compile(r"^\s*# HTMCapacity\s*:\s*(\d+)"),
    "tx_total": re.compile(r"^Total Tx :\s*(\d+)"),
}
FIRST_FLOAT = {
    "avg_read_set":  re.compile(r"^Average Read set :\s*([0-9.]+)"),
    "avg_write_set": re.compile(r"^Average Write set :\s*([0-9.]+)"),
    "tx_time_s":     re.compile(r"^Total Tx Time\(second\) :\s*([0-9.]+)"),
}
# Built-in self-checks of the STAMP programs (kmeans and ssca2 have none).
VALID = {
    "genome":   (re.compile(r"^Sequence matches gene: yes"), re.compile(r"^Sequence matches gene: no")),
    "labyrinth": (re.compile(r"^Verification passed"), re.compile(r"^Verification failed")),
    "yada":     (re.compile(r"^Final mesh is valid"), re.compile(r"^Final mesh is not valid")),
    "vacation": (re.compile(r"^Checking tables\.\.\. done"), None),
}


def parse(path):
    meta, rec = {}, {}
    lines = open(path, errors="replace").read().splitlines()
    for ln in lines:
        if ln.startswith("AE-"):
            k, _, v = ln.partition(" ")
            meta[k[3:].lower()] = v
    cfg = meta.get("config", "")
    bench = cfg.split("_")[0]
    pats = [SSCA2_PATTERNS[SSCA2_TIMER]] if bench == "ssca2" else TIME_PATTERNS
    for ln in lines:
        if "time" not in rec:
            for p in pats:
                m = p.match(ln)
                if m:
                    rec["time"] = float(m.group(1)); break
        for k, p in FIRST_INT.items():
            if k not in rec:
                m = p.match(ln)
                if m: rec[k] = int(m.group(1))
        for k, p in FIRST_FLOAT.items():
            if k not in rec:
                m = p.match(ln)
                if m: rec[k] = float(m.group(1))
    if bench == "intruder":
        a = re.search(r"^Num attack\s*=\s*(\d+)", "\n".join(lines), re.M)
        f = re.search(r"^Num found\s*=\s*(\d+)", "\n".join(lines), re.M)
        rec["valid"] = "yes" if a and f and a.group(1) == f.group(1) else "no"
    elif bench in VALID:
        ok, bad = VALID[bench]
        if any(bad and bad.match(l) for l in lines): rec["valid"] = "no"
        elif any(ok.match(l) for l in lines): rec["valid"] = "yes"
        else: rec["valid"] = "no"
    else:
        rec["valid"] = "n/a"
    rec["exit"] = meta.get("exit", "missing")
    return meta, rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results"))
    a = ap.parse_args()
    raw = os.path.join(a.results, "raw")
    if not os.path.isdir(raw):
        sys.exit(f"no raw results under {raw}; run ae/run.sh first")

    rows = []
    for size in sorted(os.listdir(raw)):
        for series in sorted(os.listdir(os.path.join(raw, size))):
            for cfg in sorted(os.listdir(os.path.join(raw, size, series))):
                d = os.path.join(raw, size, series, cfg)
                for fn in sorted(os.listdir(d)):
                    m = re.match(r"c(\d+)_r(\d+)\.txt$", fn)
                    if not m: continue
                    meta, rec = parse(os.path.join(d, fn))
                    rows.append(dict(size=size, series=series, config=cfg,
                                     cores=int(m.group(1)), rep=int(m.group(2)), **rec))

    cols = ["size", "series", "config", "cores", "rep", "exit", "valid", "time",
            "commits", "aborts", "htm_commits", "htm_fallback", "htm_conflict", "htm_capacity",
            "tx_total", "avg_read_set", "avg_write_set", "tx_time_s"]
    with open(os.path.join(a.results, "runs.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, cols); w.writeheader()
        for r in rows: w.writerow({c: r.get(c, "") for c in cols})

    # ---- summary -------------------------------------------------------
    good = [r for r in rows if r["exit"] == "0" and "time" in r]
    groups = defaultdict(list)
    for r in good:
        groups[(r["size"], r["series"], r["config"], r["cores"])].append(r)
    seq = {}
    for (size, series, cfg, _), rs in groups.items():
        if series == "stamp-seq":
            seq[(size, cfg)] = statistics.mean(x["time"] for x in rs)

    scols = ["size", "series", "config", "cores", "n", "time_mean", "time_stdev",
             "speedup", "aborts_mean", "commits_mean", "abort_commit_ratio",
             "avg_read_set", "avg_write_set", "invalid_runs"]
    out = []
    for key in sorted(groups, key=lambda k: (k[0], k[1], k[2], k[3])):
        size, series, cfg, n = key
        rs = groups[key]
        t = [x["time"] for x in rs]
        tm = statistics.mean(t)
        row = dict(size=size, series=series, config=cfg, cores=n, n=len(rs),
                   time_mean=f"{tm:.6g}",
                   time_stdev=f"{statistics.stdev(t):.3g}" if len(t) > 1 else "",
                   speedup=f"{seq[(size, cfg)] / tm:.4g}" if (size, cfg) in seq and tm > 0 else "",
                   invalid_runs=sum(1 for x in rs if x.get("valid") == "no"))
        for k, src in (("aborts_mean", "aborts"), ("commits_mean", "commits"),
                       ("avg_read_set", "avg_read_set"), ("avg_write_set", "avg_write_set")):
            v = [x[src] for x in rs if src in x]
            if v: row[k] = f"{statistics.mean(v):.6g}"
        if row.get("aborts_mean") and row.get("commits_mean") and float(row["commits_mean"]) > 0:
            row["abort_commit_ratio"] = f"{float(row['aborts_mean']) / float(row['commits_mean']):.4g}"
        out.append(row)
    with open(os.path.join(a.results, "summary.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, scols); w.writeheader()
        for r in out: w.writerow({c: r.get(c, "") for c in scols})

    bad = [r for r in rows if r["exit"] != "0" or r.get("valid") == "no"]
    print(f"{len(rows)} runs parsed, {len(out)} summary rows, {len(bad)} failed/invalid runs")
    for r in bad[:20]:
        print(f"  ! {r['size']}/{r['series']}/{r['config']} c{r['cores']} r{r['rep']}: exit={r['exit']} valid={r.get('valid')}")
    print(f"wrote {a.results}/runs.csv and {a.results}/summary.csv")


if __name__ == "__main__":
    main()
