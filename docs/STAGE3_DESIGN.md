# Stage 3 – System Design

## Design overview

The project is split into separate parts for sensor readings, control logic, the web server and testing. The browser communicates with the C++ application through HTTP. The control thread reads data from either the Linux driver or the software simulator.

## How the parts work together

The sensor source provides temperature, humidity and pressure readings. The control logic checks the readings and calculates heater output using the PID controller. The application stores the latest readings and status, and the web server sends this information to the dashboard as JSON. Dashboard controls update the target temperature or select a test fault. The application records telemetry in `telemetry.csv`.

## Source files

- `driver/vsensor.c` contains the Linux character-device driver.
- `include/vsensor_ioctl.h` defines the shared data structures and driver commands.
- `src/sensor_source.hpp` and `src/sensor_source.cpp` handle the simulator and driver sources.
- `src/control.hpp` contains the PID controller and sensor-health checker.
- `src/sample.hpp` defines a sensor sample used by the application.
- `src/main.cpp` starts the control loop and connects the application components.
- `src/web_server.cpp` and `src/web_server.hpp` implement the HTTP server.
- `src/dashboard_page.hpp` contains the dashboard page.
- `tests/` contains unit and HTTP integration tests.

## Data and control

A sensor sample contains the readings and status information used by the application. The driver interface also provides fault settings and statistics. Shared application data contains recent samples, events, the target temperature and selected fault mode. A mutex protects this shared data so the web server and control thread can use it safely.

## Development environment

The project is intended to run on Ubuntu or another compatible Linux distribution. Install the basic build tools with:

```bash
sudo apt update
sudo apt install -y build-essential git curl
```

For driver mode, also install headers that match the running kernel:

```bash
sudo apt install -y linux-headers-$(uname -r)
```

Simulator mode can be built and run without loading the kernel module.
