cmake_minimum_required (VERSION 3.1.3) 

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/../../include/${PROJECT_NAME} DESTINATION include)

