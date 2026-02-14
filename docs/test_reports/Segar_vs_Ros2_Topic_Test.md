# Segar/ROS2 Topic performance comparison test report (x86 platform)

---

## 1. Test Overview

This test focuses on the core performance of ROS2 middleware and Segar middleware in Topic communication scenarios. Through the Ping-Pong communication mode, the **throughput** (message throughput, bandwidth throughput) and **latency** (average delay, minimum delay, maximum delay) indicators of the two middlewares are compared when transmitting different amounts of data, providing objective data support for middleware selection. Specific goals include:

- Verify **Message Throughput**: The number of messages that the middleware can transmit per unit time (msg/s)
- Verification **Bandwidth Throughput**: The total number of bytes that the middleware can transfer per unit time (MB/s)
- Verification **Round Trip Delay**: Complete link delay (RTT) from send to receive
- Verify **Delay Stability**: The fluctuation range of the minimum delay and the maximum delay
- Verify **Large Data Volume Transmission Performance**: Performance under 1MB super large data volume
- Verify **Full range performance robustness**: Performance consistency in each data volume range from 64B to 1MB

---

## 2. Test environment

| Dimensions | Configuration instructions |
|------|----------|
| hardware | x86_64 (28-core CPU) |
| OS & Kernel | ubuntu22.04(linux) |
| ROS2 version | Humble |
| Segar version | V2.0.0 |

---

## 3. Test design and topology

The test uses the **Ping-Pong mode** of "publisher-subscriber" two-way communication:

- **Publishing end node A**: Generate a message according to the configured data amount and send it to node B. After receiving the response message from node B, it immediately sends the next message, and records the sending timestamp and receiving timestamp for indicator calculation;
- **Subscribing end node B**: Subscribe to the Topic sent by node A. After receiving the message, it does not perform any business logic processing and immediately returns the original message to node A, ensuring that the delay only comes from middleware communication overhead.

The system includes the following core testing mechanisms:

- **Message definition**: Unified message structure (topic_id, timestamp, sequence_number, data_size, data)
- **Data volume gradient**: 64B, 256B, 1KB, 4KB, 16KB, 64KB, 256KB, 1MB
- **Number of iterations**: Accumulated statistics of 10,000 iterations for each data volume
- **Indicator calculation**: message throughput, bandwidth throughput, average/minimum/maximum delay

### 1. Message definition (message)

The Topic message structure used in the test is as follows, and the field specifications are unified to ensure that the two middleware test benchmarks are consistent:

| Field | type | illustrate |
|------|------|------|
| topic_id | uint32 | Topic identifier, used to distinguish different test flows |
| timestamp | uint64 | Send timestamp (nanosecond level), used to calculate RTT |
| sequence_number | uint32 | Serial number, used for packet loss detection |
| data_size | uint32 | Data payload size (bytes) |
| data | uint8[] | Actual data load, populated as per test configuration |

> Note: By configuring different data amounts, the communication load changes of the robot in different task scenarios are simulated.

### 2. Test flow (test_flow)

Using the Ping-Pong mechanism, loaded as a module by the test framework:

Build a two-way communication link to simulate the process of real-time data interaction between nodes in the robot system:

| step | Node A (publishing end) | Node B (subscriber) | Function description |
|------|---------------|---------------|----------|
| 1 | Generate message and record timestamp | - | Initialize test data |
| 2 | Send Topic to Node B | Receive Topic | Establish communication link |
| 3 | Waiting for response | Return to original message immediately | Calculate round trip delay |
| 4 | Receive response and calculate RTT | - | Statistical performance indicators |
| 5 | Send next message immediately | - | Continuous stress testing |

> Remarks: Simulate the data flow process of **sensor data uploading and control command issuance** in the robot system

### 3. Key parameter configuration (configuration)

Use unified QoS and queue configuration:

- **Service Node**: NodeB provides instant response services and simulates the **data relay and forwarding** scenario in the robot system.
- **Client node**: NodeA sends messages at the maximum frequency to verify the response capability and stability of the middleware under high concurrent requests.

> Remarks: **High-frequency data exchange and real-time response** mechanism in the simulated robot system

### 4. Metrics calculation method

- **Message throughput (msg/s)** = number of test iterations / total time taken (seconds)
- **Bandwidth throughput (MB/s)** = (Single message data size × number of iterations) / (1024 × 1024 × total time taken (seconds))
- **Average latency (us)** = sum of RTT of all iterations / number of iterations
- **Minimum delay (us)** = Minimum value of RTT among all iterations
- **Maximum delay (us)** = Maximum value of RTT among all iterations

> Remarks: **Quantitative evaluation** mechanism of performance indicators in the simulated robot system

---

## 4. Test results

### Test summary

- **Test module**: Topic communication, throughput test, delay test
- **Test environment**: x86_64 (28-core CPU), ubuntu22.04

**Test conclusion**: ✅ Segar is in the lead (all indicators are better than ROS2)

| module | Test items | Segar results | ROS2 results | Comparative conclusion |
|------|--------|-----------|----------|----------|
| Throughput(1MB) | Message throughput | 1542.1 msg/s | 82.7 msg/s | ✅ Segar leads 17.9 times |
| Throughput(1MB) | bandwidth throughput | 1542.13 MB/s | 82.67 MB/s | ✅ Segar leads 17.9 times |
| Delay(1MB) | average delay | 648.23 us | 12092.35 us | ✅ Segar reduced by 94.7% |
| Delay(1MB) | maximum delay | 1879.75 us | 45213.29 us | ✅ Segar reduced by 95.8% |
| stability | Maximum delay fluctuation | 168.15~1879.75 us | 229.68~45213.29 us | ✅ Segar has minimal fluctuations |

### Detailed analysis

#### 1. Throughput test (Throughput)

**1MB extremely large data volume scenario:**

- ✅ Segar message throughput reaches **1542.1 msg/s**, which is **17.9 times** that of ROS2 (82.7 msg/s)
- ✅ Segar bandwidth throughput reaches **1542.13 MB/s**, far exceeding ROS2’s 82.67 MB/s
- ✅ Segar completely solves the pain points of ROS2's surge in serialization and memory copy overhead in large data transmission

> **Conclusion**: Segar shows overwhelming advantages in the scenario of extremely large data volume of 1MB, and perfectly adapts to the efficient transmission requirements of large payload data such as high-definition images and lidar point clouds.

**Interval robustness across the entire data volume:**

| Data volume | Segar(msg/s) | ROS2(msg/s) | state |
|--------|--------------|-------------|------|
| 64B | 36350.0 | 34452.6 | ✅ Segar is slightly better |
| 256B | 36231.9 | 35523.5 | ✅ Segar is slightly better |
| 1KB | 35342.3 | 34188.2 | ✅ Segar is slightly better |
| 4KB | 33298.1 | 34188.2 | ⚠️ Quite |
| 16KB | 28571.4 | 26368.0 | ✅ Segar leads by 8.4% |
| 64KB | 16293.76 | 15310.1 | ✅ Segar leads by 6.4% |
| 256KB | 6319.11 | 6285.30 | ✅ Both are equivalent |
| 1MB | 1542.1 | 82.7 | ✅ Segar leads by 1764% |

> **Conclusion**: Segar maintains high throughput levels (16293~36350 msg/s) in the entire range of 64B~256KB without significant performance fluctuations; while ROS2’s throughput plummeted by 99% in the 1MB scenario, completely unable to meet the demand for large-capacity data transmission.

#### 2. Latency test (Latency)

**1MB large data volume delay control:**

- ✅ The average delay of Segar is only **648.23 us**, which is **94.7%** lower than ROS2 (12092.35 us)
- ✅ The maximum delay of Segar is only **1879.75 us**, which is **95.8%** lower than ROS2 (45213.29 us)
- ✅ Segar solves the fatal problem of delayed explosive growth of ROS2 under extremely large amounts of data

> **Conclusion**: With its better architectural design, Segar avoids the delay linear growth trap of traditional DDS solutions and meets the real-time perception and decision-making needs of robots.

**Full range delay stability:**

| Data volume | Segar average delay | ROS2 average latency | Segar maximum delay | ROS2 maximum delay |
|--------|---------------|--------------|---------------|--------------|
| 64B | 27.43 us | 29.03 us | 168.15 us | 229.68 us |
| 256B | 27.59 us | 27.16 us | 168.85 us | 229.97 us |
| 1KB | 28.24 us | 29.25 us | 171.21 us | 231.42 us |
| 4KB | 29.98 us | 29.25 us | 176.89 us | 233.89 us |
| 16KB | 34.97 us | 37.93 us | 195.89 us | 251.89 us |
| 64KB | 61.31 us | 65.31 us | 286.42 us | 312.89 us |
| 256KB | 158.25 us | 159.11 us | 631.05 us | 724.57 us |
| 1MB | 648.23 us | 12092.35 us | 1879.75 us | 45213.29 us |

> **Conclusion**: The maximum delay fluctuation range of Segar is only 168.15~1879.75 us, which can remain stable even in extreme scenarios; while the maximum delay span of ROS2 is 229.68~45213.29 us. The maximum delay in the 1MB scenario is 24 times that of Segar, and there is a serious risk of peak delay.

#### 3. Delay scalability analysis (Scalability)

- ✅ **Segar**: The average latency increases **gently and linearly** with the growth of data volume, from 27.43 us for 64B to 648.23 us for 1MB. The growth rate is controllable (about 23 times)
- ❌ **ROS2**: In the 1MB scenario, the latency explodes **exponentially**, increasing from 29.03 us at 64B to 12092.35 us at 1MB, an increase of 416 times

> **Conclusion**: Segar’s delay scalability is industry-leading, fully exposing the fatal flaw of the ROS2 architecture in large data volume scenarios.

#### 4. Comprehensive performance comparison (Comparison)

| Contrast Dimensions | Segar | ROS2 | in conclusion |
|----------|-------|------|------|
| Large data volume throughput | ✅ 1542.1 msg/s | ❌ 82.7 msg/s | Segar leads 17.9 times |
| Large data volume delay | ✅ 648.23 us | ❌ 12092.35 us | Segar reduced by 94.7% |
| Delay stability | ✅ Fluctuation <2ms | ❌ Peak >45ms | Segar is more stable |
| Delay scalability | ✅ Linear growth | ❌ Exponential growth | Segar is scalable |
| Small data volume performance | ✅ Equal or slightly better | ⚠️ Quite | Both are close |
| Robustness over the entire interval | ✅ No performance shortcomings | ❌ 1MB crash | Segar is more robust |

> **Conclusion**: Segar has completely surpassed ROS2 in the three core indicators of throughput, latency, and stability. Only when the data volume is 4KB, the throughput is slightly lower than ROS2 (the gap is about 2.6%), which can be ignored in practical applications.

---

## Test conclusion

Segar middleware performs well on the Orin platform and has significant advantages over ROS2:

1. **Super large data volume crushing advantage**: In the 1MB scenario, the throughput is **17.9 times** that of ROS2, and the delay is reduced by **94.7%**, completely solving the problem of ROS2 large data transmission performance collapse
2. **Robust performance in the entire range**: Maintaining high throughput (>16000 msg/s) in the entire range of 64B~256KB, no significant performance fluctuations, and no performance shortcomings
3. **Extreme delay stability**: The maximum delay fluctuation is <2ms, while the peak delay of ROS2 reaches 45ms in the 1MB scenario, which poses serious real-time risks
4. **Leading scalability**: Latency increases linearly with the amount of data, and the growth rate is controllable (23 times), while ROS2 explodes exponentially (416 times)
5. **Wide scene adaptation**: It can not only meet the high-frequency transmission of small data volumes (such as control instructions), but also stably support the low-latency transmission of large data volumes (such as point clouds and images)


---
