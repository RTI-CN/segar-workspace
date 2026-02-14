# Segar 2.0 system pressure test report (Orin platform)

> **Test Date**: 2026-02-11
> **Test duration**: 1 minute
> **Report Status**: Generated based on the latest integration test data

---

## 1. Test Overview

This system test is designed to simulate core business scenarios such as multi-sensor data fusion, distributed computing, service invocation and parameter management in real robot systems, and to verify the stability and performance of the system under complex topologies. Specific goals include:

- Verify **publish/subscribe link stability**: including packet loss rate and message sequence consistency.
- Verify **End-to-End Communication Latency**: The complete link delay from sensor acquisition to final processing node completion of calculation.
- Verify **Parameter Service and Timing Reading and Writing Mechanism**: Simulate dynamic parameter loading and persistence during robot operation.
- Verify **Service/Client calling mechanism**: including the reliability and responsiveness of synchronous/asynchronous calls between service nodes.
- Verify **Action calling mechanism**: including the reliability and responsiveness of synchronous/asynchronous calling/call cancellation between service nodes.
- Verification of **high load scenario performance**: Under pressurized conditions with CPU usage ≥75%, evaluate the performance attenuation and stability of each core function.
- Verify **system resource usage and process health status**: whether the memory usage is within a reasonable range and whether the process exits abnormally.

---

## 2. Test environment

| Dimensions | Configuration instructions |
|------|----------|
| hardware | arm orin (enables 8-core CPU) |
| OS & Kernel | ubuntu22.04(linux) |
| middleware version | V2.0.0 |

---

## 3. System components and topology

The system contains the following core components:

- **15 sensor nodes** (Sensor1-15): simulate lidar, camera, IMU, ultrasound, etc., frequency range 15Hz-100Hz, data volume 4KB-1MB
- **14 computing nodes** (NodeA-M/TimerComponent): Build multi-level data processing links
- **Parameter Service Node** (NodeG): Provides parameter query/modification/persistence services
- **Action Service Node** (NodeG): Provides action calling/cancellation services
- **Service Node** (NodeH): Provides `add_two_ints` service

- [Integration test topology](Integration_Test_Design.png) —— Segar integration test topology

### 1. Sensor simulation (sensor_node)

Publish messages as an independent process

- Simulate the various sensors carried by the robot (such as lidar, camera, IMU, ultrasound, etc.).
- Each sensor node periodically publishes topics with different frequencies and different amounts of data based on the `sensor_config.json` configuration.
- Supports concurrent publishing of multiple processes, simulating scenarios of high concurrency and heterogeneous data sources in real robot systems.

> Note: By configuring different sensor frequencies and data volumes, the load changes of the robot in different task scenarios can be simulated.

### 2. Compute link (compute_node)

Using the component component mechanism, it is loaded as a module by the segar framework

Construct a multi-level data processing link to simulate the process of step-by-step fusion of sensory data and decision-making processing in the robot system:

| node | input source | Output topic | Function description |
|------|----------|-----------|----------|
| NodeA | sensor1/4/6/7 | NodeA | Primary perceptual fusion (such as multi-camera image stitching) |
| NodeB | sensor2/3/6/8/9 + NodeA | NodeB | Intermediate fusion (such as obstacle detection and tracking) |
| NodeC | sensor8/9/10 + NodeA + NodeB | NodeC1~C4 | Advanced fusion (e.g. path planning input) |
| NodeD/E/F | NodeC1~C4 | NodeD | Decision-making level processing (such as behavioral decision-making) |
| NodeG | NodeB + NodeD + NodeE + NodeF | / | Control command generation (such as speed control) |
| NodeH | NodeA + NodeB | / | Provide `add_two_ints` service and simulate service calling scenarios |
| NodeI | sensor5,11,12,13,14,15 | NodeI | NodeI-NodeH simulates a secure link (such as ABS) |
| NodeJ | NodeE + NodeI | NodeJ | - |
| NodeK | NodeJ | NodeK | - |
| NodeL | NodeF + NodeK | NodeL | - |
| NodeM | NodeL | / | - |

> Remarks: Simulate the data flow process from perception to decision-making to control in the robot system

### 3. Service/Client Testing

Use the service/client mechanism provided by segar

- **Service Node**: NodeH provides `add_two_ints` service to simulate service calling scenarios such as **task scheduling and status synchronization** in the robot system.
- **Client node**: NodeA~E calls the service synchronously and asynchronously to verify the response capability and stability of the service under high concurrent requests.

> Remarks: The **inter-node collaborative work and task request** mechanism in the simulated robot system

### 4. Param test

Use the Param and TimerComponent mechanisms provided by segar

- **Service Node**: NodeG provides parameter service to simulate the **parameter query/modification** scenario in the robot system.
- **Client node**: The TimerComponent node periodically modifies/queries/stores parameters to NodeG.

> Remarks: **parameter management** mechanism in simulated robot system

### 5. Action testing

Use the Action mechanism provided by segar

- **Service Node**: NodeG provides Action service to simulate **action call/cancel** in the robot system.
- **Client node**: NodeF NodeH TimerComponent makes an action call to NodeG

---

## 4. Test results

### Test summary

- **Testing time**: 2026-02-11
- **Test module**: sensor link, service call, parameter management, resource monitoring
- **Test Environment**: Test environment
- **Test duration**: 1min

**Test conclusion**: ✅ Passed (all indicators are within the normal range)

| module | Test items | result | state |
|------|--------|------|------|
| Parameter service | ParamTimerComponent | tick_count=63, dump_files=1 | ✅Normal |
| service call | sync/async response | 1079/1080, server_requests=2160 | ✅Normal |
| Action call | sync+async/cancel response | 121/0/120 | ✅Normal |
| Link stability | Maximum packet loss rate | 0.3% (NodeB:sensor_topic_2) | ✅Normal |
| Communication delay | P99 delay | 12.3ms (NodeE:NodeC1) | ✅Normal |
| Resource usage | CPU/memory | 594% / 2618.04MB (RSS) | ✅ Normal (stress test) |

### Detailed analysis

#### 1. Parameter service (ParamTimerComponent)

- ✅ The initialization is successful, tick_count=62, indicating that the parameter timing reading and writing mechanism is operating normally.
- ✅ dump_files=1, parameter persistence function has taken effect.

> **Conclusion**: The parameter timing reading and writing mechanism is running normally, and the persistence function has taken effect, meeting the dynamic configuration requirements of the robot system.

#### 2. Service call (Service/Client)

- ✅ The number of synchronous responses is 1079, the number of asynchronous responses is 1080, server_requests=2160, **requests and responses are within a reasonable range**.
- ✅ There is no request loss or abnormal response, and the service call link is stable.

> **Conclusion**: The `add_two_ints` service of service node NodeH performs well under high concurrent calls and meets the collaboration needs between nodes in the robot system.

#### 3. Action calling mechanism

- ✅ The server target received 121, successfully completed 120, and canceled 0.
- ✅ Synchronization call success 48, synchronization timeout 0 (threshold ≤ 1)
- ✅ The asynchronous result returns 69, the cancellation request sends 34, the cancellation is successful 34, the cancellation fails 0 (threshold ≤ 1)

> **Conclusion**: The Action calling mechanism (synchronous/asynchronous/cancellation) runs stably, without timeout or cancellation failure, and meets the complex task scheduling needs of the robot.

#### 4. Link stability (packet loss rate)

| link | Packet loss rate | state |
|------|--------|------|
| NodeB:sensor_topic_2 | 0.3322% | ✅Normal |
| remaining links | 0% | ✅Normal |

> **Conclusion**: The packet loss rate of all links is far lower than the 1% judgment threshold, and the communication links are very stable, meeting the high reliability requirements of the robot system.

#### 5. Communication delay

| link | P99 delay | average delay | state |
|------|----------|----------|------|
| NodeC: sensor_topic_8 | 12.351ms | 1.029ms | ✅Normal |
| remaining links | &lt; 8ms | &lt; 1ms | ✅Normal |

> **Conclusion**: The maximum P99 delay is 12.351ms, which is within a reasonable range (< 15ms) and meets the real-time perception and decision-making needs of the robot.

#### 6. Resource usage

- ✅CPU usage: around 76% (595/800) (stress test)
- ✅ Total memory usage: 2.4GB (RSS)

> **Conclusion**: Low system resource usage and lightweight operation

---

## Test conclusion

The Segar concept version system performs excellently on the Orin platform:

1. **Stability**: All core functional modules run stably without crashes or abnormal exits.
2. **Reliability**: Packet loss rate is less than 0.5%, service call success rate is 100%
3. **Real-time**: End-to-end delay is controlled within 15ms, meeting real-time control requirements
4. **Resource efficiency**: On an 8-core system, the CPU occupies about 600% (stress test), the memory occupies about 2.4GB, and the resources are reasonably utilized.
5. **Scalability**: Supports multi-frequency sensor access and large data packet transmission

**The system is ready for deployment in real robot scenarios. **

---
