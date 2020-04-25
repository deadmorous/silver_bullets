cmake_minimum_required(VERSION 3.5)

project(silver_bullets)

find_package(Boost)

include_directories (${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external/rapidjson/include)

add_subdirectory(examples)

