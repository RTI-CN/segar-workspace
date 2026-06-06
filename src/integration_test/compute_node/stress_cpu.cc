#include "stress_cpu.h"

#include <chrono>
#include <cmath>
#include <iostream>
void cpu_burner_ms(int time_ms) {
  using clock = std::chrono::high_resolution_clock;
  auto start = clock::now();

  // volatile 防止编译器优化掉计算
  volatile double accumulator = 1.0;
  const auto target_us = std::chrono::microseconds(time_ms * 1000);

  while (clock::now() - start < target_us) {
    // 每轮进行大量浮点运算，确保CPU满负荷
    for (int i = 0; i < 500; ++i) {
      accumulator += std::sin(i) * std::cos(i) * std::sqrt(accumulator + 1.0);
      accumulator = std::fmod(accumulator, 1e10);  // 防止数值溢出
    }
  }

  // 防止 accumulator 被优化警告
  (void)accumulator;
}
