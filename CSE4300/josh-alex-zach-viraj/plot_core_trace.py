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
import math
import random
from collections import OrderedDict
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib import ticker
from matplotlib import ticker, colors as mcolors

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

def plot_timelines(cores, title="CPU Run Timeline", seed=None):
    rng = random.Random(seed)

    # --- Gather basics ---
    ncores = len(cores)
    max_len = max((len(seq) for seq in cores.values()), default=0)

    # Normalize tokens: treat 'IDLE' or '-' as a single idle token
    IDLE = "IDLE"
    norm_cores = OrderedDict()
    all_tasks = set()
    for label, seq in cores.items():
        norm = []
        for tok in seq:
            t = tok.upper()
            if t == "-" or t == "IDLE":
                norm.append(IDLE)
            else:
                norm.append(t)
                all_tasks.add(t)
        norm_cores[label] = norm

    # --- Color map per TASK (legendable), not per span ---
    # Idle is a fixed gray, not in legend
    task_colors = {}
    for t in sorted(all_tasks, key=lambda x: (x[0] != 'T', x)):  # T1, T2, ... first
        # reasonable vivid but distinct colors
        h = rng.random()
        s = 0.65 + 0.25 * rng.random()
        v = 0.85 + 0.15 * rng.random()
        task_colors[t] = mcolors.hsv_to_rgb((h, s, v))
    idle_color = (0.75, 0.75, 0.75)

    # Figure size scales with run length and core count
    width  = min(18, max(8, 6 + math.log10(max(10, max_len)) * 4))
    height = min(16, max(3, 0.6 * ncores))

    fig, ax = plt.subplots(figsize=(width, height))
    lane_h, lane_gap = 0.6, 0.2
    yticks, ylabels = [], []

    # Compress consecutive equal tokens for efficient plotting
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

    # Draw lanes
    for idx, (label, seq) in enumerate(norm_cores.items()):
        spans = compress_runs(seq)
        ybase = idx * (lane_h + lane_gap)
        for task, start, length in spans:
            color = idle_color if task == IDLE else task_colors.get(task, idle_color)
            ax.broken_barh([(start, length)], (ybase, lane_h),
                           facecolors=color, edgecolors="none")
        yticks.append(ybase + lane_h / 2)
        ylabels.append(label)

    ax.set_ylim(0, ncores * (lane_h + lane_gap))
    ax.set_xlim(0, max_len)
    ax.set_yticks(yticks)
    ax.set_yticklabels(ylabels)
    ax.set_xlabel("Tick")
    ax.set_title(title)

    # --- Conditional axes/legend behavior ---
    SMALL_THRESHOLD = 200
    if max_len <= SMALL_THRESHOLD:
        # Show readable ticks (<= ~20 major ticks) and a legend
        step = max(1, max_len // 20)  # ~20 major ticks or fewer
        ax.xaxis.set_major_locator(ticker.MultipleLocator(base=step))
        ax.xaxis.set_minor_locator(ticker.NullLocator())
        ax.grid(True, axis="x", linestyle=":", linewidth=0.8)

        # Legend per TASK (exclude IDLE)
        legend_handles = [
            Patch(facecolor=task_colors[t], edgecolor="none", label=t)
            for t in sorted(all_tasks, key=lambda x: (x[0] != 'T', x))
        ]
        if legend_handles:
            ax.legend(handles=legend_handles, title="Task",
            loc="center left", bbox_to_anchor=(1.02, 0.5), frameon=False)
    else:
        # Large run: no x ticks, no legend, minimal grid
        ax.xaxis.set_major_locator(ticker.NullLocator())
        ax.xaxis.set_minor_locator(ticker.NullLocator())
        ax.grid(False, axis="x")

    plt.tight_layout()
    plt.show()

def main():
    cores = read_timelines("core_trace.txt")  # fixed file name
    if not cores:
        print("No valid lines found in input")
        sys.exit(1)
    plot_timelines(cores)

if __name__ == "__main__":
    main()
