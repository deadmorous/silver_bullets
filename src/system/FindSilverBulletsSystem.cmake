find_library (SILVER_BULLETS_SYSTEM_LIBRARIES NAMES libsb_os.so sb_os.dll PATHS "${CMAKE_CURRENT_LIST_DIR}/../lib")
find_path(SILVER_BULLETS_SYSTEM_INCLUDE_DIRS "silver_bullets/system/defs.h" PATHS "${CMAKE_CURRENT_LIST_DIR}/../include")

