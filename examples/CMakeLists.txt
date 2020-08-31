cmake_minimum_required(VERSION 3.5)

add_subdirectory(use_iterate_struct)
add_subdirectory(use_templatize)
add_subdirectory(use_factory)
add_subdirectory(use_trie)
add_subdirectory(use_fs_ns_workaround)
add_subdirectory(use_system)
add_subdirectory(use_task_engine)
if (BUILD_REMOTE_EXECUTOR)
    add_subdirectory(use_task_engine_remote)
endif()
add_subdirectory(use_func)

