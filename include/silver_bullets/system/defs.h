#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

#ifdef sb_system_EXPORTS
#define SILVER_BULLETS_SYSTEM_API __declspec(dllexport)
#else // sb_system_EXPORTS
#define SILVER_BULLETS_SYSTEM_API __declspec(dllimport)
#endif // sb_system_EXPORTS
#else // defined(_WIN32) && defined(_MSC_VER)
#define SILVER_BULLETS_SYSTEM_API
#endif // defined(_WIN32) && defined(_MSC_VER)
