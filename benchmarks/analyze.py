def percentile(sorted_vals, p):
    idx = int(len(sorted_vals) * p / 100)
    return sorted_vals[min(idx, len(sorted_vals) - 1)]

with open("benchmarks/results/latency_raw.csv") as f:
    vals = sorted(int(line.strip()) for line in f if line.strip())

p50  = percentile(vals, 50)
p99  = percentile(vals, 99)
p999 = percentile(vals, 99.9)

table = f"""\
| Percentile | Latency (ns) |
|---|---|
| p50   | {p50:,} |
| p99   | {p99:,} |
| p99.9 | {p999:,} |
| count | {len(vals):,} |
"""

print(table)

with open("benchmarks/results/latency_summary.md", "w") as f:
    f.write(table)
