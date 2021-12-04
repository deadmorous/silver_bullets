#pragma once

#include <iostream>

namespace silver_bullets::iterate_struct {

struct ValidatorState
{
    ValidatorState() :
        errorStream(&std::cout)
    {}

    explicit ValidatorState(std::ostream* errorStream) :
        errorStream(errorStream)
    {}

    std::ostream* errorStream;
    bool hasErrors = false;
    bool hasWarnings = false;
};

} // namespace silver_bullets::iterate_struct
