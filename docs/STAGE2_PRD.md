# Stage 2 – Project Requirements

## Purpose

This document describes what the Virtual Sensor HIL Telemetry Engine should do and the main parts needed to build it.

## Scope

The project is a Linux-based virtual sensor test bench. It includes a C kernel driver, a C++17 application, a simulator, a local browser dashboard, CSV logging and automated tests. Physical hardware and public internet deployment are outside the current scope.

## Functional requirements

- Generate simulated temperature, humidity and pressure readings.
- Let the application read the virtual device and set heater output through the driver interface.
- Use PID control to move the temperature towards the target value.
- Detect abnormal readings, including spikes and stuck values.
- Handle missing sensor data without crashing.
- Turn off the heater when the over-temperature limit is reached.
- Display readings, events and status in the dashboard.
- Allow the user to change the target temperature and select supported fault modes.
- Save readings and control information to a CSV file.
- Allow simulator mode to run without loading the kernel driver.

## Other requirements

The application runs on Linux and uses C++17. The kernel module is written in C. The control loop normally runs every 100 milliseconds. A mutex protects shared application data. The server listens on localhost by default. The project includes unit tests and HTTP integration tests.

## Main modules

- `driver/vsensor.c`: models the virtual chamber and provides device operations.
- `include/vsensor_ioctl.h`: defines the shared driver interface.
- `src/sensor_source.cpp` and `src/sensor_source.hpp`: provide simulator and driver sensor sources.
- `src/control.hpp`: contains PID control and sensor-health checks.
- `src/main.cpp`: runs the control loop, web routes and CSV logging.
- `src/web_server.cpp` and `src/web_server.hpp`: handle HTTP requests.
- `src/dashboard_page.hpp`: contains the browser dashboard.
- `tests/`: contains unit and integration tests.

## Development plan

The work was divided into six stages: introduction, requirements, design, prototype, testing and final report. Each stage builds on the earlier work. The simulator is useful for checking the application without needing to load a kernel module. Driver mode is handled separately because it depends on the running Linux kernel and its matching headers.

## Completion checks

The application should build, unit tests and integration tests should run, the dashboard should display readings, setpoint validation should work, supported faults should be visible and telemetry should be saved to CSV. Driver mode should only be described as verified after it has been built and run on a compatible Linux system.
