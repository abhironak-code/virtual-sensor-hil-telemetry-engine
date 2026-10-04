# Stage 6 – Final Report

## Project summary

The Virtual Sensor Hardware-in-the-Loop (HIL) Telemetry Engine is a Linux-based project for testing embedded control logic without physical sensors. A C driver models a virtual sensor device, and a C++17 application reads the data, runs a PID controller, checks sensor faults and provides a local web dashboard.

## Main features

The project supports simulated temperature, humidity and pressure readings, PID-based heater control, detection of spikes, stuck readings and dropouts, and an over-temperature cut-off. The dashboard displays readings and status and allows the user to change the target temperature and select test faults. Telemetry is saved in CSV format.

The application can use either the software simulator or the Linux character device `/dev/vsensor0`. Simulator mode is simpler to run because it does not require loading a kernel module.

## Test results

The unit tests completed successfully and reported `ALL TESTS PASSED`. The PID simulation reached approximately 59.95°C with 36% heater output for a 60°C target. The HTTP integration tests reported 12 passed and 0 failed. These results come from the software test environment and do not represent measurements from physical hardware.

## Limitations

The heated-chamber model is simplified and may not match real equipment. The current project uses one virtual sensor device. The dashboard is intended for local use and does not include authentication or encrypted remote access. Driver mode depends on kernel headers that match the running Linux kernel.

## Future improvements

The project could be extended to support physical sensors over I2C or SPI, multiple devices, more sensor models, secure remote access, live dashboard updates and long-term storage of telemetry. Additional fault cases and configurable PID settings could also be added.

## Conclusion

This project combines Linux driver concepts, C++ system programming, PID control, sensor-fault handling, a web dashboard and automated testing in one software test bench. The simulator and its automated tests provide a way to try the main functions without physical sensors. Driver-mode results depend on testing the module on the target Linux system.
