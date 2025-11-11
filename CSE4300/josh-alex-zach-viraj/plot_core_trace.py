#!/usr/bin/env python3
"""
timeline_plot.py
Usage:
    python timeline_plot.py input.txt [output.png]

Input format example:
Core 0: [T1, T1, T1, T1, T2, T2, T3, T3]
Core 1: [T2, T2, T2, T1, T1, T1, T1]
"""

import sys
import re
from collections import OrderedDict
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

def parse_line(line):
    """
    Returns (label, timeline_list_of_tokens_as_str)
    Accepts tokens like T1 without quotes.
    """
    if ":" not in line:
        return None
    label, rest = line.split(":", 1)
    label = label.strip()
    m = re.search(r"\[(.*)\]", rest)
    if not m:
        return None
    inside = m.group(1).strip()
    if not inside:
        return label, []
    # split on commas, allow spaces
    tokens = [tok.strip() for tok in inside.split(",")]
    # allow tokens like T1 or 'T1' or "T1"
    cleaned = []
    for t in tokens:
        if t.startswith("'") and t.endswith("'"):
            cleaned.append(t[1:-1])
        elif t.startswith('"') and t.endswith('"'):
            cleaned.append(t[1:-1])
        else:
            cleaned.append(t)
    return label, cleaned

def compress_runs(seq):
    """Turn [T1, T1, T2, T2, T2] into [(T1,0,2),(T2,2,3)]."""
    if not seq:
        return []
    spans = []
    cur = seq[0]
    start = 0
    length = 1
    for i in range(1, len(seq)):
        if seq[i] == cur:
            length += 1
        else:
            spans.append((cur, start, length))
            cur = seq[i]
            start = i
            length = 1
    spans.append((cur, start, length))
    return spans

def read_timelines(path):
    """
    Returns an ordered dict: label -> list_of_tokens
    Keeps input order of cores.
    """
    cores = OrderedDict()
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parsed = parse_line(line)
            if parsed:
                label, seq = parsed
                cores[label] = seq
    return cores

def plot_timelines(cores, title="CPU Run Timeline"):
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch

    # gather unique task names
    tasks = []
    for seq in cores.values():
        for t in seq:
            if t not in tasks:
                tasks.append(t)

    cmap = plt.cm.get_cmap("tab20", max(20, len(tasks)))
    color_map = {t: cmap(i % cmap.N) for i, t in enumerate(tasks)}

    # constrained_layout helps prevent overlaps automatically
    fig, ax = plt.subplots(
        figsize=(12, 0.8 + 0.7 * max(1, len(cores))),
        constrained_layout=True
    )
    lane_height = 0.6
    lane_gap = 0.2
    yticks, ylabels = [], []

    max_len = max((len(seq) for seq in cores.values()), default=0)

    def compress_runs(seq):
        if not seq:
            return []
        spans, cur, start, length = [], seq[0], 0, 1
        for i in range(1, len(seq)):
            if seq[i] == cur:
                length += 1
            else:
                spans.append((cur, start, length))
                cur, start, length = seq[i], i, 1
        spans.append((cur, start, length))
        return spans

    for idx, (label, seq) in enumerate(cores.items()):
        spans = compress_runs(seq)
        ybase = idx * (lane_height + lane_gap)
        for task, start, length in spans:
            ax.broken_barh([(start, length)], (ybase, lane_height),
                           facecolors=color_map[task], edgecolors="none")
        yticks.append(ybase + lane_height / 2)
        ylabels.append(label)

    ax.set_ylim(0, len(cores) * (lane_height + lane_gap))
    ax.set_xlim(0, max_len)
    ax.set_xlabel("Tick")
    ax.set_yticks(yticks)
    ax.set_yticklabels(ylabels)
    ax.set_title(title)
    ax.grid(True, axis="x", linestyle=":", linewidth=0.8)
    ax.set_xticks(range(0, max_len + 1))

    # put legend above the axes so it cannot collide with the x label
    handles = [Patch(facecolor=color_map[t], label=t) for t in tasks]
    ax.legend(handles=handles, title="Task",
            loc="center left", bbox_to_anchor=(1.02, 0.5), frameon=False)

    plt.show()

def main():
    cores = read_timelines("core_trace.txt")  # fixed file name
    if not cores:
        print("No valid lines found in input")
        sys.exit(1)
    plot_timelines(cores)

if __name__ == "__main__":
    main()
