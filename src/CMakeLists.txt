cmake_minimum_required(VERSION 3.5)

add_subdirectory(enum_names)
add_subdirectory(factory)
add_subdirectory(iterate_struct)
add_subdirectory(templatize)
add_subdirectory(trie)
add_subdirectory(fs_ns_workaround)
add_subdirectory(system)
add_subdirectory(sync)
add_subdirectory(task_engine)
add_subdirectory(func)

install(FILES
    ${SILVER_BULLETS_INCLUDE_DIR}/import_export.hpp
    ${SILVER_BULLETS_INCLUDE_DIR}/lib_warmup.hpp
    DESTINATION include/silver_bullets/)
