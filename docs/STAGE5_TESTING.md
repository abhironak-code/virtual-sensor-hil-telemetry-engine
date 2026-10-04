# Stage 5 – Testing

## Running the tests

Run the tests from the project root:

```bash
make test
make itest
```

The unit tests check the PID controller, sensor-health logic and simulator fault handling. The HTTP integration tests start the application in simulator mode and use `curl` to check the dashboard and API behaviour.

## Unit test result

The unit test run completed successfully and reported `ALL TESTS PASSED`. In the PID simulation, the temperature reached approximately 59.95°C with 36% heater output for a target of 60°C. This is a simulated result, not a physical measurement.

## Integration test result

The integration test run completed with 12 passed and 0 failed. The checks cover the dashboard page, data API, simulator source, valid and invalid setpoints, unknown routes, fault modes and CSV logging.

## Driver mode

Driver mode depends on a compatible Linux kernel and matching kernel headers. It uses `/dev/vsensor0` and should be checked separately from simulator mode. The main checks are whether the module builds and loads, the device file appears, readings update, setpoint changes work, the supported faults can be observed and the module can be unloaded after the application stops.

## Improvements in the implementation

The code includes PID anti-windup, derivative-on-measurement, sensor-health checks, a last-known-good reading, an over-temperature cut-off, a fixed-rate control loop and mutex protection for shared data. Compiler warnings are enabled in the main Makefile.
