#pragma once

#include "EmptyValidator.hpp"
#include "StructFieldsMismatchValidator.hpp"
#include "CompositeValidator.hpp"
#include "StructDataValidator.hpp"

namespace silver_bullets::iterate_struct {

// using DefaultValidator = EmptyValidator;
template<class... MoreComponents>
using DefaultValidator =
    CompositeValidator<StructFieldsMismatchValidator,
                       StructDataValidator,
                       MoreComponents...>;

} // namespace silver_bullets::iterate_struct
