#pragma once

#if (defined(__GNUC__) && (__GNUC_MINOR__ <= 7)) \
    || (defined(__clang__) && __clang_major__ <= 6)
#include <experimental/filesystem>
namespace std {
namespace filesystem {
using namespace std::experimental::filesystem;
}
}
// #define STD_FILESYSTEM_NAMESPACE std::experimental::filesystem
#else
#include <filesystem>
// #define STD_FILESYSTEM_NAMESPACE std::filesystem
#endif

