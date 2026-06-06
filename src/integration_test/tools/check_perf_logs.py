#!/usr/bin/env python3
# ****************************************************************************
# Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
# SPDX-License-Identifier: LicenseRef-Segar-Proprietary
#
# PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
# for license terms and restrictions.
# ****************************************************************************

import argparse
import glob
import json
import os
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

# =============================================================================
# Configuration: Thresholds and expected values (override via CLI when needed)
# =============================================================================
# Expected totals from perf test (fixed run sizes)
EXPECTED_SERVICE_TOTAL_CALLS = 80000
EXPECTED_ACTION_TOTAL_SUCCESS = 4400

# =============================================================================
def status_icon(ok: bool) -> str:
    return "✅" if ok else "❌"


# Error patterns (matched case-insensitively, aligned with parse_itest_logs.py)
ERROR_PATTERNS = [
    r"\[E\s+",  # Error level log
    r"\[C\s+",  # Critical/Fatal level log
    r"\berror\b",
    r"\bexception\b",
    r"\bfatal\b",
    r"\bcrash\b",
    r"\bsegmentation fault\b",
    r"\bSEGFAULT\b",
    r"\bassert\b",
]

# Skip initial lines in log files for error detection (to ignore initialization errors)
SKIP_INITIAL_ERROR_LINES = 50

# Updated ACTION_LINE_RE to be more flexible
ACTION_LINE_RE = re.compile(
    r".*(?:\[SEGAR\]\[SYNC\]|INFO).*payload=(?P<payload>\d+)B.*iter=(?P<iter>\d+).*sent=(?P<sent>\d+).*success=(?P<success>\d+).*"
    r"loss=(?P<loss>[\d.]+).*thr=(?P<thr_mbps>[\d.]+).*MB/s.*avg=(?P<avg_ms>[\d.]+).*ms.*"
    r"p10=(?P<p10_ms>[\d.]+).*ms.*p50=(?P<p50_ms>[\d.]+).*ms.*p90=(?P<p90_ms>[\d.]+).*ms.*p99=(?P<p99_ms>[\d.]+).*ms.*max=(?P<max_ms>[\d.]+).*ms.*$"
)
TOPIC_DONE_RE = re.compile(r"All payload tests done\.")

# Updated TOPIC_LINE_RE to be more flexible for different log formats
TOPIC_LINE_RE = re.compile(
    r".*\[Segar\].*payload=(?P<payload>\d+)B\s+iters=(?P<iters>\d+)\s+elapsed=(?P<elapsed_s>[\d.]+)s\s+"
    r"TPS=(?P<tps>[\d.]+)\s+Throughput=(?P<thr_mbps>[\d.]+)\s+MB/s\s+\|\s+"
    r"avg=(?P<avg_us>[\d.]+)(?:us)?\s+p10=(?P<p10_us>[\d.]+)(?:us)?\s+p30=(?P<p30_us>[\d.]+)(?:us)?\s+"
    r"p50=(?P<p50_us>[\d.]+)(?:us)?\s+p90=(?P<p90_us>[\d.]+)(?:us)?\s+"
    r"p99=(?P<p99_us>[\d.]+)(?:us)?\s+max=(?P<max_us>[\d.]+)(?:us)?\s*$"
)

# More flexible patterns for service logs
SERVICE_CALLS_RE = re.compile(
    r"^calls=(?P<calls>\d+)[,\s]+success=(?P<success>\d+)[,\s]+failed=(?P<failed>\d+)[,\s]+seconds=(?P<seconds>[\d.]+)[,\s]+TPS=(?P<tps>[\d.]+)\s*$"
)
SERVICE_LAT_RE = re.compile(
    r"^latency\(us\):.*min=(?P<min_us>[\d.]+).*avg=(?P<avg_us>[\d.]+).*stddev=(?P<stddev_us>[\d.]+).*"
    r"p10=(?P<p10_us>[\d.]+).*p50=(?P<p50_us>[\d.]+).*p90=(?P<p90_us>[\d.]+).*p99=(?P<p99_us>[\d.]+).*p99\.9=(?P<p999_us>[\d.]+).*max=(?P<max_us>[\d.]+).*$"
)
# Async client: "Round end: payload_size=N total_time_ms=... responses=... send_failures=... completed=yes|timeout"
SERVICE_ASYNC_ROUND_END_RE = re.compile(
    r".*Round end:\s*payload_size=(?P<payload>\d+)\s+total_time_ms=(?P<total_time_ms>\d+)\s+"
    r"responses=(?P<responses>\d+)\s+send_failures=(?P<send_failures>\d+)\s+completed=(?P<completed>yes|timeout)"
)
# Latency line that may have log prefix (for async AINFO output)
SERVICE_LAT_FLEXIBLE_RE = re.compile(
    r"latency\(us\):.*min=(?P<min_us>[\d.]+).*avg=(?P<avg_us>[\d.]+).*stddev=(?P<stddev_us>[\d.]+).*"
    r"p10=(?P<p10_us>[\d.]+).*p50=(?P<p50_us>[\d.]+).*p90=(?P<p90_us>[\d.]+).*p99=(?P<p99_us>[\d.]+).*p99\.9=(?P<p999_us>[\d.]+).*max=(?P<max_us>[\d.]+)"
)

# Fallback patterns when primary regex does not match (compiled once at module load)
TOPIC_LINE_FALLBACK_RE = re.compile(
    r"payload=(?P<payload>\d+)B\s+iters=(?P<iters>\d+)\s+elapsed=(?P<elapsed_s>[\d.]+)s\s+"
    r"TPS=(?P<tps>[\d.]+)\s+Throughput=(?P<thr_mbps>[\d.]+)\s+MB/s\s+\|\s+"
    r"avg=(?P<avg_us>[\d.]+)(?:us)?(\s+p10=(?P<p10_us>[\d.]+)(?:us)?)?(\s+p30=(?P<p30_us>[\d.]+)(?:us)?)?\s+"
    r"p50=(?P<p50_us>[\d.]+)(?:us)?\s+p90=(?P<p90_us>[\d.]+)(?:us)?\s+"
    r"p99=(?P<p99_us>[\d.]+)(?:us)?\s+max=(?P<max_us>[\d.]+)(?:us)?"
)
ACTION_LINE_FALLBACK_RE = re.compile(
    r"payload=(?P<payload>\d+)B.*iter=(?P<iter>\d+).*sent=(?P<sent>\d+).*success=(?P<success>\d+).*"
    r"loss=(?P<loss>[\d.]+).*thr=(?P<thr_mbps>[\d.]+).*avg=(?P<avg_ms>[\d.]+).*"
    r"p10=(?P<p10_ms>[\d.]+).*p50=(?P<p50_ms>[\d.]+).*p90=(?P<p90_ms>[\d.]+).*p99=(?P<p99_ms>[\d.]+)"
)


@dataclass
class TopicRecord:
    payload: int
    iters: int
    elapsed_s: float
    tps: float
    thr_mbps: float
    p10_us: float
    p30_us: float
    p50_us: float
    p90_us: float
    p99_us: float
    avg_us: float
    max_us: float


@dataclass
class ServiceRecord:
    payload: Optional[int]
    calls: int
    success: int
    failed: int
    seconds: float
    tps: float
    p10_us: float
    p50_us: float
    p90_us: float
    p99_us: float
    avg_us: float
    max_us: float


@dataclass
class ActionRecord:
    payload: int
    iters: int
    sent: int
    success: int
    loss: float
    thr_mbps: float
    p10_ms: float
    p50_ms: float
    p90_ms: float
    p99_ms: float
    avg_ms: float
    max_ms: float


def _read_lines(path: str) -> List[str]:
    with open(path, "r", errors="ignore") as f:
        return f.read().splitlines()


def find_latest_log_dir(tool_dir: str) -> Optional[str]:
    candidates = sorted(glob.glob(os.path.join(tool_dir, "logs", "perf_*")))
    return candidates[-1] if candidates else None


def scan_errors(log_files: List[str]) -> List[Tuple[str, int, str]]:
    res: List[Tuple[str, int, str]] = []
    patterns = [re.compile(p, re.IGNORECASE) for p in ERROR_PATTERNS]
    for path in log_files:
        for idx, line in enumerate(_read_lines(path), start=1):
            if idx <= SKIP_INITIAL_ERROR_LINES:
                continue  # Ignore errors in initial lines (e.g. initialization)
            if any(p.search(line) for p in patterns):
                res.append((os.path.basename(path), idx, line.strip()))
    return res


def parse_topic(log_path: str) -> Tuple[List[TopicRecord], bool]:
    records: List[TopicRecord] = []
    done = False
    if not os.path.exists(log_path):
        return records, done
        
    for line in _read_lines(log_path):
        if TOPIC_DONE_RE.search(line):
            done = True
            
        m = TOPIC_LINE_RE.match(line.strip())
        if not m:
            m = TOPIC_LINE_FALLBACK_RE.search(line.strip())
        
        if m:
            d = m.groupdict()
            # 确保所有必需的字段都有值
            try:
                records.append(
                    TopicRecord(
                        payload=int(d["payload"]),
                        iters=int(d["iters"]),
                        elapsed_s=float(d["elapsed_s"]),
                        tps=float(d["tps"]),
                        thr_mbps=float(d["thr_mbps"]),
                        p10_us=float(d.get("p10_us") or 0.0),
                        p30_us=float(d.get("p30_us") or 0.0),
                        p50_us=float(d["p50_us"]),
                        p90_us=float(d["p90_us"]),
                        p99_us=float(d["p99_us"]),
                        avg_us=float(d["avg_us"]),
                        max_us=float(d.get("max_us") or 0.0),
                    )
                )
            except (ValueError, TypeError, KeyError):
                continue
    return records, done


def parse_service(log_path: str) -> List[ServiceRecord]:
    recs: List[ServiceRecord] = []
    if not os.path.exists(log_path):
        return recs
    
    lines = _read_lines(log_path)
    i = 0
    while i < len(lines):
        # 查找标题行，格式为 "==== Segar Single-Concurrency | payload=XXB ===="
        line = lines[i].strip()
        if not line or "====" not in line:
            i += 1
            continue
            
        # 检查是否包含 "Segar Single-Concurrency" 和 "payload"
        if "Segar Single-Concurrency" in line and "payload=" in line:
            # 从标题行提取负载大小
            payload_match = re.search(r"payload=(\d+)B", line)
            payload = int(payload_match.group(1)) if payload_match else None

            # 下一行应该是calls行
            i += 1
            if i < len(lines):
                calls_line = lines[i].strip()
                calls_match = SERVICE_CALLS_RE.match(calls_line)
                
                # 再下一行应该是latency行
                i += 1
                if i < len(lines):
                    lat_line = lines[i].strip()
                    lat_match = SERVICE_LAT_RE.match(lat_line)
                    
                    if calls_match and lat_match:
                        d1 = calls_match.groupdict()
                        d2 = lat_match.groupdict()
                        
                        try:
                            recs.append(
                                ServiceRecord(
                                    payload=payload,
                                    calls=int(d1["calls"]),
                                    success=int(d1["success"]),
                                    failed=int(d1["failed"]),
                                    seconds=float(d1["seconds"]),
                                    tps=float(d1["tps"]),
                                    p10_us=float(d2["p10_us"]),
                                    p50_us=float(d2["p50_us"]),
                                    p90_us=float(d2["p90_us"]),
                                    p99_us=float(d2["p99_us"]),
                                    avg_us=float(d2["avg_us"]),
                                    max_us=float(d2["max_us"]),
                                )
                            )
                        except (ValueError, TypeError, KeyError):
                            pass
        i += 1
    return recs


# Async client uses kTotalCalls=1000 per round
SERVICE_ASYNC_TOTAL_CALLS_PER_ROUND = 1000


def parse_service_async(log_path: str) -> List[ServiceRecord]:
    """Parse service_client_async.log: Round end + latency(us) lines per payload."""
    recs: List[ServiceRecord] = []
    if not os.path.exists(log_path):
        return recs
    lines = _read_lines(log_path)
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        round_m = SERVICE_ASYNC_ROUND_END_RE.search(line)
        if round_m:
            i += 1
            if i < len(lines):
                lat_line = lines[i].strip()
                lat_m = SERVICE_LAT_FLEXIBLE_RE.search(lat_line)
                if lat_m:
                    d1 = round_m.groupdict()
                    d2 = lat_m.groupdict()
                    try:
                        total_time_ms = int(d1["total_time_ms"])
                        seconds = total_time_ms / 1000.0
                        responses = int(d1["responses"])
                        send_failures = int(d1["send_failures"])
                        tps = responses / seconds if seconds > 0 else 0.0
                        recs.append(
                            ServiceRecord(
                                payload=int(d1["payload"]),
                                calls=SERVICE_ASYNC_TOTAL_CALLS_PER_ROUND,
                                success=responses,
                                failed=send_failures,
                                seconds=seconds,
                                tps=tps,
                                p10_us=float(d2["p10_us"]),
                                p50_us=float(d2["p50_us"]),
                                p90_us=float(d2["p90_us"]),
                                p99_us=float(d2["p99_us"]),
                                avg_us=float(d2["avg_us"]),
                                max_us=float(d2["max_us"]),
                            )
                        )
                    except (ValueError, TypeError, KeyError):
                        pass
        i += 1
    return recs


def parse_action(log_path: str) -> List[ActionRecord]:
    recs: List[ActionRecord] = []
    if not os.path.exists(log_path):
        return recs
        
    for line in _read_lines(log_path):
        # 尝试原始的正则表达式
        m = ACTION_LINE_RE.search(line.strip())
        
        if not m:
            m = ACTION_LINE_FALLBACK_RE.search(line.strip())

        if m:
            d = m.groupdict()
            try:
                recs.append(
                    ActionRecord(
                        payload=int(d["payload"]),
                        iters=int(d["iter"]),
                        sent=int(d["sent"]),
                        success=int(d["success"]),
                        loss=float(d["loss"]),
                        thr_mbps=float(d["thr_mbps"]),
                        p10_ms=float(d["p10_ms"]),
                        p50_ms=float(d["p50_ms"]),
                        p90_ms=float(d["p90_ms"]),
                        p99_ms=float(d["p99_ms"]),
                        avg_ms=float(d["avg_ms"]),
                        max_ms=float(d.get("max_ms") or d.get("p99_ms") or 0.0),
                    )
                )
            except (ValueError, TypeError, KeyError):
                continue
    return recs


def check_thresholds(
    topic: List[TopicRecord],
    service: List[ServiceRecord],
    action: List[ActionRecord],
    args: argparse.Namespace,
) -> Tuple[bool, List[str]]:
    ok = True
    issues: List[str] = []

    if args.topic_max_p99_us is not None:
        mx = max((r.p99_us for r in topic), default=None)
        if mx is None or mx > args.topic_max_p99_us:
            ok = False
            issues.append(f"topic_p99_us>{args.topic_max_p99_us}({mx})")
    if args.topic_min_throughput_mbps is not None:
        mn = min((r.thr_mbps for r in topic), default=None)
        if mn is None or mn < args.topic_min_throughput_mbps:
            ok = False
            issues.append(f"topic_thr_mbps<{args.topic_min_throughput_mbps}({mn})")
    
    # Check if topic records match expected iterations
    if hasattr(args, 'topic_min_iters') and args.topic_min_iters is not None:
        min_iters = min((r.iters for r in topic), default=None)
        if min_iters is None or min_iters < args.topic_min_iters:
            ok = False
            issues.append(f"topic_iters<{args.topic_min_iters}({min_iters})")

    if args.service_max_p99_us is not None:
        mx = max((r.p99_us for r in service), default=None)
        if mx is None or mx > args.service_max_p99_us:
            ok = False
            issues.append(f"service_p99_us>{args.service_max_p99_us}({mx})")
    if args.service_min_tps is not None:
        mn = min((r.tps for r in service), default=None)
        if mn is None or mn < args.service_min_tps:
            ok = False
            issues.append(f"service_tps<{args.service_min_tps}({mn})")
    
    # Check service success rate
    if hasattr(args, 'service_min_success_rate') and args.service_min_success_rate is not None:
        if service:
            total_calls = sum(s.calls for s in service)
            total_success = sum(s.success for s in service)
            if total_calls > 0:
                success_rate = total_success / total_calls
                if success_rate < args.service_min_success_rate:
                    ok = False
                    issues.append(f"service_success_rate<{args.service_min_success_rate}({success_rate})")

    if args.action_max_p99_ms is not None:
        mx = max((r.p99_ms for r in action), default=None)
        if mx is None or mx > args.action_max_p99_ms:
            ok = False
            issues.append(f"action_p99_ms>{args.action_max_p99_ms}({mx})")
    if args.action_max_loss is not None:
        mx = max((r.loss for r in action), default=None)
        if mx is None or mx > args.action_max_loss:
            ok = False
            issues.append(f"action_loss>{args.action_max_loss}({mx})")
    if args.action_min_throughput_mbps is not None:
        mn = min((r.thr_mbps for r in action), default=None)
        if mn is None or mn < args.action_min_throughput_mbps:
            ok = False
            issues.append(
                f"action_thr_mbps<{args.action_min_throughput_mbps}({mn})"
            )

    # Check action success rate
    if hasattr(args, 'action_min_success_rate') and args.action_min_success_rate is not None:
        if action:
            total_sent = sum(a.sent for a in action)
            total_success = sum(a.success for a in action)
            if total_sent > 0:
                success_rate = total_success / total_sent
                if success_rate < args.action_min_success_rate:
                    ok = False
                    issues.append(f"action_success_rate<{args.action_min_success_rate}({success_rate})")
    
    # Check action iteration count
    if hasattr(args, 'action_min_iters') and args.action_min_iters is not None:
        min_iters = min((r.iters for r in action), default=None)
        if min_iters is None or min_iters < args.action_min_iters:
            ok = False
            issues.append(f"action_iters<{args.action_min_iters}({min_iters})")

    # Additional custom checks - only fail if we have records to check against
    if service:  # Only perform service checks if we have service records
        total_calls = sum(r.calls for r in service)
        if total_calls != EXPECTED_SERVICE_TOTAL_CALLS:
            ok = False
            issues.append(f"service_total_calls_not_{EXPECTED_SERVICE_TOTAL_CALLS}({total_calls})")
    else:
        # If no service records found, we can't check against expected values
        if hasattr(args, 'service_min_tps') and args.service_min_tps is not None:
            ok = False
            issues.append("no_service_records_found")

    if action:  # Only perform action checks if we have action records
        total_success = sum(r.success for r in action)
        if total_success != EXPECTED_ACTION_TOTAL_SUCCESS:
            ok = False
            issues.append(f"action_total_success_not_{EXPECTED_ACTION_TOTAL_SUCCESS}({total_success})")
    else:
        # If no action records found, we can't check against expected values
        if hasattr(args, 'action_max_p99_ms') and args.action_max_p99_ms is not None:
            ok = False
            issues.append("no_action_records_found")

    return ok, issues


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Check topic/service/action performance test logs and output summary."
    )
    ap.add_argument(
        "log_dir",
        nargs="?",
        default="",
        help="Log directory. If omitted, use the latest under segar/integration_test/tool/logs/perf_*",
    )
    ap.add_argument("--topic-max-p99-us", type=float, default=None)
    ap.add_argument("--topic-min-throughput-mbps", type=float, default=None)
    ap.add_argument("--topic-min-iters", type=int, default=None, help="Minimum iterations for topic tests")
    ap.add_argument("--service-max-p99-us", type=float, default=None)
    ap.add_argument("--service-min-tps", type=float, default=None)
    ap.add_argument("--service-min-success-rate", type=float, default=None, help="Minimum success rate for service calls (0.0 to 1.0)")
    ap.add_argument("--action-max-p99-ms", type=float, default=None)
    ap.add_argument("--action-max-loss", type=float, default=None)
    ap.add_argument("--action-min-throughput-mbps", type=float, default=None)
    ap.add_argument("--action-min-success-rate", type=float, default=None, help="Minimum success rate for action calls (0.0 to 1.0)")
    ap.add_argument("--action-min-iters", type=int, default=None, help="Minimum iterations for action tests")
    args = ap.parse_args()

    # Resolve default log_dir if not provided
    if not args.log_dir:
        # .../segar/integration_test/tool
        tool_dir = os.path.abspath(os.path.join(os.path.dirname(__file__)))
        latest = find_latest_log_dir(tool_dir)
        if not latest:
            print("用法: check_perf_logs.py <log_dir>")
            return 1
        log_dir = latest
    else:
        log_dir = os.path.abspath(args.log_dir)

    if not os.path.isdir(log_dir):
        print(f"未找到日志目录: {log_dir}")
        return 1

    topic_a_log = os.path.join(log_dir, "topic_node_a.log")
    service_client_log = os.path.join(log_dir, "service_client.log")
    service_client_async_log = os.path.join(log_dir, "service_client_async.log")
    action_client_log = os.path.join(log_dir, "action_client_sync.log")

    log_files = sorted(glob.glob(os.path.join(log_dir, "*.log")))
    err_hits = scan_errors(log_files)
    err_ok = len(err_hits) == 0

    topic_records, topic_done = parse_topic(topic_a_log)
    service_records = parse_service(service_client_log)
    service_async_records = parse_service_async(service_client_async_log)
    action_records = parse_action(action_client_log)

    topic_ok = bool(topic_records) and topic_done
    service_ok = bool(service_records)
    service_async_ok = bool(service_async_records)
    action_ok = bool(action_records)

    thr_ok, thr_issues = check_thresholds(
        topic_records, service_records, action_records, args
    )

    all_ok = err_ok and topic_ok and service_ok and service_async_ok and action_ok and thr_ok

    summary: Dict[str, object] = {
        "log_dir": log_dir,
        "error_hits": [{"file": f, "line": ln, "text": t} for f, ln, t in err_hits],
        "topic": [r.__dict__ for r in topic_records],
        "topic_done": topic_done,
        "service": [r.__dict__ for r in service_records],
        "service_async": [r.__dict__ for r in service_async_records],
        "action": [r.__dict__ for r in action_records],
        "threshold_issues": thr_issues,
        "result": "pass" if all_ok else "fail",
    }
    with open(os.path.join(log_dir, "perf_summary.json"), "w", encoding="utf-8") as f:
        json.dump(summary, f, ensure_ascii=False, indent=2)

    # Build Markdown-style output
    out_lines: List[str] = []

    def emit(line: str = "") -> None:
        out_lines.append(line)

    emit("## Performance Test Summary")
    emit("")
    emit(f"- **log_dir**: `{log_dir}`")
    emit("- **perf_summary.json**: generated")
    emit("")

    emit(f"**Overall Result**: {'✅ PASS' if all_ok else '❌ FAIL'}")
    emit("")

    # Overview table
    emit("### Overview")
    emit("")
    emit("| Item             | Status | Key Metrics / Notes |")
    emit("|------------------|:------:|---------------------|")

    # Error overview
    err_note = f"{len(err_hits)} errors" if not err_ok else "0 errors"
    emit(f"| Error scan       | {status_icon(err_ok)} | {err_note} |")

    # Topic overview
    if topic_records:
        mx_p99_topic = max(r.p99_us for r in topic_records)
        mx_max_topic = max(r.max_us for r in topic_records)
        mn_thr_topic = min(r.thr_mbps for r in topic_records)
        topic_note = f"max_p99_us={mx_p99_topic:.2f}, max_us={mx_max_topic:.2f}, min_thr_mbps={mn_thr_topic:.2f}"
    else:
        topic_note = "no topic records"
    emit(f"| Topic check      | {status_icon(topic_ok)} | {topic_note} |")

    # Service overview (sync + async)
    def _service_overview_note(records: List[ServiceRecord]) -> str:
        if not records:
            return "no service records"
        mx_p99 = max(r.p99_us for r in records)
        mx_max = max(r.max_us for r in records)
        mn_tps = min(r.tps for r in records)
        total_calls = sum(r.calls for r in records)
        total_failed = sum(r.failed for r in records)
        return (
            f"max_p99_us={mx_p99:.2f}, max_us={mx_max:.2f}, min_tps={mn_tps:.2f}, "
            f"total_calls={total_calls}, total_failed={total_failed}"
        )

    for label, ok, recs in [
        ("Service-client", service_ok, service_records),
        ("Service-client-async", service_async_ok, service_async_records),
    ]:
        emit(f"| {label}   | {status_icon(ok)} | {_service_overview_note(recs)} |")

    # Action overview
    if action_records:
        mx_p99_act = max(r.p99_ms for r in action_records)
        mx_max_act = max(r.max_ms for r in action_records)
        mx_loss_act = max(r.loss for r in action_records)
        mn_thr_act = min(r.thr_mbps for r in action_records)
        total_sent_act = sum(r.sent for r in action_records)
        total_success_act = sum(r.success for r in action_records)
        action_note = (
            f"max_p99_ms={mx_p99_act:.3f}, max_ms={mx_max_act:.3f}, max_loss={mx_loss_act:.6f}, "
            f"min_thr_mbps={mn_thr_act:.2f}, total_sent={total_sent_act}, "
            f"total_success={total_success_act}"
        )
    else:
        action_note = "no action records"
    emit(f"| Action check     | {status_icon(action_ok)} | {action_note} |")

    # Threshold overview
    thr_note = "no issues" if thr_ok or not thr_issues else ",".join(thr_issues)
    emit(f"| Threshold checks | {status_icon(thr_ok)} | {thr_note} |")
    emit("")

    # Error scan detail
    emit("### Error scan")
    emit("")
    emit(f"- **result**: {'PASS' if err_ok else 'FAIL'}")
    emit(f"- **total_hits**: {len(err_hits)}")
    emit("")
    if not err_ok:
        emit("| # | File | Line | Message |")
        emit("|---|------|------|---------|")
        for idx, (fname, ln, text) in enumerate(err_hits[:50], start=1):
            safe_text = text.replace("|", "\\|")
            emit(f"| {idx} | `{fname}` | {ln} | {safe_text} |")
        if len(err_hits) > 50:
            emit("")
            emit(f"> Only first 50 errors shown (total {len(err_hits)}).")
    emit("")

    # Topic detail
    emit("### Topic performance")
    emit("")
    emit(f"- **result**: {'PASS' if topic_ok else 'FAIL'}")
    emit(f"- **records**: {len(topic_records)}")
    emit(f"- **done flag**: {topic_done}")
    if topic_records:
        mx_p99 = max(r.p99_us for r in topic_records)
        mx_max = max(r.max_us for r in topic_records)
        mn_thr = min(r.thr_mbps for r in topic_records)
        payload_sizes = sorted(list(set(r.payload for r in topic_records)))
        emit(f"- **max_p99_us**: {mx_p99:.2f}")
        emit(f"- **max_us**: {mx_max:.2f}")
        emit(f"- **min_thr_mbps**: {mn_thr:.2f}")
        emit(f"- **payload_sizes**: `{payload_sizes}`")
        emit("")
        emit("#### Topic latency by payload")
        emit("")
        emit("| payload(B) | iters | avg(us) | p10(us) | p30(us) | p50(us) | p90(us) | p99(us) | max(us) |")
        emit("|-----------:|------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|")
        for record in sorted(topic_records, key=lambda x: x.payload):
            emit(
                f"| {record.payload} | {record.iters} | {record.avg_us:.2f} | "
                f"{record.p10_us:.2f} | {record.p30_us:.2f} | {record.p50_us:.2f} | "
                f"{record.p90_us:.2f} | {record.p99_us:.2f} | {record.max_us:.2f} |"
            )
    else:
        emit("")
        emit("> No topic records found.")
    emit("")

    # Service detail (shared helper for sync and async)
    def _emit_service_detail_section(
        title: str, records: List[ServiceRecord], ok: bool
    ) -> None:
        emit(f"### {title}")
        emit("")
        emit(f"- **result**: {'PASS' if ok else 'FAIL'}")
        emit(f"- **records**: {len(records)}")
        if records:
            mx_p99 = max(r.p99_us for r in records)
            mx_max = max(r.max_us for r in records)
            mn_tps = min(r.tps for r in records)
            total_calls = sum(r.calls for r in records)
            total_failed = sum(r.failed for r in records)
            emit(f"- **max_p99_us**: {mx_p99:.2f}")
            emit(f"- **max_us**: {mx_max:.2f}")
            emit(f"- **min_tps**: {mn_tps:.2f}")
            emit(f"- **total_calls**: {total_calls}")
            emit(f"- **total_failed**: {total_failed}")
            emit("")
            emit("#### Service-async latency by payload")
            emit("")
            emit("| payload(B) | calls | failed | avg(us) | p10(us) | p50(us) | p90(us) | p99(us) | max(us) |")
            emit("|-----------:|------:|-------:|--------:|--------:|--------:|--------:|--------:|--------:|")
            for record in sorted(
                records, key=lambda x: x.payload if x.payload is not None else 0
            ):
                if record.payload is not None:
                    emit(
                        f"| {record.payload} | {record.calls} | {record.failed} | {int(record.avg_us)} | "
                        f"{int(record.p10_us)} | {int(record.p50_us)} | {int(record.p90_us)} | "
                        f"{int(record.p99_us)} | {int(record.max_us)} |"
                    )
        else:
            emit("")
            emit("> No service records found.")
        emit("")

    _emit_service_detail_section("Service-client performance", service_records, service_ok)
    _emit_service_detail_section("Service-client-async performance", service_async_records, service_async_ok)

    # Action detail
    emit("### Action performance")
    emit("")
    emit(f"- **result**: {'PASS' if action_ok else 'FAIL'}")
    emit(f"- **records**: {len(action_records)}")
    if action_records:
        mx_p99 = max(r.p99_ms for r in action_records)
        mx_max = max(r.max_ms for r in action_records)
        mx_loss = max(r.loss for r in action_records)
        mn_thr = min(r.thr_mbps for r in action_records)
        total_sent = sum(r.sent for r in action_records)
        total_success = sum(r.success for r in action_records)
        payload_sizes = sorted(list(set(r.payload for r in action_records)))
        emit(f"- **max_p99_ms**: {mx_p99:.3f}")
        emit(f"- **max_ms**: {mx_max:.3f}")
        emit(f"- **max_loss**: {mx_loss:.6f}")
        emit(f"- **min_thr_mbps**: {mn_thr:.2f}")
        emit(f"- **total_sent**: {total_sent}")
        emit(f"- **total_success**: {total_success}")
        emit(f"- **payload_sizes**: `{payload_sizes}`")
        emit("")
        emit("#### Action latency by payload")
        emit("")
        emit("| payload(B) | sent | success | avg(us) | p10(us) | p50(us) | p90(us) | p99(us) | max(us) |")
        emit("|-----------:|-----:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|")
        for record in sorted(action_records, key=lambda x: (x.payload, x.sent)):
            emit(
                f"| {record.payload} | {record.sent} | {record.success} | {int(record.avg_ms * 1000)} | "
                f"{int(record.p10_ms * 1000)} | {int(record.p50_ms * 1000)} | {int(record.p90_ms * 1000)} | "
                f"{int(record.p99_ms * 1000)} | {int(record.max_ms * 1000)} |"
            )
    else:
        emit("")
        emit("> No action records found.")
    emit("")

    # Threshold detail
    emit("### Threshold checks")
    emit("")
    emit(f"- **result**: {'PASS' if thr_ok else 'FAIL'}")
    if thr_issues:
        emit("")
        emit("```text")
        emit(f"issues={','.join(thr_issues)}")
        emit("```")
    emit("")

    # Final verdict
    emit("")
    emit("---")
    emit("")
    emit("**Final Verdict**: " + ("✅ PERF TEST ALL PASS" if all_ok else "❌ PERF TEST FAIL"))
    emit("")

    # Print to stdout
    for line in out_lines:
        print(line)

    # Write markdown file
    md_path = os.path.join(log_dir, "perf_summary.md")
    try:
        with open(md_path, "w", encoding="utf-8") as f_md:
            f_md.write("\n".join(out_lines) + "\n")
        print(f"\nMarkdown summary written to: {md_path}")
    except OSError as e:
        print(f"\nWarning: Failed to write markdown file: {e}", file=sys.stderr)

    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())