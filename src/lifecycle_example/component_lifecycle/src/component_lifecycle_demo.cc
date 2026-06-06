/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

#include "segar/component/timer_component.h"
#include "segar/proto/component_conf.pb.h"
#include "segar/segar.h"

namespace {

class LifecycleTimerComponentExample : public rti::segar::TimerComponent {
 public:
  bool Init() override { return true; }

  int ProcCount() const { return proc_count_.load(); }

 private:
  bool Proc() override {
    const int count = proc_count_.fetch_add(1) + 1;
    AINFO << "timer component proc_count=" << count;
    return true;
  }

  std::atomic<int> proc_count_ = {0};
};

}  // namespace

int main(int argc, char* argv[]) {
  if (!rti::segar::Init(argv[0])) {
    return EXIT_FAILURE;
  }

  auto component = std::make_shared<LifecycleTimerComponentExample>();
  rti::segar::proto::TimerComponentConfig config;
  config.set_inner_node_name("component_lifecycle_demo");
  config.set_interval(100);

  if (!component->Initialize(config)) {
    return EXIT_FAILURE;
  }

  AINFO << "component initial state: " << static_cast<int>(component->GetState());
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  const int active_count_before = component->ProcCount();
  if (active_count_before <= 0) {
    AERROR << "component Proc() was not triggered while active";
    return EXIT_FAILURE;
  }

  if (!component->Deactivate()) {
    AERROR << "component deactivate failed";
    return EXIT_FAILURE;
  }
  AINFO << "component state after deactivate: "
        << static_cast<int>(component->GetState());
  const int inactive_begin_count = component->ProcCount();
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  const int inactive_end_count = component->ProcCount();
  if (inactive_begin_count != inactive_end_count) {
    AERROR << "component Proc() should not run while inactive";
    return EXIT_FAILURE;
  }

  if (!component->Activate()) {
    AERROR << "component activate failed";
    return EXIT_FAILURE;
  }
  AINFO << "component state after activate: "
        << static_cast<int>(component->GetState());
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  const int active_count_after = component->ProcCount();
  if (active_count_after <= inactive_end_count) {
    AERROR << "component Proc() did not resume after activate";
    return EXIT_FAILURE;
  }

  AINFO << "expected behavior: proc_count increases while active, stays unchanged while inactive, then resumes after activate. final proc_count="
        << active_count_after;
  return EXIT_SUCCESS;
}
