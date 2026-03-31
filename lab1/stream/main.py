#!/usr/bin/env python3

import argparse
import subprocess

import polars as pl
import matplotlib.pyplot as plt


KERNEL_MULTIPLIERS = {
    "stream::copy": 2,
    "stream::scale": 2,
    "stream::add": 3,
    "stream::triad": 3,
    "stream::dot": 2,
}


def parse_args():
    parser = argparse.ArgumentParser(description="Plot STREAM benchmark bandwidth")
    parser.add_argument("input", nargs="?", help="Path to STREAM CSV results file")
    parser.add_argument(
        "-r",
        "--run",
        action="store_true",
        help="Run the benchmark executable",
    )
    parser.add_argument(
        "-s",
        "--size",
        type=int,
        required=True,
        help="Buffer size in bytes (per array)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="stream_bw.pdf",
        help="Output PDF path (default: stream_bw.pdf)",
    )
    parser.add_argument(
        "-e",
        "--exe",
        default="build/src/bin/top.stream_main",
        help="Benchmark executable path (default: build/src/bin/top.stream_main)",
    )
    parser.add_argument(
        "--no-save",
        action="store_true",
        help="Do not save benchmark results to CSV",
    )
    return parser.parse_args()


def run_benchmark(exe: str, size_mib: int) -> str:
    result = subprocess.run([exe, str(size_mib)], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"Benchmark failed: {result.stderr}")
    return result.stdout


def save_csv(output: str, csv_path: str) -> None:
    with open(csv_path, "w") as f:
        f.write(output)


def parse_output(output: str) -> pl.DataFrame:
    lines = output.strip().split("\n")
    raw_col = lines[0].strip()

    rows = []
    for line in lines[1:]:
        parts = [p.strip() for p in line.split() if p.strip()]
        if parts:
            rows.append(parts)

    return pl.DataFrame(
        {
            "name": [r[0] for r in rows],
            "mean (s)": [float(r[1]) for r in rows],
            "sdev (%)": [float(r[2]) for r in rows],
            "min (s)": [float(r[3]) for r in rows],
            "med (s)": [float(r[4]) for r in rows],
            "max (s)": [float(r[5]) for r in rows],
        }
    )


def compute_bandwidth(df: pl.DataFrame, size_bytes: int) -> pl.DataFrame:
    n_elements = size_bytes / 8

    def get_bandwidth(row: dict) -> float | None:
        name = row["name"]
        mean_time = row["mean (s)"]
        multiplier = KERNEL_MULTIPLIERS.get(name)
        if multiplier is None:
            return None
        bytes_accessed = n_elements * 8 * multiplier
        return bytes_accessed / mean_time

    rows = df.iter_rows(named=True)
    bandwidth_data = []
    for row in rows:
        bw = get_bandwidth(row)
        if bw is not None:
            sdev_pct = row["sdev (%)"]
            bandwidth_data.append(
                {
                    "name": row["name"],
                    "bandwidth": bw,
                    "error": (sdev_pct / 100) * bw,
                }
            )
    return pl.DataFrame(bandwidth_data)


def plot_bandwidth(df: pl.DataFrame, output_path: str) -> None:
    names = [n.removeprefix("stream::") for n in df["name"]]
    bandwidths = df["bandwidth"].to_list()
    errors = df["error"].to_list()

    if all(b >= 1000 for b in bandwidths):
        bandwidths = [b / 1000 for b in bandwidths]
        errors = [e / 1000 for e in errors]
        ylabel = "Bandwidth (GB/s)"
    else:
        ylabel = "Bandwidth (MB/s)"

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.bar(names, bandwidths, yerr=errors, capsize=5, color="steelblue", alpha=0.8)
    ax.set_xlabel("Kernel")
    ax.set_ylabel(ylabel)
    ax.set_title("STREAM Benchmark Bandwidth")
    ax.grid(axis="y", linestyle="--", alpha=0.7)
    plt.xticks(rotation=15)
    plt.tight_layout()
    plt.savefig(output_path, format="pdf")
    plt.close()


def main():
    args = parse_args()

    if args.run:
        size_mib = args.size // (1024 * 1024)
        output = run_benchmark(args.exe, size_mib)

        if not args.no_save:
            csv_path = f"stream_{size_mib}_res.csv"
            save_csv(output, csv_path)
            print(f"Saved results to {csv_path}")
    else:
        if args.input is None:
            raise ValueError("input argument required when not using --run")
        with open(args.input) as f:
            output = f.read()

    df = parse_output(output)
    bw_df = compute_bandwidth(df, args.size)
    plot_bandwidth(bw_df, args.output)


if __name__ == "__main__":
    main()
