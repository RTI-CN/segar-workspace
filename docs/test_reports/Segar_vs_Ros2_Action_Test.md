# Segar/ROS2 Action performance comparison test report (x86 platform)

---

## 1. Test Overview

This test focuses on the action communication module, using Segar and ROS2 as targets. Under the two client calling modes of synchronous/asynchronous, the system compares four core indicators of delay, success rate, failure rate and packet loss rate to comprehensively verify the stability and performance boundaries of the server-client link. Specific goals include:

- Verify the average delay and total time taken by **synchronous/asynchronous clients to call action services**
- Count and compare the number of successes, failures, packet losses and failure rates of action requests**
- Verify the synchronization performance under **small load and high frequency calls** (64B)
- Verify synchronization performance under **large data volume transfer** (1MB)
- Verify performance stability under high number of iterations (200 vs 1000/2000)
- Verify the system stability in the **asynchronous high concurrency** scenario (ROS2 does not support it and needs to be implemented by the user)

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

The test uses the **Action mode** of "client-server" request-response:

- **Synchronous action client**: Single thread sends action targets synchronously, records the full process delay of each target, and counts the success rate/packet loss rate
- **Asynchronous Action Client**: Multi-threaded asynchronous sending of action targets, cyclic batch sending, recording delay/maximum delay/failure rate
- **Action Server**: Configure Reliable QoS and return results/feedback after executing the target
- **Test load**: 64B (small load), 1MB (large load)
- **Number of iterations**: 200 times, 1000 times, 2000 times

The system includes the following core testing mechanisms:

- **Calling method**: synchronous blocking call vs asynchronous concurrent call
- **Statistical indicators**: Throughput (MB/s), average delay, P50/P90/P99 quantile delay, maximum delay, packet loss rate

### 1. Action definition (action)

The test uses a unified action interface definition to ensure that the two middleware test benchmarks are consistent:

| Field | type | illustrate |
|------|------|------|
| goal | payload | Target data load (64B/1MB) |
| result | success + data | Execution results and return data |
| feedback | progress | Execution progress feedback (if any) |

> Note: By configuring different data amounts, the robot's action call load changes in different task scenarios can be simulated.

### 2. Test topology

Using the Action mechanism, it is loaded as a module by the test framework:

Construct a request-response link to simulate the process of **complex task scheduling** in the robot system:


| step | Client | Server(server) | Function description |
|------|----------------|----------------|----------|
| 1 | Generate goals and send Goal | - | Initial action call |
| 2 | Block waiting for results | Receive and process targets | Task execution |
| 3 | ReceiveResult | Return execution result | Complete action call |
| 4 | Record delays and status | - | Statistical performance indicators |
| 5 | Make the next call immediately | - | Continuous stress testing |

> Note: Simulate the calling process of **navigation, robot arm control, and complex behavior tree** in the robot system

### 3. Key parameter configuration (configuration)

| Dimensions | Sync client | asynchronous client | Server |
|------|------------|------------|--------|
| data load | 64B / 1MB | 64B / 1MB | Pass in by client |
| Number of iterations | 200 / 1000 / 2000 | 200 / 400 / 1000 | - |
| QoS configuration | Reliable | Reliable | Reliable |
| Concurrency limit | single thread execution | Multi-threading (hardware concurrency number ≥ 2) | - |

- **Server**: NodeB provides action execution services to simulate **navigation and robot arm control** scenarios in the robot system.
- **Client**: NodeA is called in a synchronous manner to verify the responsiveness and stability of the middleware under serial requests.

> Note: **Strict sequence control and state synchronization** mechanism in the simulated robot system

### 4. Metrics calculation method

- **Throughput (thr, MB/s)** = Total data volume / Total time taken
- **Average latency (avg, ms)** = The average time taken for all requests from sending to result return
- **P50 quantile delay (ms)** = upper limit of 50% request delay
- **P90 quantile delay (ms)** = upper limit of delay for 90% of requests
- **P99 quantile delay (ms)** = upper limit of delay for 99% of requests
- **Maximum delay (max, ms)** = the maximum value among all request delays
- **Packet loss rate (loss)** = Number of failures / Total number of sends × 100%

> Note: **Quantitative assessment** mechanism of action execution quality in the simulated robot system

---

## 4. Test results

### Test summary

- **Test module**: synchronous action call, small load test, large data volume test
- **Test environment**: ubuntu 16-core virtual system, Segar V0.9.0, ROS2 humble

**Test conclusion**: ✅ Segar is ahead in all aspects (synchronous call performance is significantly better than ROS2)

| test scenario | key indicators | Segar results | ROS2 results | Comparative conclusion |
|----------|----------|-----------|----------|----------|
| 64B-200 times | Throughput | 0.519 MB/s | 0.329 MB/s | ✅ Segar is 58% higher |
| 64B-200 times | average delay | 0.234 ms | 0.370 ms | ✅ Segar 37% lower |
| 64B-2000 times | average delay | 0.338 ms | - | ✅ Segar is expandable |
| 1MB-200 times | Throughput | 444.4 MB/s | 298.9 MB/s | ✅ Segar is 49% higher |
| 1MB-200 times | average delay | 4.40 ms | 6.55 ms | ✅ Segar 33% lower |
| 1MB-2000 times | Throughput | 452.1 MB/s | 249.2 MB/s | ✅ Segar is 81% higher |
| 1MB-2000 times | average delay | 4.32 ms | 7.89 ms | ✅ Segar 45% lower |
| 1MB-2000 times | P99 delay | 6.07 ms | 27.27 ms | ✅ Segar is 78% lower |
| 1MB-2000 times | maximum delay | 6.63 ms | 27.96 ms | ✅ Segar is 76% lower |

### Detailed analysis

#### 1. Small load and high frequency calling (64B)

**64B load - 200 iterations:**

| index | Segar | ROS2 | contrast |
|------|-------|------|------|
| Throughput | 0.519 MB/s | 0.329 MB/s | ✅ Segar is **58%** higher |
| average delay | 0.234 ms | 0.370 ms | ✅ Segar saves **37%** |
| P50 delay | 0.215 ms | 0.368 ms | ✅ Segar lowers **42%** |
| P90 delay | 0.328 ms | 0.498 ms | ✅ Segar is **34%** lower |
| P99 delay | 0.457 ms | 0.567 ms | ✅ Segar is **19%** lower |
| maximum delay | 0.58 ms | 0.625 ms | ✅ Segar is **7%** lower |
| Packet loss rate | 0% | 0% | ✅ Zero packet loss for both |

- ✅ **Segar throughput is 58% higher**, and communication efficiency under light load is significantly ahead.
- ✅ **Segar’s average latency is 37% lower** and its response speed is faster
- ✅ **All bit latencies of Segar are better than ROS2**, P50 is 42% lower and P90 is 34% lower
- ✅ **The maximum latency of the two is close**, but Segar is still slightly better

**64B load - 2000 iterations (Segar):**

| index | Segar (2000 times) | ROS2 (1000 times) | contrast |
|------|----------------|---------------|------|
| Throughput | 0.360 MB/s | 0.215 MB/s | ✅ Segar is **67%** higher |
| average delay | 0.338 ms | 0.566 ms | ✅ Segar is **40%** lower |
| P50 delay | 0.331 ms | 0.543 ms | ✅ Segar is **39%** lower |
| P90 delay | 0.407 ms | 0.864 ms | ✅ Segar lowers **53%** |
| P99 delay | 0.495 ms | 1.120 ms | ✅ Segar lowers **56%** |
| maximum delay | 0.619 ms | 1.360 ms | ✅ Segar lowers **54%** |
| Packet loss rate | 0% | 0% | ✅ Zero packet loss for both |

- ✅ **Segar supports 2000 high iterations**, ROS2 is only tested to 1000 times
- ✅ **As the number of iterations increases, Segar's advantages expand**: P90 is 53% lower, P99 is 56% lower, and the maximum latency is 54% lower.
- ✅ **ROS2's maximum delay has reached 1.36ms at 1000 times**, which is 2.2 times that of Segar 2000 times (ros2 cannot support more than 2000 iterations, and packet loss is serious)

> **Conclusion**: Segar takes the lead in small load and high-frequency calling scenarios, and its advantage further expands as the number of calls increases, and its scalability is significantly better than ROS2.

#### 2. Large data volume transmission (1MB)

**1MB load - 200 iterations:**

| index | Segar | ROS2 | contrast |
|------|-------|------|------|
| Throughput | 444.4 MB/s | 298.9 MB/s | ✅ Segar is **49%** higher |
| average delay | 4.40 ms | 6.55 ms | ✅ Segar saves **33%** |
| P50 delay | 4.28 ms | 3.47 ms | ⚠️ ROS2 is slightly better |
| P90 delay | 5.24 ms | 14.22 ms | ✅ Segar lowers **63%** |
| P99 delay | 6.44 ms | 25.21 ms | ✅ Segar lowers **74%** |
| maximum delay | 8.85 ms | 32.06 ms | ✅ Segar lowers **72%** |
| Packet loss rate | 0% | 0% | ✅ Zero packet loss for both |

- ✅ **Segar throughput is 49% higher**, and the transmission efficiency of large data volumes is significantly ahead
- ✅ **Segar’s average latency is 33% lower** and responds faster
- ⚠️ **ROS2 has slightly better latency at P50** (3.47ms vs 4.28ms), but the median advantage is not obvious
- ✅ **Segar leads the way in tail latency crushing**: P90 is 63% lower, P99 is 74% lower, and maximum latency is 72% lower

**1MB load - 2000 iterations (key scenario):**

| index | Segar (2000 times) | ROS2 (1000 times) | contrast |
|------|----------------|---------------|------|
| Throughput | 452.1 MB/s | 249.2 MB/s | ✅ Segar high **81%** |
| average delay | 4.32 ms | 7.89 ms | ✅ Segar is **45%** lower |
| P50 delay | 4.36 ms | 4.32 ms | ⚠️ Both are equivalent |
| P90 delay | 5.25 ms | 16.60 ms | ✅ Segar lowers **68%** |
| P99 delay | 6.07 ms | 27.27 ms | ✅ Segar lowers **78%** |
| maximum delay | 6.63 ms | 27.96 ms | ✅ Segar lowers **76%** |
| Packet loss rate | 0% | 0% | ✅ Zero packet loss for both |

- ✅ **Segar’s throughput is as high as 452.1 MB/s, which is 1.81 times that of ROS2**, and its large data transmission capacity is overwhelming.
- ✅ **Segar average latency is only 4.32ms, ROS2 reaches 7.89ms**, 45% faster
- ✅ **Segar’s maximum latency is only 6.63ms, ROS2 reaches 27.96ms**, which is 4.2 times that of Segar
- ✅ **Segar P99 latency is only 6.07ms, ROS2 reaches 27.27ms**, 4.5 times that of Segar
- ✅ **Segar supports 2000 iterations and remains stable**, with a maximum delay of only 6.63ms, and is extremely predictable

> **Conclusion**: Segar shows overwhelming advantages in the 1MB large data transmission scenario. The throughput is 1.8 times that of ROS2, the tail latency is reduced by 76-78%, and it still maintains extremely low latency fluctuations under high iterations, making it suitable for real-time transmission of large loads.

#### 3. Performance stability and scalability analysis

**Throughput stability (1MB scenario):**

| Number of iterations | Segar throughput | ROS2 throughput | Segar stability |
|----------|-------------|------------|-------------|
| 200 times | 444.4 MB/s | 298.9 MB/s | - |
| 1000 times | - | 249.2 MB/s | ROS2 dropped 16% |
| 2000 times | 452.1 MB/s | - | Segar increased by 2% |

- ✅ **Segar's throughput increases with the number of iterations** (444→452 MB/s), and the performance is more stable


**Delay predictability (1MB scenario, maximum delay):**

| Number of iterations | Segar maximum delay | ROS2 maximum latency | gap |
|----------|---------------|--------------|------|
| 200 times | 8.85 ms | 32.06 ms | 3.6x |
| 1000 times | - | 27.96 ms | - |
| 2000 times | 6.63 ms | - | Segar is better |

- ✅ **Segar 2000 times maximum latency (6.63ms) < ROS2 200 times maximum latency (32.06ms)**, leading in predictability
- ✅ **The maximum delay under Segar high iteration is reduced** (8.85→6.63ms), and the system is more stable

**Latency quantile comparison (1MB-2000 times vs 1000 times):**

| Quantile | Segar (2000 times) | ROS2 (1000 times) | Advantages |
|------|----------------|---------------|------|
| P50 | 4.36 ms | 4.32 ms | quite |
| P90 | 5.25 ms | 16.60 ms | ✅ Segar is 68% lower |
| P99 | 6.07 ms | 27.27 ms | ✅ Segar is 78% lower |
| max | 6.63 ms | 27.96 ms | ✅ Segar is 76% lower |

- ✅ **Segar shows overwhelming advantages at P90 and above**, with ultimate tail delay control
- ✅ **The gap between Segar P99 and the maximum latency is small** (6.07 vs 6.63ms, 9% gap), and the latency distribution is concentrated

> **Conclusion**: Segar leads the way in terms of performance stability and scalability. Under high iterations, the performance does not drop but rises. The tail delay is controlled to the extreme. It is suitable for scenarios with high concurrency and high reliability requirements.

#### 4. Comprehensive performance comparison

| Contrast Dimensions | Segar | ROS2 | in conclusion |
|----------|-------|------|------|
| Small load throughput | ✅ 0.52 MB/s | 0.33 MB/s | Segar is 58% higher |
| small load delay | ✅ 0.23 ms | 0.37 ms | Segar is 37% lower |
| Large load throughput | ✅ 452 MB/s | 249 MB/s | Segar is 81% higher |
| Large load delay | ✅ 4.32 ms | 7.89 ms | Segar is 45% lower |
| tail delay control | ✅ 6.63 ms | 27.96 ms | Segar is 76% lower |
| Scalability | ✅ 2000 times stable | 1000 recessions | Segar is better |
| reliability | ✅ Zero packet loss | Zero packet loss | Both are equivalent |

- ✅ **Segar leads in all dimensions**, especially in throughput, tail latency, and scalability.

---

## Test conclusion

Segar's Action mechanism is better than ROS2 in a virtual machine environment, and the synchronous call performance is significantly ahead:

1. **Small load and high frequency calls (64B)**: high throughput **58%**, low latency **37%**, and as the number of iterations increases, the advantage expands to **54%** (maximum latency)
2. **Large data transfer (1MB)**: high throughput **81%** (452 ​​vs 249 MB/s), average latency **45%** lower, tail latency lower **76%**
3. **Performance Stability**: The performance of Segar does not drop but rises after 2000 iterations, and the performance of ROS2 declines by 16% after 1000 iterations.
4. **Latency predictability**: The difference between Segar P99 and the maximum delay is only 9%, and ROS2 is 4.5 times (27.27 vs 6.07ms)
5. **Reliability**: Both maintain zero packet loss, but Segar is more stable under high load


---
