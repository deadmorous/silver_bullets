find_library (SILVER_BULLETS_FACTORY_LIBRARIES NAMES libsb_factory.so sb_factory.dll PATHS "${CMAKE_CURRENT_LIST_DIR}/../lib")
find_path(SILVER_BULLETS_FACTORY_INCLUDE_DIRS "silver_bullets/factory/Factory.hpp" PATHS "${CMAKE_CURRENT_LIST_DIR}/../include")

