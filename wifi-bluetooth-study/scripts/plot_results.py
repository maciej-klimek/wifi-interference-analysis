#!/usr/bin/env python3
"""
WiFi-Bluetooth Interference Study - Results Plotter
"""

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def main():
    parser = argparse.ArgumentParser(description="Plot WiFi-Bluetooth interference results")
    parser.add_argument("--csv", required=True, help="Input CSV file")
    parser.add_argument("--output-dir", default="results", help="Output directory for plots")
    args = parser.parse_args()

    csv_file = Path(args.csv)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(exist_ok=True)

    # Load data
    print(f"Loading {csv_file}...")
    df = pd.read_csv(csv_file)
    print(f"Loaded {len(df)} rows")
    print(f"Columns: {list(df.columns)}")

    # Compute statistics
    print("\nComputing statistics...")
    bt_off = df[df["bluetooth_enabled"] == 0]
    bt_on = df[df["bluetooth_enabled"] == 1]

    bt_off_throughput = bt_off["throughput_mbps"].values
    bt_on_throughput = bt_on["throughput_mbps"].values

    stats = {
        "Off": {
            "mean": bt_off_throughput.mean(),
            "std": bt_off_throughput.std(),
            "values": bt_off_throughput,
        },
        "On": {
            "mean": bt_on_throughput.mean(),
            "std": bt_on_throughput.std(),
            "values": bt_on_throughput,
        },
    }

    print(f"\nBluetooth OFF:")
    print(f"  Throughput: {stats['Off']['mean']:.4f} ± {stats['Off']['std']:.4f} Mbps")
    print(f"  Runs: {len(bt_off_throughput)}")

    print(f"\nBluetooth ON:")
    print(f"  Throughput: {stats['On']['mean']:.4f} ± {stats['On']['std']:.4f} Mbps")
    print(f"  Runs: {len(bt_on_throughput)}")

    if stats['Off']['mean'] > 0:
        impact = ((stats['Off']['mean'] - stats['On']['mean']) / stats['Off']['mean']) * 100
        print(f"\nBluetooth Interference Impact: {impact:.2f}% throughput reduction")

    # Plot throughput comparison
    fig, ax = plt.subplots(figsize=(10, 6))

    labels = list(stats.keys())
    means = [stats[label]["mean"] for label in labels]
    stds = [stats[label]["std"] for label in labels]

    x_pos = np.arange(len(labels))
    bars = ax.bar(x_pos, means, yerr=stds, capsize=10, alpha=0.7,
                   color=["green", "red"], edgecolor="black", linewidth=2)

    ax.set_ylabel("Throughput (Mbps)", fontsize=12, fontweight="bold")
    ax.set_xlabel("Bluetooth Status", fontsize=12, fontweight="bold")
    ax.set_title("WiFi Throughput: Bluetooth Off vs On", fontsize=14, fontweight="bold")
    ax.set_xticks(x_pos)
    ax.set_xticklabels(labels)
    ax.grid(axis="y", alpha=0.3)

    # Add value labels
    for bar, mean, std in zip(bars, means, stds):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2.0, height,
                f"{mean:.3f}±{std:.3f}", ha="center", va="bottom",
                fontsize=10, fontweight="bold")

    plt.tight_layout()
    output_file = output_dir / "throughput_comparison.png"
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"\nPlot saved: {output_file}")

    # Plot distribution
    fig, ax = plt.subplots(figsize=(10, 6))

    bp = ax.boxplot([bt_off_throughput, bt_on_throughput], labels=labels,
                     patch_artist=True, widths=0.6)

    for patch, color in zip(bp['boxes'], ["green", "red"]):
        patch.set_facecolor(color)
        patch.set_alpha(0.7)

    ax.set_ylabel("Throughput (Mbps)", fontsize=12, fontweight="bold")
    ax.set_xlabel("Bluetooth Status", fontsize=12, fontweight="bold")
    ax.set_title("WiFi Throughput Distribution", fontsize=14, fontweight="bold")
    ax.grid(axis="y", alpha=0.3)

    plt.tight_layout()
    output_file = output_dir / "throughput_distribution.png"
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"Plot saved: {output_file}")

    print("\n✓ Plotting complete!")


if __name__ == "__main__":
    main()
