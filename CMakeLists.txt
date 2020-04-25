cmake_minimum_required(VERSION 3.5)

project(silver_bullets)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set (BUILD_SHARED_LIBS true)

find_package(Boost)

include_directories (${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external/rapidjson/include)

add_subdirectory(src)
add_subdirectory(examples)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/external/rapidjson/include/rapidjson DESTINATION include)

