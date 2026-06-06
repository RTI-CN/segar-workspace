#!/usr/bin/env python3
# ****************************************************************************
# Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
# SPDX-License-Identifier: LicenseRef-Segar-Proprietary
#
# PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
# for license terms and restrictions.
# ****************************************************************************

import csv
import glob
import math
import os
import re
import sys
from datetime import datetime
from subprocess import run, TimeoutExpired

# ============================================================================
# Configuration: Thresholds and Settings
# ============================================================================
# Loss rate threshold (1% = 0.01)
MAX_LOSS_RATE = 0.01

# Latency thresholds
# P99 latency threshold (20ms = 20000000 nanoseconds = 20000 microseconds)
MAX_P99_LATENCY_NS = 20000000
# Average latency threshold (12ms = 12000000 nanoseconds = 12000 microseconds)
MAX_AVG_LATENCY_US = 12000

# Resource usage thresholds
MAX_AVG_CPU_PERCENT = 800.0
MAX_AVG_MEM_MB = 3000.0

# Param timer tolerance (seconds)
PARAM_TICK_TOLERANCE_SEC = 5

# Action test thresholds
MAX_ACTION_SYNC_TIMEOUT = 1
MAX_ACTION_CANCEL_FAIL = 1

# Service test threshold (allowable difference between requests and responses)
MAX_SERVICE_DIFF = 2

# Minimum dump files required
MIN_DUMP_FILES = 1

# Skip initial lines in log files for error detection (to ignore initialization errors)
SKIP_INITIAL_ERROR_LINES = 100

# Error report display limits
MAX_ERROR_FILES_DISPLAY = 5
MAX_ERROR_LINES_TRUNCATE = 100
MAX_ERROR_LINES_DISPLAY = 50

# SyncComponent check configuration
SYNC_CHECK_CONFIG = {
    # SyncComponent integration test node
    "SyncIntegrationNode": {
        # Allowable ratio of timeout mainframes (timeout_mainframes / total_mainframes)
        "max_timeout_mainframes_ratio": 0.01,
        # Maximum allowed timeout events (ITEST_SYNC_TIMEOUT count)
        "max_timeout_events": 1,
        # Maximum allowed max_abs_diff_ms across the whole test
        "max_abs_diff_ms": 40,
        # Main input topic name (REQUIRED)
        "main_input": "sensor_topic_1",
        # REQUIRED inputs (should closely follow main_input frequency)
        "required_inputs": [
            "sensor_topic_1",
            "sensor_topic_2",
        ],
        # WAITABLE inputs (should have high coverage compared to main_input)
        "waitable_inputs": [
            "sensor_topic_3",
        ],
        # OPTIONAL inputs (should participate at least sometimes)
        "optional_inputs": [
            "sensor_topic_4",
        ],
        # Relative frequency tolerance for REQUIRED inputs vs main_input
        "required_freq_tolerance": 0.1,  # 10%
        # Minimal coverage for WAITABLE inputs vs main_input
        "waitable_min_coverage": 0.8,    # 80%
    },
}

# ============================================================================
# Regular Expressions
# ============================================================================
SEND_RE = re.compile(
    r"ITEST_SEND\s+sensor_id=(\d+)\s+topic=([^\s]+)\s+seq=(\d+)\s+msg_ts=(\d+)"
)
RECV_RE = re.compile(
    r"ITEST_LAT\s+node=([^\s]+)\s+input=([^\s]+)\s+seq=(\d+)\s+latency_ns=(\d+)"
)

# SyncComponent integration test logs
SYNC_STAT_RE = re.compile(
    r"ITEST_SYNC_STAT\s+node=([^\s]+)\s+normal=(\d+)\s+latency=(\d+)\s+timeout=(\d+)\s+max_abs_diff_ms=(\d+)"
)
SYNC_TIMEOUT_RE = re.compile(
    r"ITEST_SYNC_TIMEOUT\s+node=([^\s]+)"
)
SYNC_TOPIC_STAT_RE = re.compile(
    r"ITEST_SYNC_TOPIC_STAT\s+node=([^\s]+)\s+input=([^\s]+)\s+count=(\d+)\s+freq_hz=([\d\.]+)"
)

TS_RE = re.compile(r"\[[IWEDC]\s+(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}):(\d{3})\]")
CPU_ALL_RE = re.compile(r"pid:all\s+cpu_detail\.usage:\s+([\d\.]+)")
MEM_ALL_RE = re.compile(r"pid:all\s+mem_detail\.VmRSS:\s+(\d+)")

# ============================================================================
# Utility Functions
# ============================================================================
def status_icon(ok):
    """Return status icon for Markdown output."""
    return "✅" if ok else "❌"


def percentile(sorted_vals, p):
    """Calculate percentile value from sorted list."""
    if not sorted_vals:
        return 0
    if p <= 0:
        return sorted_vals[0]
    if p >= 100:
        return sorted_vals[-1]
    k = (len(sorted_vals) - 1) * (p / 100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return sorted_vals[int(k)]
    d0 = sorted_vals[int(f)] * (c - k)
    d1 = sorted_vals[int(c)] * (k - f)
    return int(d0 + d1)


def parse_timestamp_from_line(line):
    """Extract timestamp from log line, return Unix timestamp or None."""
    ts_match = TS_RE.search(line)
    if ts_match:
        try:
            ts_str = ts_match.group(1) + "." + ts_match.group(2)
            dt = datetime.strptime(ts_str, "%Y-%m-%d %H:%M:%S.%f")
            return int(dt.timestamp())
        except (ValueError, AttributeError):
            pass
    return None


def get_topic_fixed_config(topic_name):
    """Get fixed config for specific topics."""
    # Fixed configs for topics
    topic_configs = {
        "sensor_topic_1": (20, 64),      # 20 Hz, 64KB
        "sensor_topic_2": (100, 64),     # 100 Hz, 64KB
        "sensor_topic_8": (20, 256),     # 20 Hz, 256KB
    }
    return topic_configs.get(topic_name, (None, None))


def get_node_config(node_name):
    """Get fixed config for key nodes."""
    # Fixed configs for key nodes
    node_configs = {
        "NodeA": (20, 64),      # 20 Hz, 64KB
        "NodeB": (100, 4),     # 100 Hz, 4KB
        "NodeC": (20, 256),     # 20 Hz, 256KB
        "NodeD": (20, 64),      # 20 Hz, 64KB
        "NodeE": (20, 64),      # 20 Hz, 64KB
        "NodeF": (20, 64),      # 20 Hz, 64KB
        "NodeG": (100, 64),     # 100 Hz, 64KB
        "NodeH": (20, 64),      # 20 Hz, 64KB
        "NodeI": (15, 1024),    # 15 Hz, 1MB
        "NodeJ": (15, 256),     # 15 Hz, 256KB
        "NodeK": (15, 64),      # 15 Hz, 64KB
        "NodeL": (15, 32),      # 15 Hz, 32KB
        "NodeM": (15, 16),      # 15 Hz, 16KB
    }
    return node_configs.get(node_name, (None, None))


def calculate_duration_from_log(log_path):
    """Calculate test duration from log file timestamps."""
    first_ts = None
    last_ts = None
    if os.path.exists(log_path):
        with open(log_path, "r", errors="ignore") as f:
            for line in f:
                current_ts = parse_timestamp_from_line(line)
                if current_ts:
                    if first_ts is None:
                        first_ts = current_ts
                    last_ts = current_ts
    if first_ts is not None and last_ts is not None and last_ts >= first_ts:
        return last_ts - first_ts
    return None


def parse_resource_analysis_output(output):
    """Parse resource_analysis.py output to extract CPU and memory averages."""
    avg_cpu = None
    avg_mem = None
    for line in output.split('\n'):
        parts = line.split()
        if len(parts) >= 4:
            # Match "Usage" line (not "Usage User" or "Usage Kernel")
            if parts[0] == 'Usage' and len(parts) > 1 and parts[1] not in ('User', 'Kernel') and '%' in line:
                try:
                    mean_str = parts[3].rstrip('%')
                    avg_cpu = float(mean_str)
                except (ValueError, IndexError):
                    pass
            # Match "Memory (MB)" line
            elif len(parts) >= 5 and parts[0] == 'Memory' and parts[1] == '(MB)':
                try:
                    mean_str = parts[4].rstrip('MB')
                    avg_mem = float(mean_str)
                except (ValueError, IndexError):
                    pass
    return avg_cpu, avg_mem


def parse_resource_log_simple(resource_log_path):
    """Simple parsing of resource.log for CPU and memory values."""
    cpu_vals = []
    mem_vals = []
    if os.path.exists(resource_log_path):
        with open(resource_log_path, "r", errors="ignore") as f:
            for line in f:
                cpu_match = CPU_ALL_RE.search(line)
                if cpu_match:
                    try:
                        cpu_vals.append(float(cpu_match.group(1)))
                    except ValueError:
                        continue
                mem_match = MEM_ALL_RE.search(line)
                if mem_match:
                    try:
                        mem_val_kb = int(mem_match.group(1))
                        mem_vals.append(mem_val_kb)
                    except ValueError:
                        continue
    avg_cpu = sum(cpu_vals) / len(cpu_vals) if cpu_vals else None
    avg_mem = sum(mem_vals) / len(mem_vals) / 1024 if mem_vals else None  # Convert to MB
    return avg_cpu, avg_mem


def parse_resource_csv(resource_csv_path):
    """Parse resource.csv for CPU and memory values."""
    cpu_vals = []
    mem_vals = []
    first_ts = None
    last_ts = None
    if os.path.exists(resource_csv_path):
        with open(resource_csv_path, "r", errors="ignore") as f:
            reader = csv.reader(f)
            next(reader, None)  # Skip header
            for row in reader:
                if len(row) < 5:
                    continue
                try:
                    ts = int(row[0])
                    cpu_vals.append(float(row[3]))
                    mem_vals.append(float(str(row[4]).split()[0]))
                    if first_ts is None:
                        first_ts = ts
                    last_ts = ts
                except ValueError:
                    continue
    avg_cpu = sum(cpu_vals) / len(cpu_vals) if cpu_vals else None
    avg_mem = sum(mem_vals) / len(mem_vals) / 1024 if mem_vals else None  # Convert to MB
    duration = last_ts - first_ts if first_ts and last_ts and last_ts >= first_ts else None
    return avg_cpu, avg_mem, duration


# ============================================================================
# Main Function
# ============================================================================
def main():
    # Parse command line arguments
    if len(sys.argv) < 2:
        base_dir = os.getcwd()
        candidates = sorted(glob.glob(os.path.join(base_dir, "logs", "itest_*")))
        if not candidates:
            print("Usage: parse_itest_logs.py <log_dir>")
            return 1
        log_dir = candidates[-1]
    else:
        log_dir = sys.argv[1]

    if not os.path.isdir(log_dir):
        print(f"Error: Log directory not found: {log_dir}")
        return 1

    # ========================================================================
    # Parse Param Timer Log
    # ========================================================================
    param_log = os.path.join(log_dir, "param_timer.log")
    param_init = False
    param_tick = 0
    if os.path.exists(param_log):
        with open(param_log, "r", errors="ignore") as f:
            for line in f:
                if "ParamTimerComponent initialized" in line:
                    param_init = True
                if "ParamTimerComponent tick" in line:
                    param_tick += 1

    integration_root = os.path.abspath(os.path.join(log_dir, "..", ".."))
    dump_dir = os.path.join(integration_root, "params", "dump")
    dump_files = glob.glob(os.path.join(dump_dir, "*.yaml"))

    # ========================================================================
    # Parse Log Files for Receive Streams
    # ========================================================================
    log_files = glob.glob(os.path.join(log_dir, "*.log"))
    if not log_files:
        print(f"Error: No log files found in: {log_dir}")
        return 1

    recv_streams = {}
    # SyncComponent statistics collected from ITEST_SYNC_* logs
    sync_stats = {}
    # Per-topic statistics from ITEST_SYNC_TOPIC_STAT logs
    sync_topic_stats = {}
    for path in log_files:
        with open(path, "r", errors="ignore") as f:
            for line in f:
                # SyncComponent summary statistics
                m_sync_stat = SYNC_STAT_RE.search(line)
                if m_sync_stat:
                    node_name, normal, latency, timeout_main, max_diff_ms = m_sync_stat.groups()
                    stats = sync_stats.setdefault(node_name, {
                        "normal": 0,
                        "latency": 0,
                        "timeout_mainframes": 0,
                        "max_abs_diff_ms": 0,
                        "timeout_events": 0,
                    })
                    stats["normal"] = int(normal)
                    stats["latency"] = int(latency)
                    stats["timeout_mainframes"] = int(timeout_main)
                    max_diff_val = int(max_diff_ms)
                    if max_diff_val > stats["max_abs_diff_ms"]:
                        stats["max_abs_diff_ms"] = max_diff_val

                # SyncComponent timeout events (per-period)
                m_sync_timeout = SYNC_TIMEOUT_RE.search(line)
                if m_sync_timeout:
                    node_name = m_sync_timeout.group(1)
                    stats = sync_stats.setdefault(node_name, {
                        "normal": 0,
                        "latency": 0,
                        "timeout_mainframes": 0,
                        "max_abs_diff_ms": 0,
                        "timeout_events": 0,
                    })
                    stats["timeout_events"] += 1

                # SyncComponent per-topic statistics
                m_sync_topic = SYNC_TOPIC_STAT_RE.search(line)
                if m_sync_topic:
                    node_name, input_name, count_str, freq_str = m_sync_topic.groups()
                    node_topics = sync_topic_stats.setdefault(node_name, {})
                    node_topics[input_name] = {
                        "count": int(count_str),
                        "freq_hz": float(freq_str),
                    }

                # Regular latency logs (ITEST_LAT)
                m = RECV_RE.search(line)
                if not m:
                    continue
                node, input_name, seq, latency_ns = m.groups()
                key = f"{node}:{input_name}"
                entry = recv_streams.setdefault(key, {
                    "seqs": [], 
                    "lat": [], 
                    "repeats": 0,
                    "first_ts": None,
                    "last_ts": None,
                })
                
                # Parse timestamp for frequency calculation
                ts = parse_timestamp_from_line(line)
                if ts is not None:
                    if entry["first_ts"] is None or ts < entry["first_ts"]:
                        entry["first_ts"] = ts
                    if entry["last_ts"] is None or ts > entry["last_ts"]:
                        entry["last_ts"] = ts
                
                seq_i = int(seq)
                if entry["seqs"] and entry["seqs"][-1] == seq_i:
                    entry["repeats"] += 1
                entry["seqs"].append(seq_i)
                entry["lat"].append(int(latency_ns))

    # ========================================================================
    # Calculate Loss and Latency Statistics
    # ========================================================================
    loss_rows = []
    latency_rows = []
    for key, data in sorted(recv_streams.items()):
        seqs = data["seqs"]
        unique_seqs = sorted(set(seqs))
        if not unique_seqs:
            continue
        
        # Extract node and topic from key
        parts = key.split(":", 1)
        node_name = parts[0] if len(parts) > 0 else ""
        topic_name = parts[1] if len(parts) > 1 else ""
        
        # Find payload and frequency
        # Priority: 1. Fixed topic config, 2. Fixed node config, 3. Calculate from data
        payload_bytes = None
        freq_hz = None
        
        # First, try fixed topic config
        topic_freq, topic_payload = get_topic_fixed_config(topic_name)
        if topic_freq is not None:
            freq_hz = topic_freq
            payload_bytes = topic_payload
        else:
            # Second, try fixed node config (for A/B/C1)
            node_freq, node_payload = get_node_config(node_name)
            if node_freq is not None:
                freq_hz = node_freq
                payload_bytes = node_payload
        
        # If still not found, calculate frequency from actual data
        if freq_hz is None and data["first_ts"] is not None and data["last_ts"] is not None:
            duration = data["last_ts"] - data["first_ts"]
            if duration > 0:
                received_unique = len(unique_seqs)
                freq_hz = received_unique / duration
        
        # Set defaults if still not found
        if payload_bytes is None:
            payload_bytes = 0
        if freq_hz is None:
            freq_hz = 0.0
        
        # Calculate statistics
        first_seq = unique_seqs[0]
        last_seq = unique_seqs[-1]
        expected = last_seq - first_seq + 1
        received_unique = len(unique_seqs)
        lost = expected - received_unique
        loss_rate = (lost / expected) if expected > 0 else 0.0
        repeat_ratio = data["repeats"] / len(seqs) if seqs else 0.0

        # Add payload and freq to loss_rows
        loss_rows.append([
            key, 
            payload_bytes,      # payload_bytes
            freq_hz,            # freq_hz
            first_seq, 
            last_seq, 
            received_unique, 
            expected, 
            lost,
            f"{loss_rate:.6f}", 
            len(seqs), 
            f"{repeat_ratio:.6f}",
        ])

        lat = sorted(data["lat"])
        avg = int(sum(lat) / len(lat)) if lat else 0
        # Add payload and freq to latency_rows
        latency_rows.append([
            key, 
            payload_bytes,      # payload_bytes
            freq_hz,            # freq_hz
            len(lat), 
            avg,
            percentile(lat, 10), 
            percentile(lat, 30), 
            percentile(lat, 50),
            percentile(lat, 90), 
            percentile(lat, 99),
            lat[0] if lat else 0, 
            lat[-1] if lat else 0,
        ])

    # ========================================================================
    # Write CSV Reports
    # ========================================================================
    loss_path = os.path.join(log_dir, "loss_report.csv")
    with open(loss_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "stream", "payload_bytes", "freq_hz", "first_seq", "last_seq", 
            "received_unique", "expected", "lost", "loss_rate", "samples_total", "repeat_ratio",
        ])
        writer.writerows(loss_rows)

    latency_path = os.path.join(log_dir, "latency_report.csv")
    with open(latency_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "stream", "payload_bytes", "freq_hz", "count", "avg_ns", "p10_ns", "p30_ns", "p50_ns",
            "p90_ns", "p99_ns", "min_ns", "max_ns",
        ])
        writer.writerows(latency_rows)

    # SyncComponent statistics report (simple summary from ITEST_SYNC_* logs)
    sync_report_path = os.path.join(log_dir, "sync_report.csv")
    with open(sync_report_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "node",
            "normal_mainframes",
            "latency_mainframes",
            "timeout_mainframes",
            "timeout_events",
            "max_abs_diff_ms",
        ])
        for node_name, stats in sorted(sync_stats.items()):
            writer.writerow([
                node_name,
                stats.get("normal", 0),
                stats.get("latency", 0),
                stats.get("timeout_mainframes", 0),
                stats.get("timeout_events", 0),
                stats.get("max_abs_diff_ms", 0),
            ])

    # Per-topic SyncComponent statistics report
    sync_topic_report_path = os.path.join(log_dir, "sync_topic_report.csv")
    with open(sync_topic_report_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "node",
            "input",
            "count",
            "freq_hz",
        ])
        for node_name, topics in sorted(sync_topic_stats.items()):
            for input_name, stats in sorted(topics.items()):
                writer.writerow([
                    node_name,
                    input_name,
                    stats.get("count", 0),
                    stats.get("freq_hz", 0.0),
                ])

    # ========================================================================
    # Parse Resource Usage
    # ========================================================================
    avg_cpu = None
    avg_mem = None
    duration_sec = None
    resource_analysis_output = ""
    
    resource_log_path = os.path.join(log_dir, "resource.log")
    resource_csv_path = os.path.join(log_dir, "resource.csv")
    
    # Try resource_analysis.py first
    if os.path.exists(resource_log_path):
        current_file = os.path.abspath(__file__)
        project_root = os.path.dirname(os.path.dirname(current_file))
        resource_analysis_script = os.path.join(project_root, "scripts", "resource_analysis.py")
        
        if os.path.exists(resource_analysis_script):
            try:
                result = run(
                    ["python3", resource_analysis_script, resource_log_path],
                    capture_output=True,
                    text=True,
                    timeout=60
                )
                if result.stdout:
                    avg_cpu, avg_mem = parse_resource_analysis_output(result.stdout)
                    # Store output for later printing
                    resource_analysis_output = result.stdout
                if result.stderr:
                    resource_analysis_output = (resource_analysis_output if 'resource_analysis_output' in locals() else "") + result.stderr
                duration_sec = calculate_duration_from_log(resource_log_path)
            except (TimeoutExpired, Exception):
                pass
    
    # Fallback to simple parsing
    if avg_cpu is None or avg_mem is None:
        if os.path.exists(resource_csv_path):
            avg_cpu, avg_mem, csv_duration = parse_resource_csv(resource_csv_path)
            if duration_sec is None:
                duration_sec = csv_duration
        elif os.path.exists(resource_log_path):
            avg_cpu, avg_mem = parse_resource_log_simple(resource_log_path)
            if duration_sec is None:
                duration_sec = calculate_duration_from_log(resource_log_path)

    # ========================================================================
    # Parse Service Check
    # ========================================================================
    service_sync = 0
    service_async = 0
    service_requests = 0
    for path in glob.glob(os.path.join(log_dir, "node_*.log")):
        with open(path, "r", errors="ignore") as f:
            for line in f:
                if " sync add_two_ints response=" in line:
                    service_sync += 1
                if " async add_two_ints response=" in line:
                    service_async += 1
                if "NodeH add_two_ints request" in line:
                    service_requests += 1

    service_check_path = os.path.join(log_dir, "service_check.txt")
    with open(service_check_path, "w", encoding="utf-8") as f:
        f.write("Service check:\n")
        f.write(f"sync_responses={service_sync}\n")
        f.write(f"async_responses={service_async}\n")
        f.write(f"server_requests={service_requests}\n")

    # ========================================================================
    # Parse Action Check
    # ========================================================================
    action_counts = {
        "server_goal": 0, "server_succeed": 0, "server_canceled": 0,
        "sync_ok": 0, "sync_timeout": 0, "async_result": 0,
        "cancel_sent": 0, "cancel_ok": 0, "cancel_fail": 0,
    }
    for path in glob.glob(os.path.join(log_dir, "node_*.log")):
        with open(path, "r", errors="ignore") as f:
            for line in f:
                if "ACTION_SERVER_GOAL" in line:
                    action_counts["server_goal"] += 1
                elif "ACTION_SERVER_SUCCEED" in line:
                    action_counts["server_succeed"] += 1
                elif "ACTION_SERVER_CANCELED" in line:
                    action_counts["server_canceled"] += 1
                elif "ACTION_SYNC_OK" in line:
                    action_counts["sync_ok"] += 1
                elif "ACTION_SYNC_TIMEOUT" in line:
                    action_counts["sync_timeout"] += 1
                elif "ACTION_ASYNC_RESULT" in line:
                    action_counts["async_result"] += 1
                elif "ACTION_CANCEL_SENT" in line:
                    action_counts["cancel_sent"] += 1
                elif "ACTION_CANCEL code=" in line:
                    action_counts["cancel_ok"] += 1
                elif "ACTION_CANCEL_FAIL" in line:
                    action_counts["cancel_fail"] += 1

    action_check_path = os.path.join(log_dir, "action_check.txt")
    with open(action_check_path, "w", encoding="utf-8") as f:
        f.write("Action check:\n")
        for key, value in action_counts.items():
            f.write(f"{key}={value}\n")

    # ========================================================================
    # Validation and Check Results
    # ========================================================================
    fail_tag = "\033[31m[fail]\033[0m"
    pass_tag = "\033[32m[pass]\033[0m"

    def status_tag(ok):
        return pass_tag if ok else fail_tag

    # Param Timer Check
    param_log_found = os.path.exists(param_log)
    dump_count = len(dump_files)
    tick_ok = False
    if duration_sec is not None:
        tick_ok = abs(param_tick - duration_sec) <= PARAM_TICK_TOLERANCE_SEC

    param_ok = param_log_found and param_init and tick_ok and dump_count >= MIN_DUMP_FILES
    param_issues = []
    if not param_log_found:
        param_issues.append("log_found=false")
    if not param_init:
        param_issues.append("initialized=false")
    if duration_sec is None:
        param_issues.append("duration_sec=none")
    elif not tick_ok:
        param_issues.append(f"tick_count_out_of_range({param_tick} vs {duration_sec}±{PARAM_TICK_TOLERANCE_SEC})")
    if dump_count < MIN_DUMP_FILES:
        param_issues.append(f"dump_files<{MIN_DUMP_FILES}")

    param_check_path = os.path.join(log_dir, "param_timer_check.txt")
    with open(param_check_path, "w", encoding="utf-8") as f:
        f.write("ParamTimerComponent check:\n")
        f.write(f"log_found={param_log_found}\n")
        f.write(f"initialized={param_init}\n")
        f.write(f"tick_count={param_tick}\n")
        f.write(f"duration_sec={duration_sec}\n")
        f.write(f"dump_dir={dump_dir}\n")
        f.write(f"dump_files={dump_count}\n")
        f.write(f"result={status_tag(param_ok)}\n")
        if param_issues:
            f.write(f"issues={','.join(param_issues)}\n")

    # Service Check
    service_responses = service_sync + service_async
    service_diff = abs(service_requests - service_responses)
    service_ok = service_diff <= MAX_SERVICE_DIFF
    service_issues = []
    if not service_ok:
        service_issues.append(f"server_requests_mismatch({service_requests} vs {service_responses}, diff={service_diff} > {MAX_SERVICE_DIFF})")

    with open(service_check_path, "a", encoding="utf-8") as f:
        f.write(f"result={status_tag(service_ok)}\n")
        if service_issues:
            f.write(f"issues={','.join(service_issues)}\n")

    # Action Check
    action_ok = (action_counts["sync_timeout"] <= MAX_ACTION_SYNC_TIMEOUT and
                 action_counts["cancel_fail"] <= MAX_ACTION_CANCEL_FAIL)
    action_issues = []
    if action_counts["sync_timeout"] > MAX_ACTION_SYNC_TIMEOUT:
        action_issues.append(f"sync_timeout>{MAX_ACTION_SYNC_TIMEOUT}({action_counts['sync_timeout']})")
    if action_counts["cancel_fail"] > MAX_ACTION_CANCEL_FAIL:
        action_issues.append(f"cancel_fail>{MAX_ACTION_CANCEL_FAIL}({action_counts['cancel_fail']})")

    with open(action_check_path, "a", encoding="utf-8") as f:
        f.write(f"result={status_tag(action_ok)}\n")
        if action_issues:
            f.write(f"issues={','.join(action_issues)}\n")

    # Topic Check
    # Note: loss_rows structure: [stream, payload_bytes, freq_hz, first_seq, last_seq, received_unique, expected, lost, loss_rate, ...]
    #       loss_rate is at index 8
    max_loss = max((float(r[8]) for r in loss_rows), default=None)
    # Note: latency_rows structure: [stream, payload_bytes, freq_hz, count, avg_ns, p10_ns, p30_ns, p50_ns, p90_ns, p99_ns, ...]
    #       p99 is at index 9, avg_ns is at index 4
    max_p99 = max((int(r[9]) for r in latency_rows), default=None)
    max_avg_latency_ns = max((int(r[4]) for r in latency_rows), default=None)
    max_avg_latency_us = max_avg_latency_ns / 1000.0 if max_avg_latency_ns is not None else None
    topic_ok = (max_loss is not None and max_p99 is not None and max_avg_latency_us is not None and
                max_loss <= MAX_LOSS_RATE and max_p99 <= MAX_P99_LATENCY_NS and
                max_avg_latency_us <= MAX_AVG_LATENCY_US)
    topic_issues = []
    if max_loss is None:
        topic_issues.append("loss_rate_missing")
    elif max_loss > MAX_LOSS_RATE:
        topic_issues.append(f"loss_rate>{MAX_LOSS_RATE:.4f}({max_loss:.6f})")
    if max_p99 is None:
        topic_issues.append("p99_missing")
    elif max_p99 > MAX_P99_LATENCY_NS:
        topic_issues.append(f"p99>{MAX_P99_LATENCY_NS}({max_p99})")
    if max_avg_latency_us is None:
        topic_issues.append("avg_latency_missing")
    elif max_avg_latency_us > MAX_AVG_LATENCY_US:
        topic_issues.append(f"avg_latency>{MAX_AVG_LATENCY_US}({max_avg_latency_us:.2f}us)")

    # Resource Check
    resource_ok = (avg_cpu is not None and avg_mem is not None and
                   avg_cpu <= MAX_AVG_CPU_PERCENT and avg_mem <= MAX_AVG_MEM_MB)
    resource_issues = []
    if avg_cpu is None or avg_mem is None:
        resource_issues.append("resource_missing")
    else:
        if avg_cpu > MAX_AVG_CPU_PERCENT:
            resource_issues.append(f"cpu>{MAX_AVG_CPU_PERCENT}({avg_cpu:.2f}%)")
        if avg_mem > MAX_AVG_MEM_MB:
            resource_issues.append(f"mem>{MAX_AVG_MEM_MB}({avg_mem:.2f}MB)")

    # ========================================================================
    # Log Error Detection
    # ========================================================================
    error_patterns = [
        r'\[E\s+',  # Error level log
        r'\[C\s+',  # Critical/Fatal level log
        r'\berror\b',
        r'\bError\b',
        r'\bERROR\b',
        r'\bexception\b',
        r'\bException\b',
        r'\bEXCEPTION\b',
        r'\bfatal\b',
        r'\bFatal\b',
        r'\bFATAL\b',
        r'\bcrash\b',
        r'\bCrash\b',
        r'\bCRASH\b',
        r'\bsegmentation fault\b',
        r'\bSegmentation fault\b',
        r'\bSEGFAULT\b',
        r'\bassert\b',
        r'\bAssert\b',
        r'\bASSERT\b',
    ]
    
    error_lines = []
    error_files = set()
    # Patterns for initialization errors that should be ignored
    init_error_patterns = [
        r'conf file \[\] not found',
        r'LoadParameters: failed to load file',
        r'failed to load file.*exception: bad file',
    ]
    
    for log_file in log_files:
        # Skip resource.log file
        if os.path.basename(log_file) == "resource.log":
            continue
        
        with open(log_file, "r", errors="ignore") as f:
            for line_num, line in enumerate(f, 1):
                # Skip initial lines to ignore initialization errors
                if line_num <= SKIP_INITIAL_ERROR_LINES:
                    # Check if it's a known initialization error pattern
                    is_init_error = False
                    for init_pattern in init_error_patterns:
                        if re.search(init_pattern, line, re.IGNORECASE):
                            is_init_error = True
                            break
                    if is_init_error:
                        continue  # Skip known initialization errors
                
                for pattern in error_patterns:
                    if re.search(pattern, line, re.IGNORECASE):
                        error_lines.append((os.path.basename(log_file), line_num, line.strip()))
                        error_files.add(os.path.basename(log_file))
                        break  # Only count each line once
    
    error_ok = len(error_lines) == 0
    error_issues = []
    if not error_ok:
        error_issues.append(f"errors_found={len(error_lines)}")
        error_issues.append(f"files={len(error_files)}")
        # Add list of files with errors
        error_file_list = sorted(list(error_files))
        if len(error_file_list) <= MAX_ERROR_FILES_DISPLAY:
            error_issues.append(f"files_list=[{', '.join(error_file_list)}]")
        else:
            error_issues.append(
                f"files_list=[{', '.join(error_file_list[:MAX_ERROR_FILES_DISPLAY])}, ... ({len(error_file_list)} total)]"
            )

    # SyncComponent Check (per-node)
    sync_ok = True
    sync_issues = {}
    max_sync_timeout_ratio = None
    max_sync_timeout_events = None
    max_sync_abs_diff_ms = None

    for node_name, cfg in SYNC_CHECK_CONFIG.items():
        node_ok = True
        issues = []
        stats = sync_stats.get(node_name)

        if not stats:
            node_ok = False
            issues.append("sync_stat_missing")
        else:
            normal = stats.get("normal", 0)
            latency = stats.get("latency", 0)
            timeout_main = stats.get("timeout_mainframes", 0)
            timeout_events = stats.get("timeout_events", 0)
            max_abs_diff_ms = stats.get("max_abs_diff_ms", 0)

            total_mainframes = normal + latency + timeout_main
            timeout_ratio = (timeout_main / total_mainframes) if total_mainframes > 0 else 0.0

            if timeout_ratio > cfg["max_timeout_mainframes_ratio"]:
                node_ok = False
                issues.append(
                    f"timeout_mainframes_ratio>{cfg['max_timeout_mainframes_ratio']:.4f}({timeout_ratio:.6f})"
                )

            if timeout_events > cfg["max_timeout_events"]:
                node_ok = False
                issues.append(
                    f"timeout_events>{cfg['max_timeout_events']}({timeout_events})"
                )

            if max_abs_diff_ms > cfg["max_abs_diff_ms"]:
                node_ok = False
                issues.append(
                    f"max_abs_diff_ms>{cfg['max_abs_diff_ms']}({max_abs_diff_ms})"
                )

            # Track global maxima for overview
            if max_sync_timeout_ratio is None or timeout_ratio > max_sync_timeout_ratio:
                max_sync_timeout_ratio = timeout_ratio
            if max_sync_timeout_events is None or timeout_events > max_sync_timeout_events:
                max_sync_timeout_events = timeout_events
            if max_sync_abs_diff_ms is None or max_abs_diff_ms > max_sync_abs_diff_ms:
                max_sync_abs_diff_ms = max_abs_diff_ms

        # Topic-level checks
        node_topics = sync_topic_stats.get(node_name)
        if not node_topics:
            node_ok = False
            issues.append("sync_topic_stat_missing")
        else:
            main_input = cfg["main_input"]
            main_info = node_topics.get(main_input)
            main_freq = None

            if not main_info or main_info.get("freq_hz", 0.0) <= 0.0:
                node_ok = False
                issues.append(f"main_input_invalid({main_input})")
            else:
                main_freq = main_info["freq_hz"]

            if main_freq is not None and main_freq > 0.0:
                # REQUIRED inputs: frequency close to main_input
                for input_name in cfg["required_inputs"]:
                    topic_info = node_topics.get(input_name)
                    if not topic_info:
                        node_ok = False
                        issues.append(f"required_input_missing({input_name})")
                        continue
                    freq = topic_info.get("freq_hz", 0.0)
                    rel_err = abs(freq - main_freq) / main_freq if main_freq > 0 else 0.0
                    if rel_err > cfg["required_freq_tolerance"]:
                        node_ok = False
                        issues.append(
                            f"required_freq_mismatch({input_name},{freq:.2f}Hz vs main {main_freq:.2f}Hz)"
                        )

                # WAITABLE inputs: coverage vs main_input
                for input_name in cfg["waitable_inputs"]:
                    topic_info = node_topics.get(input_name)
                    if not topic_info:
                        node_ok = False
                        issues.append(f"waitable_input_missing({input_name})")
                        continue
                    freq = topic_info.get("freq_hz", 0.0)
                    min_freq = cfg["waitable_min_coverage"] * main_freq
                    if freq < min_freq:
                        node_ok = False
                        issues.append(
                            f"waitable_coverage_low({input_name},{freq:.2f}Hz<{min_freq:.2f}Hz)"
                        )

                # OPTIONAL inputs: should participate at least sometimes
                for input_name in cfg["optional_inputs"]:
                    topic_info = node_topics.get(input_name)
                    if not topic_info or topic_info.get("freq_hz", 0.0) <= 0.0:
                        node_ok = False
                        issues.append(f"optional_input_no_participation({input_name})")

        sync_issues[node_name] = issues
        if not node_ok:
            sync_ok = False

    # If there is no SyncComponent data at all but we have config, mark as failed.
    if SYNC_CHECK_CONFIG and not sync_stats:
        sync_ok = False
        sync_issues["__global__"] = ["sync_data_missing"]

    # Write error report
    error_report_path = os.path.join(log_dir, "error_report.txt")
    with open(error_report_path, "w", encoding="utf-8") as f:
        if error_lines:
            f.write(f"Error Report: {len(error_lines)} error(s) found in {len(error_files)} file(s)\n")
            f.write("=" * 80 + "\n")
            for filename, line_num, line_content in error_lines[:MAX_ERROR_LINES_TRUNCATE]:
                f.write(f"{filename}:{line_num}: {line_content}\n")
            if len(error_lines) > MAX_ERROR_LINES_TRUNCATE:
                f.write(f"\n... and {len(error_lines) - MAX_ERROR_LINES_TRUNCATE} more errors (truncated)\n")
        else:
            f.write("No errors found in log files.\n")

    all_ok = (param_ok and service_ok and action_ok and topic_ok and
              resource_ok and error_ok and sync_ok)

    # ========================================================================
    # Generate Markdown Summary
    # ========================================================================
    out_lines = []

    def emit(line=""):
        """Emit a line to the Markdown output."""
        out_lines.append(line)

    # Header
    emit("## Integration Test Summary")
    emit("")
    emit(f"- **log_dir**: `{log_dir}`")
    emit("- **generated files**:")
    emit("  - `loss_report.csv`")
    emit("  - `latency_report.csv`")
    emit("  - `sync_report.csv`")
    emit("  - `sync_topic_report.csv`")
    emit("  - `service_check.txt`")
    emit("  - `action_check.txt`")
    emit("  - `param_timer_check.txt`")
    emit("  - `error_report.txt`")
    emit("")
    emit(f"**Overall Result**: {'✅ INTEGRATION TEST ALL PASS' if all_ok else '❌ INTEGRATION TEST FAIL'}")
    emit("")
    emit("---")
    emit("")

    # Overview table
    emit("### Overview")
    emit("")
    emit("| Item            | Status | Key Metrics / Notes |")
    emit("|-----------------|:------:|---------------------|")

    # Param Timer overview
    param_note = f"tick={param_tick}, duration={duration_sec}, dumps={dump_count}"
    if param_issues:
        param_note += f" | {', '.join(param_issues)}"
    emit(f"| Param Timer     | {status_icon(param_ok)} | {param_note} |")

    # Service overview
    service_note = f"requests={service_requests}, sync={service_sync}, async={service_async}, diff={service_diff}"
    if service_issues:
        service_note += f" | {', '.join(service_issues)}"
    emit(f"| Service         | {status_icon(service_ok)} | {service_note} |")

    # Action overview
    action_note = f"sync_timeout={action_counts['sync_timeout']}, cancel_fail={action_counts['cancel_fail']}"
    if action_issues:
        action_note += f" | {', '.join(action_issues)}"
    emit(f"| Action          | {status_icon(action_ok)} | {action_note} |")

    # Topic overview
    if max_loss is not None and max_p99 is not None and max_avg_latency_us is not None:
        # Calculate frequency range from latency_rows
        all_freqs = [float(r[2]) for r in latency_rows if len(r) > 2 and r[2] is not None and r[2] > 0]
        if all_freqs:
            min_freq = min(all_freqs)
            max_freq = max(all_freqs)
            topic_note = f"max_loss={max_loss:.6f}, max_p99={max_p99/1000.0:.2f}us, max_avg={max_avg_latency_us:.2f}us, freq_range={min_freq:.1f}-{max_freq:.1f}Hz"
        else:
            topic_note = f"max_loss={max_loss:.6f}, max_p99={max_p99/1000.0:.2f}us, max_avg={max_avg_latency_us:.2f}us"
    else:
        topic_note = "no topic data"
    if topic_issues:
        topic_note += f" | {', '.join(topic_issues)}"
    emit(f"| Topic           | {status_icon(topic_ok)} | {topic_note} |")

    # SyncComponent overview
    if max_sync_timeout_ratio is not None and max_sync_abs_diff_ms is not None:
        sync_note = (
            f"max_timeout_ratio={max_sync_timeout_ratio:.6f}, "
            f"max_timeout_events={max_sync_timeout_events if max_sync_timeout_events is not None else 0}, "
            f"max_abs_diff_ms={max_sync_abs_diff_ms}"
        )
    else:
        sync_note = "no sync data"
    sync_issue_summaries = []
    for node_name, issues in sorted(sync_issues.items()):
        if not issues:
            continue
        sync_issue_summaries.append(
            f"{node_name}: {', '.join(issues)}"
        )
    if sync_issue_summaries:
        sync_note += " | " + " ; ".join(sync_issue_summaries)
    emit(f"| SyncComponent   | {status_icon(sync_ok)} | {sync_note} |")

    # Resource overview
    if avg_cpu is not None and avg_mem is not None:
        resource_note = f"cpu={avg_cpu:.2f}%, mem={avg_mem:.2f}MB"
    else:
        resource_note = "resource data missing"
    if resource_issues:
        resource_note += f" | {', '.join(resource_issues)}"
    emit(f"| Resource usage  | {status_icon(resource_ok)} | {resource_note} |")

    # Error overview
    if error_ok:
        error_note = "0 errors"
    else:
        error_file_list = sorted(list(error_files))
        if len(error_file_list) <= 3:
            files_str = ", ".join(error_file_list)
        else:
            files_str = ", ".join(error_file_list[:3]) + f", ... ({len(error_file_list)} files)"
        error_note = f"{len(error_lines)} errors in [{files_str}]"
    emit(f"| Log Errors      | {status_icon(error_ok)} | {error_note} |")
    emit("")

    # ParamTimer detail
    emit("### ParamTimerComponent")
    emit("")
    emit(f"- **result**: {'✅ PASS' if param_ok else '❌ FAIL'}")
    emit(f"- **log_found**: `{param_log_found}`")
    emit(f"- **initialized**: `{param_init}`")
    emit(f"- **tick_count**: `{param_tick}`")
    emit(f"- **duration_sec**: `{duration_sec}`")
    emit(f"- **dump_dir**: `{dump_dir}`")
    emit(f"- **dump_files**: `{dump_count}`")
    if param_issues:
        emit("")
        emit("- **issues**:")
        for issue in param_issues:
            emit(f"  - `{issue}`")
    emit("")

    # Service detail
    emit("### Service Check")
    emit("")
    emit(f"- **result**: {'✅ PASS' if service_ok else '❌ FAIL'}")
    emit(f"- **server_requests**: `{service_requests}`")
    emit(f"- **sync_responses**: `{service_sync}`")
    emit(f"- **async_responses**: `{service_async}`")
    emit(f"- **diff**: `{service_diff}` (threshold: `≤ {MAX_SERVICE_DIFF}`)")
    emit("")
    emit("#### Service Counters")
    emit("")
    emit("| Metric          | Value |")
    emit("|-----------------|------:|")
    emit(f"| server_requests | {service_requests} |")
    emit(f"| sync_responses  | {service_sync} |")
    emit(f"| async_responses | {service_async} |")
    emit(f"| diff            | {service_diff} |")
    emit("")
    if service_issues:
        emit("- **issues**:")
        for issue in service_issues:
            emit(f"  - `{issue}`")
    emit("")

    # Action detail
    emit("### Action Check")
    emit("")
    emit(f"- **result**: {'✅ PASS' if action_ok else '❌ FAIL'}")
    emit("- **constraints**:")
    emit(f"  - sync_timeout ≤ `{MAX_ACTION_SYNC_TIMEOUT}`")
    emit(f"  - cancel_fail ≤ `{MAX_ACTION_CANCEL_FAIL}`")
    emit("")
    emit("#### Action Counters")
    emit("")
    emit("| Metric          | Value |")
    emit("|-----------------|------:|")
    emit(f"| server_goal     | {action_counts['server_goal']} |")
    emit(f"| server_succeed  | {action_counts['server_succeed']} |")
    emit(f"| server_canceled | {action_counts['server_canceled']} |")
    emit(f"| sync_ok         | {action_counts['sync_ok']} |")
    emit(f"| sync_timeout    | {action_counts['sync_timeout']} |")
    emit(f"| async_result    | {action_counts['async_result']} |")
    emit(f"| cancel_sent     | {action_counts['cancel_sent']} |")
    emit(f"| cancel_ok       | {action_counts['cancel_ok']} |")
    emit(f"| cancel_fail     | {action_counts['cancel_fail']} |")
    emit("")
    if action_issues:
        emit("- **issues**:")
        for issue in action_issues:
            emit(f"  - `{issue}`")
    emit("")

    # Topic detail
    emit("### Topic Loss & Latency")
    emit("")
    emit(f"- **result**: {'✅ PASS' if topic_ok else '❌ FAIL'}")
    if max_loss is not None:
        emit(f"- **max_loss_rate**: `{max_loss:.6f}` (threshold: `≤ {MAX_LOSS_RATE:.4f}`)")
    else:
        emit("- **max_loss_rate**: `N/A`")
    if max_p99 is not None:
        emit(f"- **max_p99_latency**: `{max_p99/1000.0:.2f} us` (threshold: `≤ {MAX_P99_LATENCY_NS/1000.0:.2f} us`)")
    else:
        emit("- **max_p99_latency**: `N/A`")
    if max_avg_latency_us is not None:
        emit(f"- **max_avg_latency**: `{max_avg_latency_us:.2f} us` (threshold: `≤ {MAX_AVG_LATENCY_US:.2f} us`)")
    else:
        emit("- **max_avg_latency**: `N/A`")
    emit("")

    # Loss streams (all, sorted by loss rate)
    if loss_rows:
        # loss_rows structure: [stream, payload_bytes, freq_hz, first_seq, last_seq, received_unique, expected, lost, loss_rate, samples_total, repeat_ratio]
        # loss_rate is at index 8
        loss_sorted = sorted(loss_rows, key=lambda r: float(r[8]), reverse=True)
        emit("#### Loss streams (sorted by loss rate)")
        emit("")
        emit("> Data from `loss_report.csv`")
        emit("")
        emit("| Stream                  | Payload(KB) | Freq(Hz) | Loss Rate | Lost | Expected | Received | Total | Repeat Ratio |")
        emit("|-------------------------|-----------:|---------:|----------:|-----:|---------:|---------:|------:|-------------:|")
        for row in loss_sorted:
            stream = row[0]
            payload_bytes = int(row[1]) if len(row) > 1 else 0
            freq_hz = float(row[2]) if len(row) > 2 else 0.0
            received = int(row[5]) if len(row) > 5 else 0
            expected = int(row[6]) if len(row) > 6 else 0
            lost = int(row[7]) if len(row) > 7 else 0
            loss_rate = float(row[8]) if len(row) > 8 else 0.0
            total = int(row[9]) if len(row) > 9 else 0
            repeat_ratio = float(row[10]) if len(row) > 10 else 0.0
            emit(f"| `{stream}` | {payload_bytes} | {freq_hz:.2f} | {loss_rate:.6f} | {lost} | {expected} | {received} | {total} | {repeat_ratio:.6f} |")
        emit("")

    # Latency streams (all, sorted by average latency)
    if latency_rows:
        # latency_rows structure: [stream, payload_bytes, freq_hz, count, avg_ns, p10_ns, p30_ns, p50_ns, p90_ns, p99_ns, min_ns, max_ns]
        # avg_ns is at index 4
        lat_sorted = sorted(latency_rows, key=lambda r: int(r[4]), reverse=True)
        emit("#### Latency streams (sorted by average latency)")
        emit("")
        emit("> Data from `latency_report.csv`")
        emit("")
        emit("| Stream                  | Payload(KB) | Freq(Hz) | Count | Avg(us) | p10(us) | p30(us) | p50(us) | p90(us) | p99(us) | Min(us) | Max(us) |")
        emit("|-------------------------|-----------:|---------:|------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|")
        for row in lat_sorted:
            stream = row[0]
            payload_bytes = int(row[1]) if len(row) > 1 else 0
            freq_hz = float(row[2]) if len(row) > 2 else 0.0
            count = int(row[3]) if len(row) > 3 else 0
            avg = int(row[4]) if len(row) > 4 else 0
            p10 = int(row[5]) if len(row) > 5 else 0
            p30 = int(row[6]) if len(row) > 6 else 0
            p50 = int(row[7]) if len(row) > 7 else 0
            p90 = int(row[8]) if len(row) > 8 else 0
            p99 = int(row[9]) if len(row) > 9 else 0
            min_val = int(row[10]) if len(row) > 10 else 0
            max_val = int(row[11]) if len(row) > 11 else 0
            emit(f"| `{stream}` | {payload_bytes} | {freq_hz:.2f} | {count} | {avg/1000.0:.2f} | {p10/1000.0:.2f} | {p30/1000.0:.2f} | {p50/1000.0:.2f} | {p90/1000.0:.2f} | {p99/1000.0:.2f} | {min_val/1000.0:.2f} | {max_val/1000.0:.2f} |")
        emit("")

    if topic_issues:
        emit("- **issues**:")
        for issue in topic_issues:
            emit(f"  - `{issue}`")
        emit("")

    # SyncComponent detail
    emit("### SyncComponent Check")
    emit("")
    emit(f"- **result**: {'✅ PASS' if sync_ok else '❌ FAIL'}")
    emit("")

    # Per-node Sync statistics
    if sync_stats:
        emit("#### SyncComponent Summary (per node)")
        emit("")
        emit("| Node | normal_mainframes | latency_mainframes | timeout_mainframes | timeout_events | max_abs_diff_ms |")
        emit("|------|------------------:|-------------------:|-------------------:|---------------:|----------------:|")
        for node_name, stats in sorted(sync_stats.items()):
            normal = stats.get("normal", 0)
            latency = stats.get("latency", 0)
            timeout_main = stats.get("timeout_mainframes", 0)
            timeout_events = stats.get("timeout_events", 0)
            max_diff = stats.get("max_abs_diff_ms", 0)
            emit(
                f"| `{node_name}` | {normal} | {latency} | {timeout_main} | {timeout_events} | {max_diff} |"
            )
        emit("")

    # Per-node topic statistics
    if sync_topic_stats:
        emit("#### SyncComponent Topic Participation")
        emit("")
        emit("| Node | Input | Count | Freq(Hz) |")
        emit("|------|-------|------:|---------:|")
        for node_name, topics in sorted(sync_topic_stats.items()):
            for input_name, stats in sorted(topics.items()):
                count = stats.get("count", 0)
                freq = stats.get("freq_hz", 0.0)
                emit(
                    f"| `{node_name}` | `{input_name}` | {count} | {freq:.2f} |"
                )
        emit("")

    # Sync issues detail
    sync_issue_items = [
        (node_name, issues)
        for node_name, issues in sorted(sync_issues.items())
        if issues
    ]
    if sync_issue_items:
        emit("- **issues**:")
        for node_name, issues in sync_issue_items:
            emit(f"  - `{node_name}`: " + ", ".join(f"`{i}`" for i in issues))
        emit("")

    # Resource detail
    emit("### Resource Usage")
    emit("")
    emit(f"- **result**: {'✅ PASS' if resource_ok else '❌ FAIL'}")
    if avg_cpu is not None:
        emit(f"- **avg_cpu**: `{avg_cpu:.2f}%` (threshold: `≤ {MAX_AVG_CPU_PERCENT}%`)")
    else:
        emit("- **avg_cpu**: `N/A`")
    if avg_mem is not None:
        emit(f"- **avg_mem**: `{avg_mem:.2f} MB` (threshold: `≤ {MAX_AVG_MEM_MB} MB`)")
    else:
        emit("- **avg_mem**: `N/A`")
    emit("")
    if resource_issues:
        emit("- **issues**:")
        for issue in resource_issues:
            emit(f"  - `{issue}`")
    if resource_analysis_output:
        emit("")
        emit("#### Raw resource_analysis.py output")
        emit("")
        emit("```text")
        for line in resource_analysis_output.split('\n'):
            emit(line)
        emit("```")
    emit("")

    # Error detail
    emit("### Log Errors")
    emit("")
    emit(f"- **result**: {'✅ PASS' if error_ok else '❌ FAIL'}")
    emit(f"- **total_errors**: `{len(error_lines)}`")
    emit(f"- **files_with_errors**: `{len(error_files)}`")
    emit("")
    if not error_ok:
        error_file_list = sorted(list(error_files))
        emit(f"#### Files (up to {MAX_ERROR_FILES_DISPLAY} shown)")
        emit("")
        for fname in error_file_list[:MAX_ERROR_FILES_DISPLAY]:
            emit(f"- `{fname}`")
        if len(error_file_list) > MAX_ERROR_FILES_DISPLAY:
            emit(f"- `... ({len(error_file_list)} files total)`")
        emit("")
        emit(f"#### First {MAX_ERROR_LINES_DISPLAY} error hits")
        emit("")
        emit("| # | File | Line | Message |")
        emit("|---|------|------|---------|")
        for idx, (fname, ln, text) in enumerate(error_lines[:MAX_ERROR_LINES_DISPLAY], start=1):
            safe_text = text.replace("|", "\\|").replace("\n", " ")
            emit(f"| {idx} | `{fname}` | {ln} | {safe_text} |")
        if len(error_lines) > MAX_ERROR_LINES_DISPLAY:
            emit("")
            emit(
                f"> Only first {MAX_ERROR_LINES_DISPLAY} errors shown (total {len(error_lines)}). "
                "See `error_report.txt` for full details."
            )
    emit("")
    emit("> Full details in `error_report.txt`")
    emit("")

    # Final verdict
    emit("---")
    emit("")
    emit("### Final Verdict")
    emit("")
    emit(f"**Overall Result**: {'✅ INTEGRATION TEST ALL PASS' if all_ok else '❌ INTEGRATION TEST FAIL'}")
    emit("")
    if not all_ok:
        emit("#### Failed items")
        emit("")
        failed_checks = []
        if not param_ok:
            failed_checks.append(f"- **Param Timer**: `{', '.join(param_issues) if param_issues else 'unknown'}`")
        if not service_ok:
            failed_checks.append(f"- **Service**: `{', '.join(service_issues) if service_issues else 'unknown'}`")
        if not action_ok:
            failed_checks.append(f"- **Action**: `{', '.join(action_issues) if action_issues else 'unknown'}`")
        if not topic_ok:
            failed_checks.append(f"- **Topic**: `{', '.join(topic_issues) if topic_issues else 'unknown'}`")
        if not resource_ok:
            failed_checks.append(f"- **Resource**: `{', '.join(resource_issues) if resource_issues else 'unknown'}`")
        if not error_ok:
            error_file_list = sorted(list(error_files))
            files_str = ", ".join(error_file_list) if len(error_file_list) <= 10 else ", ".join(error_file_list[:10]) + f", ... ({len(error_file_list)} files)"
            failed_checks.append(f"- **Log Errors**: `{len(error_lines)} errors in {len(error_files)} file(s) - [{files_str}]`")
        emit("\n".join(failed_checks))
        emit("")
    emit("")

    # Write Markdown file
    md_path = os.path.join(log_dir, "itest_summary.md")
    try:
        with open(md_path, "w", encoding="utf-8") as f_md:
            f_md.write("\n".join(out_lines) + "\n")
        print(f"Markdown summary written to: {md_path}")
    except OSError as e:
        print(f"Warning: Failed to write markdown file: {e}", file=sys.stderr)

    # ========================================================================
    # Print Summary (also print to terminal for backward compatibility)
    # ========================================================================
    # Print resource_analysis.py output if available
    if resource_analysis_output:
        print("\n" + "="*80)
        print("Resource Analysis Output:")
        print("="*80)
        print(resource_analysis_output)
        print("="*80)
    
    # Print Markdown to terminal
    for line in out_lines:
        print(line)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
