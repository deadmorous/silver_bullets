#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

#ifdef sb_factory_EXPORTS
#define SILVER_BULLETS_FACTORY_API __declspec(dllexport)
#else // sb_factory_EXPORTS
#define SILVER_BULLETS_FACTORY_API __declspec(dllimport)
#endif // sb_factory_EXPORTS
#else // defined(_WIN32) && defined(_MSC_VER)
#define SILVER_BULLETS_FACTORY_API
#endif // defined(_WIN32) && defined(_MSC_VER)
