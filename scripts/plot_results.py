import csv
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "results.csv"
CHARTS = ROOT / "charts"


def read_rows():
    with DATA.open(newline="") as f:
        return list(csv.DictReader(f))


def plot_metric(rows, metric, ylabel, suffix):
    grouped = defaultdict(list)
    for row in rows:
        key = (row["dataset"], row["algorithm"])
        grouped[key].append((int(row["size"]), float(row[metric])))

    datasets = sorted({row["dataset"] for row in rows})
    CHARTS.mkdir(exist_ok=True)

    for dataset in datasets:
        plt.figure(figsize=(10, 6))
        for (ds, algorithm), values in sorted(grouped.items()):
            if ds != dataset:
                continue
            values.sort()
            xs = [x for x, _ in values]
            ys = [y for _, y in values]
            plt.plot(xs, ys, marker="o", linewidth=1.4, markersize=3, label=algorithm)

        plt.title(f"{dataset}: {ylabel}")
        plt.xlabel("Array size")
        plt.ylabel(ylabel)
        plt.grid(True, alpha=0.25)
        plt.legend(fontsize=8)
        plt.tight_layout()
        plt.savefig(CHARTS / f"{dataset}_{suffix}.png", dpi=160)
        plt.close()


def main():
    rows = read_rows()
    plot_metric(rows, "avg_time_us", "Average time, microseconds", "time")
    plot_metric(rows, "avg_char_comparisons", "Character comparisons / inspections", "chars")
    print(f"Saved PNG charts to {CHARTS}")


if __name__ == "__main__":
    main()
