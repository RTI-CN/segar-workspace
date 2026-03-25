# Use `example.launch` to start multiple Segar processes locally

## 0. When deploying a local application, you need to do the following two things:

1. Write an `example.launch` (launch multiple Segar components or local applications at the same time).
2. Write a local `start_example_launch.sh` script to start/stop `example.launch` or more launch files.

## 1. `example.launch` example

`src/launch_example/example.launch` is a launch file that can be directly referenced. It manages multiple dag processes and ordinary processes, and supports basic EM (execution management) strategies (such as automatic restart on failure):
Note:

- `library` module uses `dag_conf` + `process_name`.
- `binary` modules must write executable command lines.
- `<process_name>` of `binary` does not do shell variable expansion.

## 2. `start_example_launch.sh` example

`src/launch_example/start_example_launch.sh` is a launch file that can be directly referenced:

Empower and start/view running status/stop:

```bash
chmod +x start_example_launch.sh
./start_example_launch.sh start
./start_example_launch.sh status
./start_example_launch.sh stop
```
## 3. The flag_file_path and config_file_path in the .dag file can perform process-level/dag-level personalized context configuration in a more fine-grained manner.
- `flag_file_path`: Load gflags file (process-level global). Components within the same `mainboard` process share these flags.
- `config_file_path`: component private configuration file path. Use `GetProtoConfig()` in the component code to read and use it.
- See `Segar_Component.md` for related examples and usage.
