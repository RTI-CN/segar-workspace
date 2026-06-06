/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace perf {

// steady clock ns
inline uint64_t now_ns_steady() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

struct Stats {
  uint64_t total_calls = 0;
  uint64_t success_calls = 0;
  uint64_t failed_calls = 0;
  double seconds = 0.0;
  double tps = 0.0;
  double min_us = 0.0;
  double avg_us = 0.0;
  double p10_us = 0.0;
  double p30_us = 0.0;
  double p50_us = 0.0;
  double p90_us = 0.0;
  double p99_us = 0.0;
  double p999_us = 0.0;
  double max_us = 0.0;
  double stddev_us = 0.0;  // standard deviation
};

inline double percentile(const std::vector<double>& v, double p) {
  if (v.empty()) return 0.0;
  std::vector<double> sorted = v;
  std::sort(sorted.begin(), sorted.end());
  double idx = (p / 100.0) * (sorted.size() - 1);
  size_t i = static_cast<size_t>(idx);
  double frac = idx - i;
  if (i + 1 >= sorted.size()) return sorted.back();
  return sorted[i] * (1.0 - frac) + sorted[i + 1] * frac;
}

inline Stats compute_stats(const std::vector<double>& lat_us,
                           uint64_t total_calls, double seconds,
                           uint64_t success_calls = 0,
                           uint64_t failed_calls = 0) {
  Stats s;
  s.total_calls = total_calls;
  s.success_calls = success_calls > 0 ? success_calls : lat_us.size();
  s.failed_calls = failed_calls;
  s.seconds = seconds;
  s.tps = seconds > 0 ? static_cast<double>(s.success_calls) / seconds : 0.0;

  if (!lat_us.empty()) {
    s.min_us = *std::min_element(lat_us.begin(), lat_us.end());
    s.max_us = *std::max_element(lat_us.begin(), lat_us.end());
    s.avg_us =
        std::accumulate(lat_us.begin(), lat_us.end(), 0.0) / lat_us.size();
    s.p10_us = percentile(lat_us, 10);
    s.p30_us = percentile(lat_us, 30);
    s.p50_us = percentile(lat_us, 50);
    s.p90_us = percentile(lat_us, 90);
    s.p99_us = percentile(lat_us, 99);
    s.p999_us = percentile(lat_us, 99.9);

    // Calculate standard deviation
    double variance = 0.0;
    for (double lat : lat_us) {
      double diff = lat - s.avg_us;
      variance += diff * diff;
    }
    s.stddev_us = lat_us.size() > 1 ? std::sqrt(variance / lat_us.size()) : 0.0;
  }
  return s;
}

inline void print_stats(const std::string& title, const Stats& s) {
  std::cout << "\n==== " << title << " ====\n";
  std::cout << "calls=" << s.total_calls << "  success=" << s.success_calls
            << "  failed=" << s.failed_calls << "  seconds=" << std::fixed
            << std::setprecision(3) << s.seconds << "  TPS=" << std::fixed
            << std::setprecision(1) << s.tps << "\n";
  std::cout << "latency(us): min=" << std::fixed << std::setprecision(1)
            << s.min_us << "  avg=" << s.avg_us << "  stddev=" << s.stddev_us
            << "  p10=" << s.p10_us << "  p30=" << s.p30_us
            << "  p50=" << s.p50_us << "  p90=" << s.p90_us
            << "  p99=" << s.p99_us << "  p99.9=" << s.p999_us
            << "  max=" << s.max_us << "\n";
}

}  // namespace perf
