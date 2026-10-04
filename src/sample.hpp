#pragma once
// Plain data types shared by every module of the engine.
#include <cstdint>

enum class Anomaly : uint8_t { None = 0, OutOfRange, Spike, Stuck, Interlock };

inline const char* toString(Anomaly a) {
    switch (a) {
        case Anomaly::None:       return "none";
        case Anomaly::OutOfRange: return "out_of_range";
        case Anomaly::Spike:      return "spike";
        case Anomaly::Stuck:      return "stuck";
        case Anomaly::Interlock:  return "overtemp_interlock";
    }
    return "?";
}

struct Sample {
    uint64_t ts_ns      = 0;
    uint32_t seq        = 0;
    double   temp_c     = 0;
    double   hum_pct    = 0;
    double   press_pa   = 0;
    double   heater_pct = 0;   // actuator command applied after this sample
    double   setpoint_c = 0;
    uint32_t hw_flags   = 0;
    Anomaly  anomaly    = Anomaly::None;
};
