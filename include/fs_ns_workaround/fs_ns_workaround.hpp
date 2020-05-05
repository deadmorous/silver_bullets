#pragma once

#if defined(__GNUC__) && (__GNUC_MINOR__ <= 7)
#include <experimental/filesystem>
#define STD_FILESYSTEM_NAMESPACE std::experimental::filesystem
#else
#include <filesystem>
#define STD_FILESYSTEM_NAMESPACE std::filesystem
#endif

