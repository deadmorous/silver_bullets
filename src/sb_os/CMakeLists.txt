cmake_minimum_required (VERSION 3.1.3) 

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

file(GLOB HEADER_FILES
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
)

aux_source_directory(. SOURCE_FILES)
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
  COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>
)


install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/../../include/${PROJECT_NAME} DESTINATION include)
install(FILES ${PROJECT_SOURCE_DIR}/../../cmake/FindSb_os.cmake DESTINATION cmake)

