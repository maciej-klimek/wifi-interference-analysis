#!/usr/bin/env python3
"""
WiFi-Microwave Interference Study - Results Plotter
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

plt.style.use("seaborn-v0_8-whitegrid")


def save_figure(fig, output_dir: Path, basename: str) -> None:
    png_file = output_dir / f"{basename}.png"
    svg_file = output_dir / f"{basename}.svg"
    fig.savefig(png_file, dpi=300, bbox_inches="tight")
    fig.savefig(svg_file, bbox_inches="tight")
    print(f"Plot saved: {png_file}")
    print(f"Plot saved: {svg_file}")


def mean_ci(values: np.ndarray) -> tuple[float, float, float, int]:
    if len(values) == 0:
        return 0.0, 0.0, 0.0, 0
    mean = float(values.mean())
    std = float(values.std(ddof=0))
    ci95 = 1.96 * std / np.sqrt(len(values)) if len(values) > 0 else 0.0
    return mean, mean - ci95, mean + ci95, len(values)


def aggregate_time_series(df: pd.DataFrame) -> pd.DataFrame:
    grouped = (
        df.groupby(["station_label", "bin_index", "bin_start_s", "bin_end_s", "phase"], as_index=False)
        .agg(mean_throughput_mbps=("throughput_mbps", "mean"), std_throughput_mbps=("throughput_mbps", "std"), run_count=("rng_run", "nunique"))
    )
    grouped["std_throughput_mbps"] = grouped["std_throughput_mbps"].fillna(0.0)
    grouped["ci95_mbps"] = 1.96 * grouped["std_throughput_mbps"] / np.sqrt(grouped["run_count"].clip(lower=1))
    return grouped


def per_run_phase_stats(df: pd.DataFrame) -> pd.DataFrame:
    phase_run = (
        df.groupby(["rng_run", "station_label", "phase"], as_index=False)
        .agg(run_throughput_mbps=("throughput_mbps", "mean"))
    )
    phase_summary = (
        phase_run.groupby(["station_label", "phase"], as_index=False)
        .agg(mean_throughput_mbps=("run_throughput_mbps", "mean"), std_throughput_mbps=("run_throughput_mbps", "std"), run_count=("rng_run", "nunique"))
    )
    phase_summary["std_throughput_mbps"] = phase_summary["std_throughput_mbps"].fillna(0.0)
    phase_summary["ci95_mbps"] = 1.96 * phase_summary["std_throughput_mbps"] / np.sqrt(phase_summary["run_count"].clip(lower=1))
    return phase_summary


def main():
    parser = argparse.ArgumentParser(description="Plot WiFi-microwave interference results")
    parser.add_argument("csv", nargs="?", help="Input CSV file")
    parser.add_argument("--csv", dest="csv_option", help="Input CSV file")
    parser.add_argument("--output-dir", default="results/microwave", help="Output directory for plots")
    args = parser.parse_args()

    csv_path = args.csv_option or args.csv
    if not csv_path:
        parser.error("the following argument is required: csv or --csv")

    csv_file = Path(csv_path)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Loading {csv_file}...")
    df = pd.read_csv(csv_file)
    print(f"Loaded {len(df)} rows")
    print(f"Columns: {list(df.columns)}")

    required_columns = {
        "rng_run",
        "station_id",
        "station_label",
        "bin_index",
        "bin_start_s",
        "bin_end_s",
        "microwave_active",
        "phase",
        "rx_bytes",
        "throughput_mbps",
    }
    missing_columns = required_columns - set(df.columns)
    if missing_columns:
        raise SystemExit(f"CSV is missing required columns: {sorted(missing_columns)}")

    station_labels = sorted(df["station_label"].dropna().unique().tolist())
    if not station_labels:
        raise SystemExit("No station labels found in CSV")

    microwave_window = df[df["phase"] == "microwave-on"]
    if microwave_window.empty:
        raise SystemExit("No microwave-on phase found in CSV")

    mw_start = float(microwave_window["bin_start_s"].min())
    mw_stop = float(microwave_window["bin_end_s"].max())

    print("\nComputing statistics...")
    print(f"Microwave active window: {mw_start:.2f}s .. {mw_stop:.2f}s")

    time_series = aggregate_time_series(df)
    phase_summary = per_run_phase_stats(df)

    # Overall phase summary table for terminal output
    print("\nPhase summary:")
    for station in station_labels:
        station_rows = phase_summary[phase_summary["station_label"] == station]
        print(f"  {station}:")
        for phase_name in ["pre", "microwave-on", "post"]:
            row = station_rows[station_rows["phase"] == phase_name]
            if row.empty:
                continue
            mean = row.iloc[0]["mean_throughput_mbps"]
            ci95 = row.iloc[0]["ci95_mbps"]
            print(f"    {phase_name}: {mean:.3f} ± {ci95:.3f} Mbps")

    # Time series figure
    total_series = (
        df.groupby(["rng_run", "bin_index", "bin_start_s", "bin_end_s"], as_index=False)
        .agg(throughput_mbps=("throughput_mbps", "sum"), phase=("phase", "first"))
        .groupby(["bin_index", "bin_start_s", "bin_end_s", "phase"], as_index=False)
        .agg(mean_throughput_mbps=("throughput_mbps", "mean"), std_throughput_mbps=("throughput_mbps", "std"), run_count=("throughput_mbps", "count"))
    )
    total_series["std_throughput_mbps"] = total_series["std_throughput_mbps"].fillna(0.0)
    total_series["ci95_mbps"] = 1.96 * total_series["std_throughput_mbps"] / np.sqrt(total_series["run_count"].clip(lower=1))

    fig, axes = plt.subplots(len(station_labels) + 1, 1, figsize=(11, 12), sharex=True)
    if len(station_labels) == 1:
        axes = [axes]

    colors = {station_labels[0]: "#1f77b4", station_labels[1] if len(station_labels) > 1 else station_labels[0]: "#d62728"}
    if len(station_labels) > 1:
        colors[station_labels[1]] = "#d62728"

    for axis, station in zip(axes[: len(station_labels)], station_labels):
        station_series = time_series[time_series["station_label"] == station]
        axis.plot(station_series["bin_start_s"], station_series["mean_throughput_mbps"], color=colors[station], linewidth=2.0)
        axis.fill_between(
            station_series["bin_start_s"],
            station_series["mean_throughput_mbps"] - station_series["ci95_mbps"],
            station_series["mean_throughput_mbps"] + station_series["ci95_mbps"],
            color=colors[station],
            alpha=0.18,
        )
        axis.axvspan(mw_start, mw_stop, color="#ffcc00", alpha=0.18, label="microwave active" if station == station_labels[0] else None)
        axis.set_ylabel("Mbps", fontsize=11, fontweight="bold")
        axis.set_title(f"{station}: mean throughput over time", fontsize=13, fontweight="bold")
        axis.grid(axis="y", alpha=0.3)

    total_axis = axes[-1]
    total_axis.plot(total_series["bin_start_s"], total_series["mean_throughput_mbps"], color="#2ca02c", linewidth=2.2)
    total_axis.fill_between(
        total_series["bin_start_s"],
        total_series["mean_throughput_mbps"] - total_series["ci95_mbps"],
        total_series["mean_throughput_mbps"] + total_series["ci95_mbps"],
        color="#2ca02c",
        alpha=0.18,
    )
    total_axis.axvspan(mw_start, mw_stop, color="#ffcc00", alpha=0.18)
    total_axis.set_ylabel("Mbps", fontsize=11, fontweight="bold")
    total_axis.set_xlabel("Simulation time (s)", fontsize=12, fontweight="bold")
    total_axis.set_title("Total throughput over time", fontsize=13, fontweight="bold")
    total_axis.grid(axis="y", alpha=0.3)

    for axis in axes:
        axis.set_xlim(0.0, float(df["bin_end_s"].max()))

    axes[0].legend(frameon=True, loc="upper right")
    fig.suptitle("Microwave interference: throughput over time with 95% CI", fontsize=15, fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.96))
    save_figure(fig, output_dir, "throughput_time_series")

    # Phase summary figure
    phase_order = ["pre", "microwave-on", "post"]
    x = np.arange(len(phase_order), dtype=float)
    width = 0.2

    fig, ax = plt.subplots(figsize=(11, 6))
    total_phase = (
        df.groupby(["rng_run", "phase"], as_index=False)
        .agg(run_throughput_mbps=("throughput_mbps", "sum"))
        .groupby("phase", as_index=False)
        .agg(mean_throughput_mbps=("run_throughput_mbps", "mean"), std_throughput_mbps=("run_throughput_mbps", "std"), run_count=("rng_run", "count"))
    )
    total_phase["std_throughput_mbps"] = total_phase["std_throughput_mbps"].fillna(0.0)
    total_phase["ci95_mbps"] = 1.96 * total_phase["std_throughput_mbps"] / np.sqrt(total_phase["run_count"].clip(lower=1))

    plotted_series = []
    plotted_labels = []
    if len(station_labels) >= 1:
        plotted_series.append(phase_summary[phase_summary["station_label"] == station_labels[0]])
        plotted_labels.append(station_labels[0])
    if len(station_labels) >= 2:
        plotted_series.append(phase_summary[phase_summary["station_label"] == station_labels[1]])
        plotted_labels.append(station_labels[1])
    plotted_series.append(total_phase.assign(station_label="total"))
    plotted_labels.append("total")

    palette = ["#1f77b4", "#d62728", "#2ca02c"]
    for idx, (series, label, color) in enumerate(zip(plotted_series, plotted_labels, palette)):
        means = []
        cis = []
        for phase_name in phase_order:
            row = series[series["phase"] == phase_name]
            if row.empty:
                means.append(0.0)
                cis.append(0.0)
            else:
                means.append(float(row.iloc[0]["mean_throughput_mbps"]))
                cis.append(float(row.iloc[0]["ci95_mbps"]))
        offset = (idx - 1) * width
        ax.bar(x + offset, means, width=width, yerr=cis, capsize=5, color=color, alpha=0.75, label=label)

    ax.set_xticks(x)
    ax.set_xticklabels(["pre", "microwave on", "post"])
    ax.set_ylabel("Throughput (Mbps)", fontsize=12, fontweight="bold")
    ax.set_title("Average throughput by phase", fontsize=14, fontweight="bold")
    ax.grid(axis="y", alpha=0.3)
    ax.legend(frameon=True)
    save_figure(fig, output_dir, "throughput_phase_summary")

    print("\n✓ Plotting complete!")


if __name__ == "__main__":
    main()
