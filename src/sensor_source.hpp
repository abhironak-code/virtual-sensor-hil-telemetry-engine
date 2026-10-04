#pragma once
// Abstraction of "where do sensor frames come from?".
//   DriverSource : real Linux character device  (/dev/vsensor0, kernel module)
//   SimSource    : pure-software twin of the driver (fallback, CI, unit tests)
// Both expose the same interface, so the engine never knows the difference.
#include <memory>
#include <string>
#include "sample.hpp"

enum class ReadStatus { Ok, Dropout, Fatal };

class ISensorSource {
public:
    virtual ~ISensorSource() = default;
    virtual bool open() = 0;
    virtual ReadStatus read(Sample& out) = 0;
    virtual bool setHeater(unsigned pct) = 0;      // actuator
    virtual bool setFault(unsigned mode) = 0;      // 0 none,1 stuck,2 spike,3 dropout
    virtual const char* name() const = 0;
};

std::unique_ptr<ISensorSource> makeDriverSource(const std::string& path);
std::unique_ptr<ISensorSource> makeSimSource(unsigned seed = 1234);
