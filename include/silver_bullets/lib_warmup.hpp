#pragma once

#include "./import_export.hpp"

#define SILVER_BULLETS_DEFINE_LIB_WARMUP_FUNC(libName) \
    extern "C" SILVER_BULLETS_EXPORTED_SYMBOL void libName##_warmup() {}

#define SILVER_BULLETS_IMPORT_LIB_WARMUP_FUNC(libName) \
    extern "C" SILVER_BULLETS_IMPORTED_SYMBOL void libName##_warmup();

#define SILVER_BULLETS_CALL_LIB_WARMUP_FUNC(libName) \
    libName##_warmup();
