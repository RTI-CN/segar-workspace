#!/usr/bin/env python3
# Parse log file for ITEST_LAT latency_ns, convert to us, plot curve.

import argparse
import re
import sys


def parse_latency_ns(log_path):
    """从日志中提取 latency_ns 字段，返回 (样本序号, latency_us) 列表。"""
    latencies_us = []
    pattern = re.compile(r'latency_ns=(\d+)')
    with open(log_path, 'r') as f:
        for line in f:
            m = pattern.search(line)
            if m:
                ns = int(m.group(1))
                us = ns // 1000  # 转为 us，不保留小数
                latencies_us.append(us)
    return latencies_us


def main():
    parser = argparse.ArgumentParser(description='Parse latency_ns from log, convert to us, plot.')
    parser.add_argument('log_file', help='Path to log file')
    parser.add_argument('-o', '--output', help='Save image to file (default: only display)')
    args = parser.parse_args()

    latencies_us = parse_latency_ns(args.log_file)
    if not latencies_us:
        print('No latency_ns found in', args.log_file, file=sys.stderr)
        sys.exit(1)

    import matplotlib.pyplot as plt

    x = list(range(1, len(latencies_us) + 1))
    plt.figure(figsize=(10, 5))
    plt.plot(x, latencies_us, linewidth=0.8)
    plt.xlabel('Sample index')
    plt.ylabel('Latency (us)')
    plt.title(f'Latency curve (n={len(latencies_us)})')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    if args.output:
        plt.savefig(args.output, dpi=150)
        print(f'Saved {len(latencies_us)} points to {args.output}')
    plt.show()


if __name__ == '__main__':
    main()
