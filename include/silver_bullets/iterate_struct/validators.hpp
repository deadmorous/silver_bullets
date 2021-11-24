#pragma once

#include "empty_validator.hpp"
#include "struct_fields_mismatch_validator.hpp"

namespace silver_bullets::iterate_struct {

// using DefaultValidator = EmptyValidator;
using DefaultValidator = StructFieldsMismatchValidator;

} // namespace silver_bullets::iterate_struct
