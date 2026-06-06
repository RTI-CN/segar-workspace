/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "integration_test/srv/PerfData.hpp"
#include "perf_common.hpp"

#include "segar/segar.h"

using PerfData = integration_test::srv::PerfData;

namespace {

constexpr uint64_t kTotalCalls = 10000;
const std::vector<size_t> kPayloadSizes = {64,    256,   1024,   4096,
                                           16384, 65536, 262144, 1048576};

struct Args {
  bool multi = false;
  int threads = 1;
  size_t size = 0;  // 0 => run all
  std::string service_name = "perf_service";
  int timeout_ms = 2000;
};

Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (s.rfind("--size=", 0) == 0)
      a.size = static_cast<size_t>(std::stoul(s.substr(7)));
    else if (s.rfind("--service=", 0) == 0)
      a.service_name = s.substr(10);
    else if (s.rfind("--timeout_ms=", 0) == 0)
      a.timeout_ms = std::max(1, std::atoi(s.substr(13).c_str()));
  }
  return a;
}

std::shared_ptr<PerfData::Request> make_request(uint32_t id, uint32_t seq,
                                                size_t size) {
  auto req = std::make_shared<PerfData::Request>();

  req->id(id);
  req->timestamp(perf::now_ns_steady());
  req->sequence_number(seq);
  req->data_size(static_cast<uint32_t>(size));

  std::vector<uint8_t> data(size);
  for (size_t i = 0; i < size; ++i) data[i] = static_cast<uint8_t>(i & 0xFF);
  req->data(std::move(data));
  return req;
}

void run_single_concurrency(const Args& args, size_t payload_size) {
  auto node = rti::segar::CreateNode("segar_perf_client_single");
  auto client = node->CreateClient<PerfData>(args.service_name);
  client->WaitForService();

  rti::segar::service::RequestOptions opt;
  opt.timeout = std::chrono::milliseconds(args.timeout_ms);

  std::vector<size_t> sizes =
      (args.size > 0) ? std::vector<size_t>{args.size} : kPayloadSizes;

  for (size_t sz : sizes) {
    std::vector<double> lat_us;
    lat_us.reserve(kTotalCalls);

    std::cout << "\n==============================\n";
    std::cout << "Running payload size: " << sz << " bytes\n";
    std::cout << "==============================\n";

    const auto t0 = std::chrono::steady_clock::now();

    for (uint32_t i = 0; i < kTotalCalls; ++i) {
      auto req = make_request(/*id=*/1, /*seq=*/i, sz);

      const uint64_t send_ns = perf::now_ns_steady();
      auto res = client->SyncSendRequest(req, opt);
      const uint64_t recv_ns = perf::now_ns_steady();

      if (res != nullptr) {
        lat_us.push_back(static_cast<double>(recv_ns - send_ns) / 1000.0);
      }
    }

    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
            .count();

    auto st =
        perf::compute_stats(lat_us, kTotalCalls, seconds, lat_us.size(), 0);
    perf::print_stats(
        "Segar Single-Concurrency | payload=" + std::to_string(sz) + "B", st);
  }
  rti::segar::OnShutdown(0);
}

}  // namespace

int main(int argc, char* argv[]) {
  rti::segar::Init(argv[0]);
  auto args = parse_args(argc, argv);

  run_single_concurrency(args, 0);

  rti::segar::WaitForShutdown();
  return 0;
}
