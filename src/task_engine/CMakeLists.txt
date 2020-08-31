cmake_minimum_required (VERSION 3.1.3)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB TOPLEVEL_HEADER_FILES
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}.hpp
)

file(GLOB HEADER_FILES
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}/*.hpp
    ${SILVER_BULLETS_INCLUDE_DIR}/${PROJECT_NAME}/*.h
)

if (BUILD_REMOTE_EXECUTOR)
    include(${CMAKE_CURRENT_SOURCE_DIR}/protoapi/proto.cmake)
    set(EXTERNAL_LIB ${CMAKE_SOURCE_DIR}/../redist/usr)
    file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp )
    generateProtoApi(task ${CMAKE_CURRENT_SOURCE_DIR}/protoapi ${CMAKE_CURRENT_BINARY_DIR}/proto PROTO_SRC)
    file(GLOB LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

    add_library(${PROJECT_NAME} SHARED ${SRC} ${PROTO_SRC} ${HEADER_FILES} ${LIB_HEADERS})

    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_LIB}/include)
    target_link_directories(${PROJECT_NAME} PUBLIC  ${EXTERNAL_LIB}/lib/x86_64-linux-gnu/)
    target_link_libraries(${PROJECT_NAME} pthread absl_strings.so.20200225 grpc grpc++ protobuf)
    add_dependencies(${PROJECT_NAME} GENERATE_PROTO_FILES)
endif()


install(FILES ${TOPLEVEL_HEADER_FILES} DESTINATION include/silver_bullets)
install(FILES ${HEADER_FILES} DESTINATION include/silver_bullets/${PROJECT_NAME})
install(FILES ${LIB_HEADERS} DESTINATION include/silver_bullets/${PROJECT_NAME})
