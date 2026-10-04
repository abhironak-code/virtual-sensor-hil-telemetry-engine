#include "sensor_source.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <random>
#include <sys/ioctl.h>
#include <unistd.h>

#include "vsensor_ioctl.h"

namespace {

uint64_t monotonicNs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}


class DriverSource final : public ISensorSource {
public:
    explicit DriverSource(std::string path) : path_(std::move(path)) {}
    ~DriverSource() override { if (fd_ >= 0) ::close(fd_); }

    bool open() override {
        fd_ = ::open(path_.c_str(), O_RDWR);
        if (fd_ < 0) return false;
        return ::ioctl(fd_, VS_IOC_RESET) == 0;
    }

    ReadStatus read(Sample& out) override {
        vs_sample s{};
        const ssize_t n = ::read(fd_, &s, sizeof(s));
        if (n == static_cast<ssize_t>(sizeof(s))) {
            out.ts_ns      = s.ts_ns;
            out.seq        = s.seq;
            out.temp_c     = s.temp_mc / 1000.0;
            out.hum_pct    = s.hum_mpct / 1000.0;
            out.press_pa   = s.press_pa;
            out.heater_pct = s.heater_pct;
            out.hw_flags   = s.flags;
            return ReadStatus::Ok;
        }
        if (n < 0 && errno == EIO) return ReadStatus::Dropout;
        return ReadStatus::Fatal;
    }

    bool setHeater(unsigned pct) override {
        __u32 v = pct;
        return ::ioctl(fd_, VS_IOC_SET_HEATER, &v) == 0;
    }
    bool setFault(unsigned mode) override {
        __u32 v = mode;
        return ::ioctl(fd_, VS_IOC_SET_FAULT, &v) == 0;
    }
    const char* name() const override { return "kernel-driver (/dev/vsensor0)"; }

private:
    std::string path_;
    int fd_ = -1;
};


class SimSource final : public ISensorSource {
public:
    explicit SimSource(unsigned seed) : rng_(seed), noise_(-0.15, 0.15) {}

    bool open() override { return true; }

    ReadStatus read(Sample& out) override {
        temp_ += heater_ * 0.010 - (temp_ - kAmbient) * 0.01;
        ++seq_;
        if (fault_ == 3) return ReadStatus::Dropout;

        double t = temp_ + noise_(rng_);
        out.hw_flags = 0;
        if (fault_ == 1) {
            if (!latched_) { stuck_ = t; latched_ = true; }
            t = stuck_;
            out.hw_flags = 1;
        } else {
            latched_ = false;
            if (fault_ == 2 && seq_ % 10 == 0) { t += 20.0; out.hw_flags = 1; }
        }
        const double tri = (seq_ % 200) < 100 ? (seq_ % 200) : 200 - (seq_ % 200);
        out.ts_ns      = monotonicNs();
        out.seq        = seq_;
        out.temp_c     = t;
        out.hum_pct    = std::fmax(0.0, std::fmin(100.0, 50.0 - (temp_ - kAmbient) / 2.0));
        out.press_pa   = 101325 + (tri - 50) * 3;
        out.heater_pct = heater_;
        return ReadStatus::Ok;
    }

    bool setHeater(unsigned pct) override {
        if (pct > 100) return false;
        heater_ = pct;
        return true;
    }
    bool setFault(unsigned mode) override {
        if (mode > 3) return false;
        fault_ = mode; latched_ = false;
        return true;
    }
    const char* name() const override { return "software-simulator"; }

private:
    static constexpr double kAmbient = 25.0;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> noise_;
    double temp_ = kAmbient, heater_ = 0, stuck_ = 0;
    unsigned seq_ = 0, fault_ = 0;
    bool latched_ = false;
};

} 

std::unique_ptr<ISensorSource> makeDriverSource(const std::string& path) {
    return std::make_unique<DriverSource>(path);
}
std::unique_ptr<ISensorSource> makeSimSource(unsigned seed) {
    return std::make_unique<SimSource>(seed);
}
