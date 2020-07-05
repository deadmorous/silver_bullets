cmake_minimum_required(VERSION 3.5)

project(silver_bullets)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set (BUILD_SHARED_LIBS true)

set (SILVER_BULLETS_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set (SILVER_BULLETS_INCLUDE_DIR ${SILVER_BULLETS_ROOT_DIR}/include/silver_bullets)

find_package(Boost)

include_directories (${Boost_INCLUDE_DIRS})
include_directories(${SILVER_BULLETS_ROOT_DIR}/include)
include_directories(${SILVER_BULLETS_ROOT_DIR}/external/rapidjson/include)

add_subdirectory(src)
add_subdirectory(examples)

install(DIRECTORY ${SILVER_BULLETS_ROOT_DIR}/external/rapidjson/include/rapidjson DESTINATION include)
