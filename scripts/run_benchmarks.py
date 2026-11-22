import csv
import datetime
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys


def main():
    if len(sys.argv) != 1:
        raise SystemExit("Usage: python3 scripts/run_benchmarks.py")
    root = Path(__file__).resolve().parent.parent
    runs = 5

    build = root / "build-bench"
    subprocess.run(["cmake", "-S", str(root), "-B", str(build),
                    "-DCMAKE_BUILD_TYPE=Release", "-DENABLE_SANITIZERS=OFF"], check=True)
    subprocess.run(["cmake", "--build", str(build), "--target", "benchmark", "--parallel", "4"], check=True)

    timestamp = datetime.datetime.now().astimezone().strftime("%Y%m%d-%H%M%S-%f")
    output = root / "results" / timestamp
    output.mkdir(parents=True, exist_ok=False)
    print(f"Results: {output.resolve()}", flush=True)

    rows = []
    with (output / "runs.csv").open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=["run", "seq", "par"])
        writer.writeheader()
        file.flush()
        for run in range(1, runs + 1):
            order = ("seq", "par") if run % 2 else ("par", "seq")
            row = {"run": run}
            for mode in order:
                threads = 1 if mode == "seq" else 4
                env = dict(os.environ, PARLAY_NUM_THREADS=str(threads))
                result = subprocess.run([str(build / "benchmark"), mode],
                                        env=env, text=True, stdout=subprocess.PIPE, check=True)
                seconds = float(result.stdout)
                if not math.isfinite(seconds) or seconds <= 0:
                    raise RuntimeError("Benchmark returned invalid timing")
                row[mode] = seconds
                print(f"Run {run}/{runs}: {mode}, {seconds:.6f} s", flush=True)
            writer.writerow(row)
            file.flush()
            rows.append(row)

    seq_mean = statistics.mean(row["seq"] for row in rows)
    par_mean = statistics.mean(row["par"] for row in rows)
    speedup = seq_mean / par_mean
    report = [
        "# Quicksort benchmark", "",
        f"- Elements: 100000000; runs per mode: {runs}",
        "- seq: 1 thread; par: 4 threads; each measurement in a fresh process",
        "- Timed: sort call including scratch allocation/free; generation and verification excluded", "",
        "| Run | seq, s | par, s |",
        "|---|---:|---:|",
    ]
    for row in rows:
        report.append(f"| {row['run']} | {row['seq']:.6f} | {row['par']:.6f} |")
    report.extend([f"| Mean | {seq_mean:.6f} | {par_mean:.6f} |", "",
                   f"**Speedup: {speedup:.3f}x** (mean(seq) / mean(par))", ""])
    (output / "summary.md").write_text("\n".join(report), encoding="utf-8")
    print(f"\nMean seq: {seq_mean:.6f} s\nMean par: {par_mean:.6f} s\nSpeedup: {speedup:.3f}x")
    print(f"Report: {output / 'summary.md'}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
