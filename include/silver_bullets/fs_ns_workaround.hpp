#pragma once

#if (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ <= 7)) \
    || (defined(__clang__) && __clang_major__ <= 6)
#include <experimental/filesystem>
namespace std {
namespace filesystem {
using namespace std::experimental::filesystem;
}
}
#else
#include <filesystem>
#endif

