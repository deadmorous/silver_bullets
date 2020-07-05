cmake_minimum_required (VERSION 3.1.3) 

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

file(GLOB TOPLEVEL_HEADER_FILES
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}.hpp
)

file(GLOB HEADER_FILES
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}/*.hpp
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}/*.h
)

aux_source_directory(. SOURCE_FILES)
add_library(sb_${PROJECT_NAME} ${SOURCE_FILES} ${TOPLEVEL_HEADER_FILES} ${HEADER_FILES})

set_property(TARGET sb_${PROJECT_NAME} APPEND PROPERTY
  COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>
)


install(TARGETS sb_${PROJECT_NAME} DESTINATION lib)
install(FILES ${TOPLEVEL_HEADER_FILES} DESTINATION include/silver_bullets)
install(FILES ${HEADER_FILES} DESTINATION include/silver_bullets/${PROJECT_NAME})
install(FILES ./FindSilverBulletsSystem.cmake DESTINATION cmake)
