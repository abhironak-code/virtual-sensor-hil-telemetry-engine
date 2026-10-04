# Virtual Sensor HIL Telemetry Engine

**Author:** Abhisekh Routray  
**Project area:** IoT, embedded systems and Linux system programming  
**Languages:** C and C++17  
**Platform:** Ubuntu/Linux

## About the project

This project simulates a heated chamber and its sensors. It generates temperature, humidity and pressure readings. A C++ application reads the sensor data, controls a simulated heater using a PID controller, checks for sensor faults and stops heating if the temperature becomes too high.

The application also runs a local web dashboard. The dashboard displays sensor readings and system status, and lets the user change the target temperature and test sensor faults. The readings are saved in a CSV file.

The project has two modes. Simulator mode generates readings in the application and is the easiest way to run it. Driver mode reads data through the Linux character device `/dev/vsensor0` after the kernel driver has been built and loaded.

## Requirements

- Ubuntu or another Linux distribution
- GCC and G++ with C++17 support
- GNU Make
- `curl` for the integration tests
- Linux kernel headers matching the running kernel, if using driver mode
- A web browser

The application uses C++17. The Linux character-device driver is written in C. No separate Python or Node.js installation or external database is needed.

To install the basic packages on Ubuntu, run:

```bash
sudo apt update
sudo apt install -y build-essential git curl
```

For driver mode, install the headers for the current kernel:

```bash
sudo apt install -y linux-headers-$(uname -r)
```

## Build the project

After extracting the ZIP, open the `virtual-sensor-hil-telemetry-engine-v3` folder in Terminal:

```bash
cd ~/virtual-sensor-hil-telemetry-engine-v3
```

Build the application:

```bash
make
```

After the build finishes, the executable is available at `build/bin/hil_server`.

## Run in simulator mode

Start the application:

```bash
./build/bin/hil_server --mode sim
```

Keep the Terminal open. Open a browser and go to:

http://localhost:8080

The dashboard displays the simulated sensor readings and available controls. Press `Ctrl+C` in the Terminal to stop the application. The readings are saved to `telemetry.csv` in the directory from which the application was started.

Some useful options are:

```bash
./build/bin/hil_server --mode sim --port 8080
./build/bin/hil_server --mode sim --period-ms 100
./build/bin/hil_server --mode sim --log telemetry.csv
```

For local use, keep the server bound to localhost. The dashboard does not have login protection, so it should not be exposed to an untrusted network.

## Run the tests

Run these commands from the project folder:

```bash
make test
make itest
```

`make test` runs the unit tests. `make itest` runs the HTTP integration tests. In the development run for this project, the integration tests reported 12 passed and 0 failed.

## Run in driver mode

Driver mode requires matching Linux kernel headers and permission to load a kernel module. From the project folder, build the driver:

```bash
make driver
```

Load the driver:

```bash
./scripts/load_driver.sh
```

Start the application in driver mode:

```bash
./build/bin/hil_server --mode driver
```

Open http://localhost:8080 in the browser. When finished, stop the application with `Ctrl+C`, then unload the driver:

```bash
./scripts/unload_driver.sh
```

If driver mode does not start, check that the module built successfully and that `/dev/vsensor0` is available. Simulator mode can be used without loading the driver.

## Main project files

- `driver/vsensor.c` — Linux character-device driver
- `include/vsensor_ioctl.h` — interface shared by the driver and application
- `src/main.cpp` — application entry point and control loop
- `src/control.hpp` — PID control and sensor-health logic
- `src/sensor_source.*` — simulator and driver sensor sources
- `src/web_server.*` — local web server
- `src/dashboard_page.hpp` — dashboard page and browser-side code
- `tests/test_main.cpp` — unit tests
- `tests/integration_test.sh` — HTTP integration tests
- `scripts/` — driver load and unload scripts
- `docs/` — project stage documents and design notes
- `Makefile` — build and test commands

## Project documents

The `docs` folder contains the project introduction, requirements, design, architecture, prototype notes, testing notes, final report notes and development process notes.

## Author

Abhisekh Routray
