# Generate c++ code from *.proto files
# PROTOFILE - base name for proto
# SRC_DIR - proto sources directory
# DST_DIR - generated files destionation
function(generateProtoApi PROTOFILE SRC_DIR DST_DIR PROTO_SRC)
    message (STATUS "Generate cpp files from ${PROTOFILE}.proto")
    message (STATUS "SRC_DIR: ${SRC_DIR}")
    message (STATUS "DST_DIR: ${DST_DIR}")

    set(GENERATED_FILES
        ${DST_DIR}/${PROTOFILE}.pb.cc ${DST_DIR}/${PROTOFILE}.pb.h
        ${DST_DIR}/${PROTOFILE}.grpc.pb.cc ${DST_DIR}/${PROTOFILE}.grpc.pb.h)

    if (NOT TARGET GENERATE_PROTO_FILES)
        # Define custom target for proto generation
        add_custom_target(GENERATE_PROTO_FILES ALL)
    endif()

    add_custom_command(
        TARGET GENERATE_PROTO_FILES
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}
        COMMAND LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${EXTERNAL_LIB}/lib/x86_64-linux-gnu ${EXTERNAL_LIB}/bin/protoc -I=${SRC_DIR} --cpp_out=${DST_DIR} ${SRC_DIR}/${PROTOFILE}.proto
        COMMAND LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${EXTERNAL_LIB}/lib/x86_64-linux-gnu ${EXTERNAL_LIB}/bin/protoc -I=${SRC_DIR} --grpc_out=${DST_DIR} --plugin=protoc-gen-grpc=${EXTERNAL_LIB}/bin/grpc_cpp_plugin ${SRC_DIR}/${PROTOFILE}.proto)

    set_source_files_properties(
        ${GENERATED_FILES} PROPERTIES GENERATED TRUE)

    set(PROTO_SRC ${GENERATED_FILES} PARENT_SCOPE)
endfunction()
