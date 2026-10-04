# Stage 4 – Prototype

## Implemented parts

The prototype includes a software sensor source, a C++ control application, PID control, sensor-health checks, a local HTTP server, a browser dashboard and CSV logging. A Linux character-device driver is also included for driver mode.

## Build the application

Run these commands from the project root:

```bash
make
make test
```

## Run simulator mode

Start the application:

```bash
./build/bin/hil_server --mode sim
```

Keep the terminal open and visit `http://localhost:8080` in a browser. The dashboard shows the simulated readings and provides controls for the target temperature and supported fault modes. The application writes telemetry to `telemetry.csv` by default. Press Ctrl+C in the terminal to stop the server.

## Run driver mode

Driver mode requires a compatible Linux system and matching kernel headers. Build and load the module, then start the application:

```bash
make driver
./scripts/load_driver.sh
ls -l /dev/vsensor0
./build/bin/hil_server --mode driver
```

When finished, press Ctrl+C to stop the application and unload the module:

```bash
./scripts/unload_driver.sh
```

## Common problems

If the driver does not build, check that the kernel headers match the running kernel. If `/dev/vsensor0` does not appear, check whether the module loaded successfully. If the dashboard does not open, confirm that the server is running and that the browser is using the correct port. If port 8080 is already in use, try another port, for example `--port 8090`.

## Integration

The parts are connected step by step: sensor source, control logic, CSV logging, HTTP API and dashboard. This keeps the code easier to understand and makes it simpler to locate problems.
