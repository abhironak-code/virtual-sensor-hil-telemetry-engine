#pragma once
// Control logic: a PID controller and a simple sensor-health checker.
// Header-only on purpose, so it is easy to read and to unit test.
#include <algorithm>
#include <cmath>
#include "sample.hpp"

// ---------------------------------------------------------------------------
// PID: output = Kp*error + Ki*integral(error) + Kd*(rate of change)
// Output is limited to 0..100 (% heater). The integral is limited too
// ("anti-windup") so it cannot grow while the heater is already at 100 %.
class PID {
public:
    PID(double kp, double ki, double kd) : kp_(kp), ki_(ki), kd_(kd) {}

    double update(double setpoint, double measured, double dt) {
        double error = setpoint - measured;
        integral_ += ki_ * error * dt;
        integral_ = std::clamp(integral_, 0.0, 100.0);          // anti-windup
        double derivative = first_ ? 0.0 : -(measured - prev_) / dt;
        prev_ = measured;
        first_ = false;
        return std::clamp(kp_ * error + integral_ + kd_ * derivative, 0.0, 100.0);
    }
    void reset() { integral_ = 0; first_ = true; }

private:
    double kp_, ki_, kd_;
    double integral_ = 0, prev_ = 0;
    bool first_ = true;
};

// ---------------------------------------------------------------------------
// Detects bad sensor data:
//   OutOfRange : impossible value (< -40 C or > 125 C)
//   Spike      : jumps more than 5 C between two samples (plant cannot do that)
//   Stuck      : exactly the same value 4 times in a row (frozen sensor)
class HealthChecker {
public:
    Anomaly check(double temp) {
        Anomaly result = Anomaly::None;

        if (temp < -40.0 || temp > 125.0)                      result = Anomaly::OutOfRange;
        else if (haveGood_ && std::fabs(temp - lastGood_) > 5.0) result = Anomaly::Spike;

        sameCount_ = (haveRaw_ && temp == lastRaw_) ? sameCount_ + 1 : 0;
        if (sameCount_ >= 4) result = Anomaly::Stuck;

        lastRaw_ = temp;  haveRaw_ = true;
        if (result == Anomaly::None) { lastGood_ = temp; haveGood_ = true; }
        return result;
    }
    bool   haveGood() const { return haveGood_; }
    double lastGood() const { return lastGood_; }

private:
    double lastGood_ = 0, lastRaw_ = 0;
    bool haveGood_ = false, haveRaw_ = false;
    int sameCount_ = 0;
};
