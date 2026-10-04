#pragma once

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
    virtual bool setFault(unsigned mode) = 0;      
    virtual const char* name() const = 0;
};

std::unique_ptr<ISensorSource> makeDriverSource(const std::string& path);
std::unique_ptr<ISensorSource> makeSimSource(unsigned seed = 1234);
