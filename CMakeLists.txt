cmake_minimum_required(VERSION 2.8)

project(vrfs)
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only support 64bit platforms")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(WIN32)
    find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED PATHS "C:/Program Files (x86)/WinFsp/inc")
    find_file(FUSE3_LIBRARY winfsp-x64.dll REQUIRED PATHS "C:/Program Files (x86)/WinFsp/bin")
    find_library(FUSE3_IMPORT_LIBRARY winfsp-x64 REQUIRED PATHS "C:/Program Files (x86)/WinFsp/lib")
else()
    find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED)
    find_library(FUSE3_LIBRARY fuse3 REQUIRED)
endif()

add_library(FUSE3 SHARED IMPORTED)
set_target_properties(FUSE3 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${FUSE3_INCLUDE_DIR})
set_target_properties(FUSE3 PROPERTIES IMPORTED_LOCATION ${FUSE3_LIBRARY})
if(WIN32)
    set_target_properties(FUSE3 PROPERTIES IMPORTED_IMPLIB ${FUSE3_IMPORT_LIBRARY})
endif()

set(VRFS_SOURCE
    src/file_ops.h
    src/file_ops.cc
)

add_library(vrfs_lib STATIC ${VRFS_SOURCE})
target_link_libraries(vrfs_lib Threads::Threads FUSE3)

if(WIN32)
    target_compile_definitions(vrfs_lib PUBLIC NOMINMAX)
    add_custom_command(TARGET vrfs_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FUSE3_LIBRARY}" $<TARGET_FILE_DIR:vrfs_lib>)
endif()

if(MSVC)
    target_compile_options(vrfs_lib PUBLIC /W4 /WX /MP)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vrfs_lib PUBLIC -Wall -Wextra -Werror -pedantic)
    target_compile_options(vrfs_lib PRIVATE -O3)
endif()

add_executable(vrfs src/main.cc)
target_link_libraries(vrfs vrfs_lib)
add_executable(vrfs_benchmark src/benchmark.cc)
target_link_libraries(vrfs_benchmark vrfs_lib)
add_executable(vrfs_verify src/verify.cc)
target_link_libraries(vrfs_verify vrfs_lib)
