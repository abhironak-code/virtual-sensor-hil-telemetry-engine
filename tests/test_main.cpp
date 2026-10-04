// Dependency-free unit tests (run with: make test)
#include <cmath>
#include <cstdio>
#include "control.hpp"
#include "sensor_source.hpp"

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)

static void test_detector() {
    std::printf("anomaly_detector\n");
    HealthChecker d;
    CHECK(d.check(25.0) == Anomaly::None);
    CHECK(d.check(25.4) == Anomaly::None);
    CHECK(d.check(45.4) == Anomaly::Spike);
    CHECK(d.check(200.0) == Anomaly::OutOfRange);
    HealthChecker s;
    Anomaly last = Anomaly::None;
    for (int i = 0; i < 6; ++i) last = s.check(30.0);
    CHECK(last == Anomaly::Stuck);
}

static void test_pid_closed_loop() {
    std::printf("pid + simulated plant (closed loop)\n");
    auto src = makeSimSource(7);
    src->open();
    PID pid(8.0, 0.4, 1.0);
    Sample s; double heater = 0;
    for (int i = 0; i < 600; ++i) {                    // 60 s of simulated time
        CHECK(src->read(s) == ReadStatus::Ok);
        heater = pid.update(60.0, s.temp_c, 0.1);
        src->setHeater(static_cast<unsigned>(std::lround(heater)));
    }
    std::printf("  final T=%.2f heater=%.0f%%\n", s.temp_c, heater);
    CHECK(std::fabs(s.temp_c - 60.0) < 1.0);
    CHECK(heater > 20 && heater < 60);                  // ~35% expected at equilibrium
}

static void test_fault_modes() {
    std::printf("sim fault injection\n");
    auto src = makeSimSource(1);
    src->open();
    Sample s;
    src->setFault(3);
    CHECK(src->read(s) == ReadStatus::Dropout);
    src->setFault(0);
    CHECK(src->read(s) == ReadStatus::Ok);
    CHECK(!src->setHeater(101));
    src->setFault(1);
    src->read(s); double a = s.temp_c;
    src->read(s);
    CHECK(s.temp_c == a);                               // frozen
}

int main() {
    test_detector();
    test_pid_closed_loop();
    test_fault_modes();
    std::printf(g_fail ? "\n%d CHECK(S) FAILED\n" : "\nALL TESTS PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
