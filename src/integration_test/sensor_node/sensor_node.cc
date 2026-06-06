/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "integration_test/msg/IntegrationTestData.hpp"
#include "nlohmann/json.hpp"

#include "segar/segar.h"
#include "segar/time/rate.h"
#include "segar/time/time.h"

using json = nlohmann::json;
using rti::segar::Node;
using rti::segar::Rate;
using rti::segar::Time;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

struct SensorConfig {
  int id;
  std::string name;
  std::string topic;
  int frequency;
  int data_size_kb;
};

class SensorNode {
 public:
  SensorNode(const SensorConfig& config) : config_(config) {}

  void Initialize() {
    // 创建节点
    node_ = rti::segar::CreateNode(config_.name);

    // 为该主题创建写入器
    writer_ = node_->CreateWriter<IntegrationTestData>(config_.topic);

    std::cout << "初始化传感器 " << config_.name << " 发布到主题 "
              << config_.topic << " 频率 " << config_.frequency << " Hz"
              << " 数据大小 " << config_.data_size_kb << " KB" << std::endl;
  }

  void Run() {
    Rate rate(double(config_.frequency));
    uint64_t seq = 0;

    while (rti::segar::OK()) {
      auto msg = std::make_shared<IntegrationTestData>();
      msg->sensor_id(config_.id);

      msg->sequence_number(seq++);
      msg->data_size(config_.data_size_kb * 1024);
      msg->data().resize(config_.data_size_kb * 1024);

      // 用虚拟值填充数据
      for (int i = 0; i < config_.data_size_kb * 1024 && i < 1024 * 1024; ++i) {
        msg->data()[i] = static_cast<unsigned char>(i % 256);
      }
      msg->timestamp(rti::segar::Time::Now().ToNanosecond());

      writer_->Write(msg);

      AINFO << "ITEST_SEND sensor_id=" << config_.id
            << " topic=" << config_.topic << " seq=" << msg->sequence_number()
            << " msg_ts=" << msg->timestamp();

      rate.Sleep();
    }

    // 目前仅模拟操作因为我们没有实际的IDL
    // Rate rate(double(config_.frequency));
    // uint64_t seq = 0;

    // while (rti::segar::OK()) {
    //   std::cout << config_.name << " 将发送一条消息! 编号 " << seq
    //             << " 大小: " << config_.data_size_kb << "KB"
    //             << " 主题: " << config_.topic << std::endl;
    //   seq++;
    //   rate.Sleep();
    // }
  }

 private:
  SensorConfig config_;
  std::shared_ptr<Node> node_;
  std::shared_ptr<Writer<IntegrationTestData>> writer_;
};

SensorConfig LoadSensorConfig(const std::string& config_file, int sensor_id) {
  std::ifstream file(config_file);
  if (!file.is_open()) {
    throw std::runtime_error("无法打开配置文件: " + config_file);
  }

  json j;
  file >> j;

  for (const auto& sensor_json : j["sensors"]) {
    if (sensor_json["id"] == sensor_id) {
      SensorConfig config;
      config.id = sensor_json["id"];
      config.name = sensor_json["name"];
      config.topic = sensor_json["topic"];
      config.frequency = sensor_json["frequency"];
      config.data_size_kb = sensor_json["data_size_kb"];
      return config;
    }
  }

  throw std::runtime_error("未找到ID为 " + std::to_string(sensor_id) +
                           " 的传感器配置");
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "使用方法: " << argv[0] << " <配置文件> <传感器ID>"
              << std::endl;
    std::cerr << "例如: " << argv[0] << " sensor_config.json 1" << std::endl;
    return -1;
  }

  try {
    // 初始化Segar框架
    rti::segar::Init(argv[0]);

    // 加载特定传感器的配置
    int sensor_id = std::stoi(argv[2]);
    auto sensor_config = LoadSensorConfig(argv[1], sensor_id);

    // 创建并初始化传感器节点
    auto sensor_node = std::make_shared<SensorNode>(sensor_config);
    sensor_node->Initialize();

    // 运行传感器节点
    sensor_node->Run();

  } catch (const std::exception& e) {
    std::cerr << "错误: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
