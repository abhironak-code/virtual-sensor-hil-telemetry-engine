# Development Process

I divided the work into stages so that I could build and check the project in smaller parts instead of developing everything at once.

## Project stages

1. **Introduction:** decide the project idea, problem, objectives and scope.
2. **Requirements:** list the main functions and software requirements.
3. **Design:** decide how the driver, application, controller and dashboard work together.
4. **Prototype:** build the application and run the simulator; prepare the driver mode.
5. **Testing:** run unit and HTTP integration tests and check the main functions.
6. **Final report:** describe the implementation, results, limitations and possible improvements.

## Development approach

I used an iterative approach. I worked on one part at a time, built it, checked its behaviour and then connected it to the next part. For example, the sensor source and control logic are handled before the dashboard is tested with the full application.

Git can be used to keep track of source-code changes. The main project files and documentation are kept together so that the implementation and its explanation can be updated as the project changes.

## Testing

Run the unit tests from the project root with:

```bash
make test
```

Run the HTTP integration tests with:

```bash
make itest
```

Driver mode also needs a compatible Linux kernel and matching kernel headers. It should be checked separately from simulator mode.
