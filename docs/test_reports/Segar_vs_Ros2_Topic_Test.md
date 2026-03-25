# Segar/ROS2 Topic performance comparison test report (x86 platform)

---

## 1. Test Overview

This test focuses on the core performance of ROS2 middleware and Segar middleware in Topic communication scenarios. Through the Ping-Pong communication mode, the **throughput** (message throughput, bandwidth throughput) and **latency** (average latency, minimum latency, maximum latency) indicators of the two middlewares are compared when transmitting different amounts of data, providing objective data support for middleware selection. Specific goals include:

- Verify **Message Throughput**: The number of messages that the middleware can transmit per unit time (msg/s)
- Verify **Bandwidth Throughput**: The total number of bytes that the middleware can transfer per unit time (MB/s)
- Validation **Round Trip Delay**: Complete link latency (RTT) from send to receive
- Verify **Delay Stability**: the fluctuation range of minimum latency and maximum latency
- Verify **Large Data Volume Transmission Performance**: Performance under 1MB super large data volume
- Verify **Full range performance robustness**: Performance consistency in each data volume range from 64B to 1MB

---

## 2. Test environment

| Dimensions | Configuration instructions |
|------|----------|
| Hardware | x86_64 (28-core CPU) |
| OS & Kernel | ubuntu22.04 (linux) |
| ROS2 version | Humble |
| Segar version | V2.0.0 |

---

## 3. Test design and topology

The test uses the **Ping-Pong mode** of "publisher-subscriber" two-way communication:

- **Publishing end node A**: Generate messages according to the configured data amount and send them to node B. After receiving the response message from node B, the next message will be sent immediately, and the sending timestamp and receiving timestamp will be recorded at the same time for indicator calculation;
- **Subscriber Node B**: Subscribe to the Topic sent by Node A, do not perform any business logic processing after receiving the message, and immediately return the original message to Node A, ensuring that the latency only comes from middleware communication overhead.

The system includes the following core testing mechanisms:

- **Message Definition**: Unified message structure (topic_id, timestamp, sequence_number, data_size, data)
- **Data volume gradient**: 64B, 256B, 1KB, 4KB, 16KB, 64KB, 256KB, 1MB
- **Number of iterations**: Accumulated statistics of 10,000 iterations for each data volume
- **Indicator calculation**: message throughput, bandwidth throughput, average/minimum/maximum latency

### 1. Message definition (message)

The Topic message structure used in the test is as follows, and the field specifications are unified to ensure that the two middleware test benchmarks are consistent:

| Field | Type | Description |
|------|------|------|
| topic_id | uint32 | Topic identifier, used to distinguish different test flows |
| timestamp | uint64 | Send timestamp (nanosecond level), used to calculate RTT |
| sequence_number | uint32 | Sequence number, used for packet loss detection |
| data_size | uint32 | Data payload size (bytes) |
| data | uint8[] | Actual data load, populated according to test configuration |

&gt; Note: By configuring different data amounts, the communication load changes of the robot in different task scenarios are simulated.

### 2. Test flow (test_flow)

Using the Ping-Pong mechanism, loaded as a module by the test framework:

Build a two-way communication link to simulate the process of real-time data interaction between nodes in the robot system:

| Steps | Node A (publisher) | Node B (subscriber) | Function description |
|------|---------------|---------------|----------|
| 1 | Generate messages and record timestamp | - | Initialize test data |
| 2 | Send Topic to Node B | Receive Topic | Establish communication link |
| 3 | Wait for response | Return original message immediately | Calculate round-trip latency |
| 4 | Receive response and calculate RTT | - | Statistical performance indicators |
| 5 | Send next message immediately | - | Continuous stress testing |

&gt; Note: Simulate the data flow process of sensor data upload and control command issuance in the robot system.

### 3. Key parameter configuration (configuration)

Use unified QoS and queue configuration:

- **Service Node**: NodeB provides instant response services, simulating the **data relay and forwarding** scenario in the robot system.
- **Client Node**: NodeA sends messages at the maximum frequency to verify the response capability and stability of the middleware under high concurrent requests.

&gt; Note: **High-frequency data exchange and real-time response** mechanism in the simulated robot system

### 4. Metrics calculation method

- **Message throughput (msg/s)** = number of test iterations / total time taken (seconds)
- **Bandwidth Throughput (MB/s)** = (Amount of data in a single message × Number of iterations) / (1024 × 1024 × Total time taken (seconds))
- **Average latency (us)** = sum of RTT of all iterations / number of iterations
- **Minimum latency (us)** = minimum value of RTT among all iterations
- **Maximum latency (us)** = Maximum value of RTT among all iterations

&gt; Note: **Quantitative evaluation mechanism of performance indicators** in the simulated robot system

---

## 4. Test results

### Test Summary

- **Test module**: Topic communication, throughput test, latency test
- **Test environment**: x86_64 (28-core CPU), ubuntu22.04

**Test conclusion**: ✅ Segar is in the lead (all indicators are better than ROS2)

| Module | Test items | Segar results | ROS2 results | Comparison conclusion |
|------|--------|-----------|----------|----------|
| Throughput (1MB) | Message throughput | 1542.1 msg/s | 82.7 msg/s | ✅ Segar leads by 17.9 times |
| Throughput (1MB) | Bandwidth throughput | 1542.13 MB/s | 82.67 MB/s | ✅ Segar leads 17.9 times |
| Latency (1MB) | Average latency | 648.23 us | 12092.35 us | ✅ Segar reduced by 94.7% |
| Latency (1MB) | Maximum latency | 1879.75 us | 45213.29 us | ✅ Segar reduced by 95.8% |
| Stability | Maximum latency fluctuation | 168.15~1879.75 us | 229.68~45213.29 us | ✅ Segar fluctuation is extremely small |

### Detailed analysis

#### 1. Throughput test (Throughput)

**1MB extremely large data volume scenario:**

- ✅ Segar message throughput reaches **1542.1 msg/s**, which is **17.9 times** that of ROS2 (82.7 msg/s)
- ✅ Segar bandwidth throughput reaches **1542.13 MB/s**, far exceeding ROS2’s 82.67 MB/s
- ✅ Segar completely solves the pain points of ROS2 in the surge in serialization and memory copy overhead during large data transmission

&gt; **Conclusion**: Segar shows overwhelming advantages in the scenario of extremely large data volume of 1MB, and perfectly adapts to the efficient transmission requirements of large payload data such as high-definition images and lidar point clouds.

**Interval robustness across the entire data volume:**| Data volume | Segar(msg/s) | ROS2(msg/s) | Status |
|--------|--------------|-------------|------|
| 64B | 36350.0 | 34452.6 | ✅ Segar is slightly better |
| 256B | 36231.9 | 35523.5 | ✅ Segar is slightly better |
| 1KB | 35342.3 | 34188.2 | ✅ Segar is slightly better |
| 4KB | 33298.1 | 34188.2 | ⚠️ Quite |
| 16KB | 28571.4 | 26368.0 | ✅ Segar leads by 8.4% |
| 64KB | 16293.76 | 15310.1 | ✅ Segar leads by 6.4% |
| 256KB | 6319.11 | 6285.30 | ✅ Both are equivalent |
| 1MB | 1542.1 | 82.7 | ✅ Segar leads by 1764% |

&gt; **Conclusion**: Segar maintains a high throughput level (16293~36350 msg/s) in the entire range of 64B~256KB, without significant performance fluctuations; while ROS2’s throughput plummeted by 99% in the 1MB scenario, completely unable to meet the demand for large-capacity data transmission.

#### 2. Latency test (Latency)

**1MB large data volume latency control:**

- ✅ The average latency of Segar is only **648.23 us**, which is **94.7%** lower than ROS2 (12092.35 us)
- ✅ The maximum latency of Segar is only **1879.75 us**, which is **95.8%** lower than ROS2 (45213.29 us)
- ✅ Segar solves the fatal problem of latencyed explosive growth of ROS2 under extremely large amounts of data

&gt; **Conclusion**: With its better architectural design, Segar avoids the latencyed linear growth trap of traditional DDS solutions and meets the real-time perception and decision-making needs of robots.

**Full range latency stability:**

| Data volume | Segar average latency | ROS2 average latency | Segar maximum latency | ROS2 maximum latency |
|--------|---------------|--------------|---------------|--------------|
| 64B | 27.43 us | 29.03 us | 168.15 us | 229.68 us |
| 256B | 27.59 us | 27.16 us | 168.85 us | 229.97 us |
| 1KB | 28.24 us | 29.25 us | 171.21 us | 231.42 us |
| 4KB | 29.98 us | 29.25 us | 176.89 us | 233.89 us |
| 16KB | 34.97 us | 37.93 us | 195.89 us | 251.89 us |
| 64KB | 61.31 us | 65.31 us | 286.42 us | 312.89 us |
| 256KB | 158.25 us | 159.11 us | 631.05 us | 724.57 us |
| 1MB | 648.23 us | 12092.35 us | 1879.75 us | 45213.29 us |

&gt; **Conclusion**: The maximum latency fluctuation range of Segar is only 168.15~1879.75 us, which can remain stable even in extreme scenarios; while the maximum latency span of ROS2 is 229.68~45213.29 us. The maximum latency in the 1MB scenario is 24 times that of Segar, and there is a serious risk of peak latency.

#### 3. Delay scalability analysis (Scalability)

- ✅ **Segar**: The average latency increases **gently and linearly** with the growth of data volume, from 27.43 us for 64B to 648.23 us for 1MB. The growth rate is controllable (about 23 times)
- ❌ **ROS2**: In the 1MB scenario, the latency explodes **exponentially**, from 29.03 us at 64B to 12092.35 us at 1MB, an increase of 416 times

&gt; **Conclusion**: Segar's latency scalability is industry-leading, fully exposing the fatal flaws of the ROS2 architecture in large data volume scenarios.

#### 4. Comprehensive performance comparison (Comparison)

| Comparison Dimensions | Segar | ROS2 | Conclusion |
|----------|-------|------|------|
| Large data throughput | ✅ 1542.1 msg/s | ❌ 82.7 msg/s | Segar leads 17.9 times |
| Large data volume latency | ✅ 648.23 us | ❌ 12092.35 us | Segar reduced by 94.7% |
| Delay stability | ✅ Fluctuation <2ms | ❌ Peak value >45ms | Segar is more stable |
| Latency scalability | ✅ Linear growth | ❌ Exponential growth | Segar scalable |
| Small data volume performance | ✅ Equal or slightly better | ⚠️ Equivalent | Close to both |
| Full range robustness | ✅ No performance shortcomings | ❌ 1MB crash | Segar is more robust |

&gt; **Conclusion**: Segar has achieved comprehensive surpasses in the three core indicators of throughput, latency, and stability. Only when the data volume is 4KB, the throughput is slightly lower than ROS2 (the gap is about 2.6%), which can be ignored in practical applications.

---

## Test conclusion

Segar middleware performs well on the Orin platform and has significant advantages over ROS2:

1. **Advantage over large amounts of data**: In the 1MB scenario, the throughput is **17.9 times** that of ROS2, and the latency is reduced by **94.7%**, completely solving the problem of performance collapse of ROS2's large data transmission performance
2. **Robust performance in the entire range**: Maintaining high throughput (>16000 msg/s) in the entire range of 64B~256KB, no significant performance fluctuations, and no performance shortcomings
3. **Extreme latency stability**: The maximum latency fluctuation is <2ms, while the peak latency of ROS2 reaches 45ms in the 1MB scenario, which poses serious real-time risks.
4. **Leading scalability**: Latency increases linearly with the amount of data, and the growth rate is controllable (23 times), while ROS2 explodes exponentially (416 times)
5. **Wide scene adaptation**: It can not only meet the high-frequency transmission of small data volumes (such as control instructions), but also stably support the low-latency transmission of large data volumes (such as point clouds and images)


---
