# Integration Test Plan

## Goals
- Verify publish/subscribe stability: loss rate and repeat ratio.
- Verify end-to-end latency from sender timestamp to first-input receive time.
- Verify parameter timer behavior: periodic load/list/dump and dump output files.
- Verify service/client sync and async calls.
- Monitor process health and resource usage (CPU/MEM/RSS).

## Topology Summary
- Sensor nodes:
  - `sensor_node` reads `sensor_config.json` and publishes 10 topics.
- Compute nodes:
  - NodeA: sensor1/4/6/7 -> NodeA
  - NodeB: sensor2/3/6/8/9 + NodeA -> NodeB
  - NodeC: sensor8/9/10 + NodeA + NodeB -> NodeC1..NodeC4
  - NodeD/E/F: NodeC1..NodeC4 -> NodeD/NodeE/NodeF
  - NodeG: NodeB + NodeD + NodeE + NodeF -> NodeG
  - NodeH: NodeA + NodeB; provides AddTwoInts service
- Param timer:
  - `param_timer_component` periodically load/list/dump parameters to `params/dump/*.yaml`.

## Service/Client Coverage
- Service:
  - NodeH provides `add_two_ints`.
- Clients:
  - NodeA-F issue sync and async requests on a fixed interval.
- Validation signals:
  - Server: `NodeH add_two_ints request`
  - Client sync: `sync add_two_ints response=`
  - Client async: `async add_two_ints response=`

## Logging and Metrics
- Sender logs:
  - `ITEST_SEND` (sensor nodes)
- Receiver logs:
  - `ITEST_LAT` (latency for first input)
- Parsing outputs:
  - `loss_report.csv` (loss rate, repeat ratio)
  - `latency_report.csv` (avg/P50/P90/P99/min/max)
  - `param_timer_check.txt`
  - `service_check.txt`
- Resource monitoring:
  - `resource.csv` with CPU/MEM/RSS snapshots.

## Running the Test
- Install output directory:
  - `build_x86/output/integration_test`
- Start:
  - `./run_itest.sh [integration_dir] [log_dir] [duration_min]`
  - Ctrl+C or timeout stops all processes and triggers parsing.
- Parse only:
  - `./parse_itest_logs.py logs/itest_YYYYMMDD_HHMMSS`

## Pass/Fail Guidance
- Loss rate:
  - < 0.1% normal
  - 0.1% to 1% watch
  - > 1% investigate
- Latency:
  - P99 stable without large spikes.
- Param timer:
  - `tick_count > 0` and dump files present.
- Service:
  - `sync_responses` and `async_responses` > 0
  - `server_requests` > 0

## Key Files
- Sensor: `src/integration_test/sensor_node/*`
- Compute: `src/integration_test/compute_node/*`
- DAGs: `src/integration_test/compute_node/dag/*.dag`
- Params: `src/integration_test/params/node_c.yaml`
- Tools: `src/integration_test/tools/run_integration_test.sh`, `src/integration_test/tools/parse_itest_logs.py`
