# Project Architecture

## How the project works

The browser opens the dashboard served by the C++ application. The browser sends HTTP requests to read the latest sensor data or change settings. The application keeps the current readings and commands in shared data protected by a mutex.

A separate control thread reads sensor data, checks the reading, calculates the heater output using the PID controller and saves the result. The sensor data comes from either the software simulator or the Linux device `/dev/vsensor0`. Simulator mode is easier to run because it does not need the kernel driver.

## Main parts of the code

- `driver/vsensor.c` contains the Linux character-device driver.
- `include/vsensor_ioctl.h` defines the data structures and commands shared by the driver and application.
- `src/sensor_source.hpp` and `src/sensor_source.cpp` provide the simulator and driver sensor sources.
- `src/control.hpp` contains the PID controller and sensor-health checks.
- `src/sample.hpp` defines the sensor sample used by the application.
- `src/main.cpp` runs the control loop and connects the main parts of the application.
- `src/web_server.cpp` and `src/web_server.hpp` handle HTTP requests.
- `src/dashboard_page.hpp` contains the dashboard page.
- `tests/` contains unit and HTTP integration tests.

## One control cycle

The control thread checks for new commands, reads a sensor sample and checks whether it is valid. It then calculates the heater output and stores the latest readings and status for the dashboard. This cycle normally runs every 100 milliseconds. Only the control thread accesses the sensor device; the web server updates shared commands. A mutex protects the shared application data.

## Sensor model and faults

The simulator and driver use a simple heated-chamber model. The heater raises the temperature, while the model gradually moves back towards an ambient temperature of about 25°C. This is a simplified software model and not a physical measurement.

The project can demonstrate stuck readings, spikes and dropouts. It also includes an over-temperature cut-off. When a reading is rejected or a read fails, the controller uses its last accepted reading or retains the previous heater output as implemented by the control logic.

## Driver interface

The application uses `read()` to get a sensor sample. It uses `ioctl()` commands to set heater output, select a fault mode, reset the device and read driver statistics. Heater output is represented as a percentage from 0 to 100. The supported fault modes are normal, stuck, spike and dropout.

## Web requests

- `GET /` opens the dashboard.
- `GET /api/data` returns the current source, setpoint, fault status, statistics, recent samples and events.
- `GET /api/setpoint?value=55` requests a new target temperature.
- `GET /api/fault?mode=spike` requests a fault mode. The supported modes are `none`, `spike`, `stuck` and `dropout`.

The dashboard is intended for local use. It does not provide login or encrypted remote access, so keep it on localhost during normal testing.
