cmake_minimum_required(VERSION 3.5)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED program_options)

aux_source_directory(. SOURCE_FILES)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} stdc++fs ${Boost_LIBRARIES})

set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
  COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>
)

file(COPY config_example.json DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
