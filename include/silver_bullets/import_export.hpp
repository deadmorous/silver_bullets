#pragma once

#if defined(_WIN32) && defined(_MSC_VER)
#define SILVER_BULLETS_EXPORTED_SYMBOL __declspec(dllexport)
#define SILVER_BULLETS_IMPORTED_SYMBOL __declspec(dllimport)
#else // defined(_WIN32) && defined(_MSC_VER)
#define SILVER_BULLETS_EXPORTED_SYMBOL __attribute__ ((visibility ("default")))
#define SILVER_BULLETS_IMPORTED_SYMBOL
#endif // defined(_WIN32) && defined(_MSC_VER)
