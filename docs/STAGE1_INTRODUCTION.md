# Stage 1 – Project Introduction

## Project overview

The Virtual Sensor Hardware-in-the-Loop (HIL) Telemetry Engine is a software project for testing embedded control logic without connecting physical sensors. It uses a Linux device driver written in C and a C++17 application.

The project represents a heated chamber and produces virtual temperature, humidity and pressure readings. The application reads the values, adjusts a simulated heater and checks for abnormal sensor readings. A local web dashboard shows the readings and lets the user change settings and try different fault conditions.

## Problem

Testing embedded software often depends on physical sensors and controllers. This can make testing more expensive and make some faults difficult or unsafe to reproduce. A software-based test bench makes it possible to try normal readings and selected sensor faults before using real hardware.

## Objectives

- Generate virtual temperature, humidity and pressure readings.
- Read sensor data through a Linux character driver or software simulator.
- Use a PID controller to adjust the simulated heater towards a target temperature.
- Detect sensor problems such as spikes, stuck readings and dropouts.
- Turn off the heater when the over-temperature limit is reached.
- Show readings and system status on a browser dashboard.
- Save telemetry in a CSV file.

## Scope

The project includes the Linux driver, simulator, C++ application, PID control, sensor-health checks, local web server, dashboard, CSV logging and automated tests. It does not currently use physical sensors or provide user accounts and secure remote access.

## Technologies

The kernel driver is written in C. The application uses C++17 and Linux system-programming features such as device files, `read()`, `ioctl()`, threads, mutexes, sockets and file I/O. GNU Make is used to build the project, and shell scripts are used for selected test and driver tasks.
