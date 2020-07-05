cmake_minimum_required (VERSION 3.1.3) 

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

file(GLOB HEADER_FILES
    ${SILVER_BULLETS_ROOT_DIR}/include/${PROJECT_NAME}.hpp
    ${SILVER_BULLETS_ROOT_DIR}/include/${PROJECT_NAME}/*.hpp
    ${SILVER_BULLETS_ROOT_DIR}/include/${PROJECT_NAME}/*.h
)

install(FILES ${HEADER_FILES} DESTINATION include/silver_bullets)

