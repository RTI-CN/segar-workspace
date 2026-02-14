# Use `example.launch` to start multiple Segar processes locally

## 0. When deploying a local application, you need to do the following two things:

1. Write a `example.launch` (pull up multiple Segar components or local applications at the same time).
2. Write a local `start_example_launch.sh` script to start/stop `example.launch` or more launch files.

## 1. `example.launch` example

`src/launch_example/example.launch` is a launch file that can be directly referenced. It manages multiple dag processes and ordinary processes, and supports basic EM (execution management) strategies (such as automatic restart on failure):
Notice:

- `library` module uses `dag_conf` + `process_name`.
- The `binary` module must write an executable command line.
- `binary`'s `<process_name>` does not do shell variable expansion.

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
- `flag_file_path`: Load gflags file (process-level global). Components in the same `mainboard` process share this flags.
- `config_file_path`: component private configuration file path. Use `GetProtoConfig()` to read and use it in the component code.
- For related examples and usage, see `Segar_Component.md`.
