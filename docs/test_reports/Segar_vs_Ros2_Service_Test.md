# Segar/ROS2 Service performance comparison test report (x86 platform)

---

## 1. Test Overview

This test follows the hardware and software environment of the Topic test, switches to the Service/Client (request-response) model, and quantitatively compares the **TPS (call/second)** and **average/quantile/maximum delay (μs)** of Segar and ROS2 under a total of 8 payloads of 64B → 1MB, providing a basis for selection for autonomous driving and industrial control scenarios that require remote process calls (parameter delivery, configuration reading and writing, image servitization, etc.). Specific goals include:

- Verify **service call throughput (TPS)**: the number of requests and responses that can be completed per unit time
- Verification **Average response delay**: service response time in typical scenarios
- Verification **P99 quantile delay**: the upper limit of service response time for most requests
- Verification **Maximum Latency (Tail Latency)**: Service response time under extreme circumstances
- Verification of **large data volume service performance**: service capability under a large load of 256KB-1MB
- Validating **Latency Predictability**: Concentration of latency distributions and long-tail risk

---

## 2. Test environment

| Dimensions | Configuration instructions |
|------|----------|
| hardware | x86 (28-core CPU) |
| OS & Kernel | ubuntu22.04(linux) |
| ROS2 version | Humble |
| Segar version | V2.0.0 |
| test mode | Single-threaded Client loop call, Service side echoes immediately |

---

## 3. Test design and topology

The test uses the "client-server" request-response **Ping-Pong mode**:

- **Client node**: Single thread calls Service in a loop, records sending timestamp, and counts 10,000 RTTs
- **Server node**: After receiving the request, the original load will be echoed immediately without any business logic processing.
- **Load gradient**: 256B, 1KB, 4KB, 16KB, 64KB, 256KB, 1MB (7 levels in total)

The system includes the following core testing mechanisms:

- **Calling method**: Synchronous blocking call to ensure strict sequential execution
- **Statistical indicators**: TPS, average RTT, P50/P90/P99 quantile delay, maximum RTT
- **Number of iterations**: 10,000 calls per load to ensure statistical significance

### 1. Service definition (service)

The test uses a unified Service interface definition to ensure that the two middleware test benchmarks are consistent:

| Field | type | illustrate |
|------|------|------|
| request_data | uint8[] | Request data payload, populated as per test configuration |
| response_data | uint8[] | Response data load, echo with the same length as the request |

> Note: By configuring different data amounts, the service call load changes of the robot in different task scenarios are simulated.

### 2. Test topology

Using the Ping-Pong mechanism, loaded as a module by the test framework:

Build a request-response link to simulate the process of **remote procedure call** in the robot system


| step | Client | Server(server) | Function description |
|------|----------------|----------------|----------|
| 1 | Generate request and record timestamp | - | Initialize service call |
| 2 | Send service request | receive request | Establish communication link |
| 3 | Block waiting for response | Immediately echo the original load | Calculate round trip delay |
| 4 | Receive response and calculate RTT | - | Statistical performance indicators |
| 5 | Make the next call immediately | - | Continuous stress testing |

> Note: Simulate the calling process of parameter distribution, configuration reading and writing, and algorithm servitization in the robot system

### 3. Key parameter configuration (configuration)

Use unified QoS and call configuration:

- **Server**: NodeB provides instant echo service, simulating the **stateless computing service** scenario in the robot system
- **Client**: NodeA is called cyclically at the maximum frequency of a single thread to verify the responsiveness and stability of the middleware under serial requests.

> Note: **Strict sequence control and state synchronization** mechanism in the simulated robot system

### 4. Metrics calculation method

- **TPS (calls/second)** = number of test iterations / total time taken (seconds)
- **Average delay (avg-RTT)** = sum of RTT of all iterations / number of iterations (unit: microseconds)
- **P50 quantile delay** = upper limit of RTT for 50% of requests (unit: microseconds)
- **P90 quantile delay** = upper limit of RTT for 90% of requests (unit: microseconds)
- **P99 quantile delay** = upper limit of RTT for 99% of requests (unit: microseconds)
- **Maximum delay (max-RTT)** = the maximum value of RTT in all iterations (unit: microseconds)

> Note: **Service quality quantitative assessment** mechanism in the simulated robot system

---

## 4. Test results

### Test summary


- **Test module**: Service call, TPS test, delay test, tail delay analysis
- **Test environment**: x86 (28-core CPU), ubuntu22.04

**Test conclusion**: ✅ Segar is better than ROS2 in most data volume ranges

| Load gear | key indicators | Segar results | ROS2 results | Comparison conclusion |
|----------|----------|-----------|----------|----------|
| 256B small load | TPS | 10342.9 | 10850 | ⚠️ Quite |
| 256B small load | average delay | 95.6 us | 91.2 us | ⚠️ Quite |
| 1MB large payload | TPS | 961.7 calls/second | 116 calls/second | ✅ Segar leads 8.3 times |
| 1MB large payload | average delay | 715.0 us | 8076 us | ✅ Segar reduced by 91.1% |
| 1MB large payload | P99 delay | 1386.9 us | 25429 us | ✅ Segar reduced by 94.5% |

### Detailed analysis

#### 1. Small load and high frequency calls (256B – 4KB)


**256B scene:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 10342.9 | 10850 | ⚠️ Quite |
| average delay | 95.6 us | 91.2 us | ⚠️ Quite |
| P99 delay | 277.7 us | - | - |
| maximum delay | 1378.0 us | 1150 us | ⚠️ Quite |

**1KB scenario:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 11202.6 | 10200 | ✅ Segar 10% higher |
| average delay | 87.9 us | 96.5 us | ✅ Segar 9% lower |
| P99 delay | 237.1 us | - | - |
| maximum delay | 1393.8 us | 1280 us | ⚠️ Quite |

**4KB common configuration scenarios:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 16210.0 | 9200 | ✅ Segar is 76% higher |
| average delay | 58.9 us | 108.3 us | ✅ Segar is 46% lower |
| P99 delay | 127.4 us | - | - |
| maximum delay | 1279.8 us | 1520 us | ✅ Segar 16% lower |

- ✅ **Segar shows advantages in the 1KB-4KB range**, TPS overtakes and leads significantly, and latency is lower
- ⚠️ **64B small load scenario ROS2 has a slight advantage**, but Segar P99 and maximum delay fluctuate greatly (may be affected by system noise)
- ✅ **Segar performs well in 4KB scenarios**, with a TPS of 16210 and a latency of only 58.9us, suitable for data transmission with medium configurations

> **Conclusion**: Segar begins to show advantages under medium load (1KB-4KB) scenarios, but is slightly inferior to ROS2 under extremely small loads (64B), which may be related to the high proportion of system call overhead.

#### 2. Medium load batch data (16KB – 64KB)

**16KB bulk sensor data:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 21075.8 | 8517 | ✅ Segar is 147% higher |
| average delay | 42.8 us | 108.1 us | ✅ Segar 60% lower |
| P50 delay | 38.5 us | 107.6 us | ✅ Segar 64% lower |
| P90 delay | 52.5 us | 144.2 us | ✅ Segar 64% lower |
| P99 delay | 100.6 us | - | - |
| maximum delay | 263.6 us | 1850 us | ✅ Segar is 86% lower |

**64KB Medium Point Cloud Frame:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 6864.8 | 4395 | ✅ Segar is 56% higher |
| average delay | 117.7 us | 196.3 us | ✅ Segar 40% lower |
| P50 delay | 120.2 us | 188.5 us | ✅ Segar 36% lower |
| P90 delay | 158.9 us | 265.9 us | ✅ Segar 40% lower |
| P99 delay | 235.1 us | - | - |
| maximum delay | 622.9 us | 3850 us | ✅ Segar is 84% ​​lower |

- ✅ **Segar performs amazingly in the 16KB scene**, with TPS exceeding 21,000 and average latency of only 42.8us, the best in the entire range
- ✅ **Segar remains ahead in the 64KB scenario**, TPS is 1.56 times that of ROS2, and latency is controlled at the sub-millisecond level
- ✅ **Segar tail delay control is excellent**, the maximum delay of 16KB is only 263.6us, and ROS2 reaches 1850us

> **Conclusion**: Segar has significant advantages in medium-load batch data transmission scenarios. The 16KB scenario reaches the performance sweet spot, with extremely low latency and extremely high throughput, making it very suitable for service-based transmission of batch sensor data.

#### 3. Large load service-oriented transmission (256KB – 1MB)

**256KB large point cloud/image frame:**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 2714.5 | 2069 | ✅ Segar is 31% higher |
| average delay | 268.3 us | 399 us | ✅ Segar 33% lower |
| P50 delay | 267.0 us | 380 us | ✅ Segar 30% lower |
| P90 delay | 400.7 us | 558 us | ✅ Segar 28% lower |
| P99 delay | 515.6 us | 824 us | ✅ Segar 37% lower |
| maximum delay | 1284.7 us | 8240 us | ✅ Segar is 84% ​​lower |

**1MB extra large image frame (key scene):**

| index | Segar | ROS2  | contrast |
|------|--------------|-------------|------|
| TPS | 961.7 | 116 | ✅ Segar is 8.3 times higher |
| average delay | 715.0 us | 8076 us | ✅ Segar is 91.1% lower |
| P50 delay | 626.3 us | 2805 us | ✅ Segar is 78% lower |
| P90 delay | 1077.8 us | 22185 us | ✅ Segar 95% lower |
| P99 delay | 1386.9 us | 25429 us | ✅ Segar is 94.5% lower |
| maximum delay | 3059.5 us | 43959 us | ✅ Segar 93.0% lower |

- ✅ **1MB scene Segar TPS is 8.3 times that of ROS2**, and the average delay is only 8.9% (715us vs 8ms)
- ❌ **ROS2 has a P99 of 25ms+ and an extreme tail delay of 44ms**, which can no longer meet the budget of 100ms end-to-end real-time link.
- ✅ **Segar maintains sub-millisecond response even with large data volumes**, P99 is only 1.4ms, and can be safely used in scenarios such as "service-based lidar" or "remote image algorithm"

> **Conclusion**: Segar shows overwhelming advantages in the 1MB large load scenario, completely solves the performance collapse problem of ROS2 large data volume service calls, and meets the real-time requirements of the service-oriented perception algorithm.

#### 4. Full range performance trend analysis (Trend)

**Measured TPS trend (Segar):**

| load | Segar TPS | ROS2 TPS  | Advantages |
|------|------------------|-----------------|-----------|
| 256B | 10342.9 | 10850 | 0.95x |
| 1KB | 11202.6 | 10200 | **1.10x** |
| 4KB | 16210.0 | 9200 | **1.76x** |
| 16KB | 21075.8 | 8517 | **2.47x** |
| 64KB | 6864.8 | 4395 | **1.56x** |
| 256KB | 2714.5 | 2069 | **1.31x** |
| 1MB | 961.7 | 116 | **8.29x** |

**Measured delay trend (Segar):**

| load | Segar avg | ROS2 avg  | latency gap |
|------|------------------|-----------------|----------|
| 256B | 95.6 us | 91.2 us | 1.05x |
| 1KB | 87.9 us | 96.5 us | **0.91x** |
| 4KB | 58.9 us | 108.3 us | **0.54x** |
| 16KB | 42.8 us | 108.1 us | **0.40x** |
| 64KB | 117.7 us | 196.3 us | **0.60x** |
| 256KB | 268.3 us | 399 us | **0.67x** |
| 1MB | 715.0 us | 8076 us | **0.09x** |

- ✅ **Segar is better than ROS2** in the entire range of 1KB-1MB, and the advantage expands as the load increases, reaching 8.3 times at 1MB
- ✅ **Segar reaches the performance sweet spot at 16KB**, TPS exceeds 21,000, and the latency is only 42.8us, which is the best in the entire range
- ✅ **Segar latency growth is gentle**, only growing 16.7 times from 16KB to 1MB (42.8→715us)

> **Conclusion**: Segar leads the way in all workloads above 1KB, with 16KB being the optimal working point; ROS2 exposes DDS serialization and copy bottlenecks under large loads, with TPS falling below 120 at 1MB, and engineering availability has been lost.

#### 5. Tail Latency Risk Analysis (Tail Latency)

| load | Segar max | ROS2 max  | risk level |
|------|------------------|-----------------|----------|
| 256B | 1378.0 us | 1150 us | 🟢 Both are acceptable |
| 1KB | 1393.8 us | 1280 us | 🟢 Both are acceptable |
| 4KB | 1279.8 us | 1520 us | 🟢 Both are acceptable |
| 16KB | 263.6 us | 1850 us | ✅ Segar is extremely low |
| 64KB | 622.9 us | 3850 us | ✅ Segar is extremely low |
| 256KB | 1284.7 us | 8240 us | ✅ Segar is low |
| 1MB | 3059.5 us | 43959 us | ✅ Segar is extremely low |

- ✅ **Excellent Segar tail delay control in the 16KB-1MB range**, always <3.1ms, highly predictable

> **Conclusion**: Except for the 256B small load, Segar's tail delay control is extreme and meets the strict and predictable real-time requirements; ROS2's tail delay is out of control under large loads and cannot meet the deterministic needs of safety-critical scenarios such as autonomous driving.

---

## Test conclusion

Segar's Service/Client mechanism is overall better than ROS2 on the Orin platform, showing significant advantages for loads above 1KB:

1. **Small load high frequency call (256B)**: equivalent to ROS2, may be affected by the high proportion of system call overhead, but still meets the high frequency parameter reading and writing requirements
2. **Medium load batch transmission (1KB-64KB)**: TPS is significantly ahead (1.1-2.5 times), latency is reduced by 9-60%, **16KB reaches the performance sweet spot** (TPS 21075, latency 42.8us), suitable for batch sensor data service
3. **Large load service (256KB-1MB)**: TPS is 31%-8.3 times higher, latency is reduced by 33%-91%, tail latency is reduced from 44ms to 3ms, suitable for "service-based lidar" and "remote image algorithm"
4. **Latency Predictability**: Segar has a tail delay of <3.1ms in the 16KB-1MB range, and ROS2 has a latency of 44ms in the 1MB range. There is a serious deterministic risk.
5. **Engineering Availability**: ROS2's TPS fell below 120 at 1MB, which is no longer able to meet real-time service needs; Segar maintains 961 TPS and still has engineering value

**Key Findings:**

- 🎯 **Segar's best working point: 16KB**, TPS 21075, latency 42.8us, the best performance in the entire range
- 🎯 **Segar performs well in the 4KB-64KB range**, with TPS >6800 and latency <120us, suitable for most service-based scenarios
- 🎯 **Segar leads the way in the 1MB scenario**, TPS is 8.3 times that of ROS2, solving the pain points of large data volume service calls

**Selection suggestions:**

- ✅ **If the system requires**: millisecond or even sub-millisecond level remote call; transfer 1KB-1MB level point cloud/image; strictly predictable tail delay
**⇒ It is recommended to use Segar’s Service/Client mechanism first, and the optimal working point is 16KB load**

- ⚠️ **Read and write parameters with very small load (<256B) and non-critical path**: The performance of the two is equivalent

**The system is ready for deployment in real robot scenarios, especially suitable for: **
- Batch sensor data service (16KB-64KB)
- Service-oriented perception algorithm (256KB-1MB)
- Remote image processing and point cloud transmission (1MB large load)
- Industrial control scenarios with strict real-time requirements

---
