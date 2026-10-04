# Virtual Sensor HIL Telemetry Engine - build file
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread -Iinclude -Isrc
BIN       = build/bin

.PHONY: all test itest driver run clean

all: $(BIN)/hil_server

$(BIN)/hil_server: src/main.cpp src/sensor_source.cpp src/web_server.cpp $(wildcard src/*.hpp) include/vsensor_ioctl.h
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) src/main.cpp src/sensor_source.cpp src/web_server.cpp -o $@

$(BIN)/unit_tests: tests/test_main.cpp src/sensor_source.cpp $(wildcard src/*.hpp)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_main.cpp src/sensor_source.cpp -o $@

test: $(BIN)/unit_tests
	./$(BIN)/unit_tests

itest: all
	./tests/integration_test.sh

driver:
	$(MAKE) -C driver

run: all
	./$(BIN)/hil_server

clean:
	rm -rf build telemetry.csv
	-$(MAKE) -C driver clean
