find_library (SB_OS_LIBRARIES NAMES libsb_os.so sb_os.dll PATHS "${CMAKE_CURRENT_LIST_DIR}/../lib")
find_path(SB_OS_INCLUDE_DIRS "sb_os/defs.h" PATHS "${CMAKE_CURRENT_LIST_DIR}/../include")

