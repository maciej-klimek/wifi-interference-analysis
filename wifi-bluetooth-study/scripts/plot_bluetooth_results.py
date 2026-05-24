#!/usr/bin/env python3
"""
WiFi-Bluetooth Interference Study - Results Plotter
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


plt.style.use("seaborn-v0_8-whitegrid")


def summarize_group(values: pd.Series) -> dict:
    if values.empty:
        return {"mean": np.nan, "std": np.nan, "count": 0, "values": values.to_numpy()}

    return {
        "mean": float(values.mean()),
        "std": float(values.std(ddof=0)),
        "count": int(values.shape[0]),
        "values": values.to_numpy(),
    }


def format_stat(value: float) -> str:
    if pd.isna(value):
        return "n/a"
    return f"{value:.3f}"


def save_figure(fig, output_dir: Path, basename: str) -> None:
    png_file = output_dir / f"{basename}.png"
    svg_file = output_dir / f"{basename}.svg"
    fig.savefig(png_file, dpi=300, bbox_inches="tight")
    fig.savefig(svg_file, bbox_inches="tight")
    print(f"Plot saved: {png_file}")
    print(f"Plot saved: {svg_file}")


def cumulative_stats(values: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    if len(values) == 0:
        return np.array([]), np.array([]), np.array([])

    cumulative_mean = np.empty(len(values), dtype=float)
    cumulative_lower = np.empty(len(values), dtype=float)
    cumulative_upper = np.empty(len(values), dtype=float)

    for index in range(1, len(values) + 1):
        window = values[:index]
        mean = window.mean()
        std = window.std(ddof=0)
        ci95 = 1.96 * std / np.sqrt(index)
        cumulative_mean[index - 1] = mean
        cumulative_lower[index - 1] = mean - ci95
        cumulative_upper[index - 1] = mean + ci95

    return cumulative_mean, cumulative_lower, cumulative_upper


def zoom_limits(values: np.ndarray, padding_ratio: float = 0.2) -> tuple[float, float]:
    if len(values) == 0:
        return 0.0, 1.0

    lower, upper = np.percentile(values, [5, 95])
    if np.isclose(lower, upper):
        lower = float(values.min())
        upper = float(values.max())

    span = max(upper - lower, 1e-6)
    padding = span * padding_ratio
    return float(lower - padding), float(upper + padding)


def main():
    parser = argparse.ArgumentParser(description="Plot WiFi-Bluetooth interference results")
    parser.add_argument("csv", nargs="?", help="Input CSV file")
    parser.add_argument("--csv", dest="csv_option", help="Input CSV file")
    parser.add_argument("--output-dir", default="results/bluetooth", help="Output directory for plots")
    args = parser.parse_args()

    csv_path = args.csv_option or args.csv
    if not csv_path:
        parser.error("the following argument is required: csv or --csv")

    csv_file = Path(csv_path)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Load data
    print(f"Loading {csv_file}...")
    df = pd.read_csv(csv_file)
    print(f"Loaded {len(df)} rows")
    print(f"Columns: {list(df.columns)}")

    required_columns = {"rng_run", "bluetooth_enabled", "throughput_mbps"}
    missing_columns = required_columns - set(df.columns)
    if missing_columns:
        raise SystemExit(f"CSV is missing required columns: {sorted(missing_columns)}")

    # Compute statistics
    print("\nComputing statistics...")
    bt_off = df[df["bluetooth_enabled"] == 0].copy()
    bt_on = df[df["bluetooth_enabled"] == 1].copy()

    stats = {
        "Off": summarize_group(bt_off["throughput_mbps"]),
        "On": summarize_group(bt_on["throughput_mbps"]),
    }

    print("\nBluetooth OFF:")
    print(f"  Throughput: {format_stat(stats['Off']['mean'])} ± {format_stat(stats['Off']['std'])} Mbps")
    print(f"  Runs: {stats['Off']['count']}")

    print("\nBluetooth ON:")
    print(f"  Throughput: {format_stat(stats['On']['mean'])} ± {format_stat(stats['On']['std'])} Mbps")
    print(f"  Runs: {stats['On']['count']}")

    if stats["Off"]["count"] > 0 and stats["On"]["count"] > 0 and not pd.isna(stats["Off"]["mean"]):
        impact = ((stats["Off"]["mean"] - stats["On"]["mean"]) / stats["Off"]["mean"]) * 100
        print(f"\nBluetooth Interference Impact: {impact:.2f}% throughput reduction")

    pair_df = (
        bt_off[["rng_run", "throughput_mbps"]]
        .rename(columns={"throughput_mbps": "throughput_off"})
        .merge(
            bt_on[["rng_run", "throughput_mbps"]].rename(columns={"throughput_mbps": "throughput_on"}),
            on="rng_run",
            how="inner",
        )
        .sort_values("rng_run")
    )

    # Plot throughput comparison
    available_statuses = [label for label in ["Off", "On"] if stats[label]["count"] > 0]
    labels = available_statuses if available_statuses else ["Off", "On"]
    means = [stats[label]["mean"] for label in labels]
    stds = [stats[label]["std"] if not pd.isna(stats[label]["std"]) else 0.0 for label in labels]
    colors = ["#2ca02c" if label == "Off" else "#d62728" for label in labels]

    # Plot throughput comparison
    fig, ax = plt.subplots(figsize=(10, 6))

    x_pos = np.arange(len(labels))
    bars = ax.bar(x_pos, means, yerr=stds, capsize=10, alpha=0.7,
                   color=colors, edgecolor="black", linewidth=2)

    ax.set_ylabel("Throughput (Mbps)", fontsize=12, fontweight="bold")
    ax.set_xlabel("Bluetooth Status", fontsize=12, fontweight="bold")
    ax.set_title(f"WiFi Throughput: Bluetooth Off vs On\n{csv_file.name}", fontsize=14, fontweight="bold")
    ax.set_xticks(x_pos)
    ax.set_xticklabels(labels)
    ax.grid(axis="y", alpha=0.3)
    ax.margins(x=0.12)

    # Add value labels
    for bar, mean, std in zip(bars, means, stds):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2.0, height,
                f"{mean:.3f}±{std:.3f}", ha="center", va="bottom",
                fontsize=10, fontweight="bold")

    plt.tight_layout()
    print()
    save_figure(fig, output_dir, "throughput_comparison")

    # Plot distribution
    fig, axes = plt.subplots(1, len(labels), figsize=(13, 5), sharey=False)
    if len(labels) == 1:
        axes = [axes]

    rng = np.random.default_rng(12345)
    jitter_width = 0.08

    for ax, label, color in zip(axes, labels, colors):
        values = stats[label]["values"]
        position = 0.0

        ax.boxplot(
            [values],
            positions=[position],
            patch_artist=True,
            widths=0.35,
            showfliers=False,
        )
        ax.patches[0].set_facecolor(color)
        ax.patches[0].set_alpha(0.35)

        jitter = rng.uniform(-jitter_width, jitter_width, size=len(values))
        ax.scatter(
            np.full(len(values), position) + jitter,
            values,
            s=34,
            color=color,
            alpha=0.75,
            edgecolor="white",
            linewidth=0.5,
            zorder=3,
        )

        mean = stats[label]["mean"]
        ci95 = 1.96 * stats[label]["std"] / np.sqrt(max(stats[label]["count"], 1))
        median = np.median(values) if len(values) else np.nan
        q1, q3 = np.percentile(values, [25, 75]) if len(values) else (np.nan, np.nan)

        ax.errorbar(
            position,
            mean,
            yerr=ci95,
            fmt="o",
            color="black",
            ecolor="black",
            elinewidth=2,
            capsize=6,
            markersize=0,
            zorder=4,
        )
        ax.hlines(mean, position - 0.16, position + 0.16, colors="black", linewidth=2.2, zorder=4)
        ax.hlines([q1, q3], position - 0.10, position + 0.10, colors=color, linewidth=1.6, alpha=0.9, zorder=4)

        low, high = zoom_limits(values)
        outlier_count = int(np.sum((values < low) | (values > high)))
        ax.set_ylim(low, high)
        ax.set_xticks([position])
        ax.set_xticklabels([label])
        ax.set_ylabel("Throughput (Mbps)", fontsize=12, fontweight="bold")
        ax.set_title(f"{label}: zoomed distribution", fontsize=13, fontweight="bold")
        ax.grid(axis="y", alpha=0.3)
        ax.text(
            0.5,
            0.97,
            f"mean={mean:.2f}  CI95=±{ci95:.2f}\nmedian={median:.2f}",
            transform=ax.transAxes,
            ha="center",
            va="top",
            fontsize=9,
            bbox=dict(boxstyle="round,pad=0.25", facecolor="white", alpha=0.8, edgecolor="none"),
        )
        if outlier_count > 0:
            ax.text(
                0.5,
                0.02,
                f"{outlier_count} outlier(s) outside zoom",
                transform=ax.transAxes,
                ha="center",
                va="bottom",
                fontsize=9,
                color="#b22222",
            )

    fig.suptitle(f"WiFi Throughput Distribution by Run\n{csv_file.name}", fontsize=15, fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.93))
    save_figure(fig, output_dir, "throughput_distribution")

    if not pair_df.empty:
        off_values = pair_df["throughput_off"].to_numpy()
        on_values = pair_df["throughput_on"].to_numpy()
        delta_values = off_values - on_values
        delta_pct = delta_values / off_values * 100.0

        off_mean, off_lower, off_upper = cumulative_stats(off_values)
        on_mean, on_lower, on_upper = cumulative_stats(on_values)
        delta_mean, delta_lower, delta_upper = cumulative_stats(delta_values)
        pct_mean, pct_lower, pct_upper = cumulative_stats(delta_pct)

        fig, axes = plt.subplots(4, 1, figsize=(11, 14), sharex=True)
        x = np.arange(1, len(off_values) + 1)

        axes[0].plot(x, off_mean, color="#2ca02c", linewidth=2.0, label="BT off")
        axes[0].fill_between(x, off_lower, off_upper, color="#2ca02c", alpha=0.15)
        axes[0].set_ylabel("Mbps", fontsize=11, fontweight="bold")
        axes[0].set_title("Skumulowana średnia przepustowość BT off", fontsize=13, fontweight="bold")
        axes[0].grid(axis="both", alpha=0.3)
        axes[0].set_ylim(zoom_limits(off_values, padding_ratio=0.08))

        axes[1].plot(x, on_mean, color="#d62728", linewidth=2.0, label="BT on")
        axes[1].fill_between(x, on_lower, on_upper, color="#d62728", alpha=0.15)
        axes[1].set_ylabel("Mbps", fontsize=11, fontweight="bold")
        axes[1].set_title("Skumulowana średnia przepustowość BT on", fontsize=13, fontweight="bold")
        axes[1].grid(axis="both", alpha=0.3)
        axes[1].set_ylim(zoom_limits(on_values, padding_ratio=0.08))

        axes[2].plot(x, delta_mean, color="#1f77b4", linewidth=2.0, label="BT off - BT on")
        axes[2].fill_between(x, delta_lower, delta_upper, color="#1f77b4", alpha=0.15)
        axes[2].set_ylabel("Mbps", fontsize=11, fontweight="bold")
        axes[2].set_title("Skumulowana strata throughputu", fontsize=13, fontweight="bold")
        axes[2].grid(axis="both", alpha=0.3)
        axes[2].set_ylim(zoom_limits(delta_values, padding_ratio=0.2))

        axes[3].plot(x, pct_mean, color="#9467bd", linewidth=2.0, label="Reduction %")
        axes[3].fill_between(x, pct_lower, pct_upper, color="#9467bd", alpha=0.12)
        axes[3].set_ylabel("%", fontsize=11, fontweight="bold")
        axes[3].set_xlabel("Liczba kolejnych prób", fontsize=12, fontweight="bold")
        axes[3].set_title("Skumulowana redukcja procentowa throughputu", fontsize=13, fontweight="bold")
        axes[3].grid(axis="both", alpha=0.3)
        axes[3].set_ylim(zoom_limits(delta_pct, padding_ratio=0.2))

        for axis in axes:
            axis.set_xlim(1, len(off_values))

        fig.suptitle(f"Convergence of Throughput Statistics and 95% CI\n{csv_file.name}", fontsize=15, fontweight="bold")
        plt.tight_layout(rect=(0, 0, 1, 0.96))
        save_figure(fig, output_dir, "throughput_convergence")
    else:
        print("No paired BT off/on runs found; skipping convergence plot.")

    print("\n✓ Plotting complete!")


if __name__ == "__main__":
    main()
