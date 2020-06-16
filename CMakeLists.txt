cmake_minimum_required(VERSION 2.8)

project(vrfs)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED)
find_library(FUSE3_LIBRARY fuse3 REQUIRED)

set(VRFS_SOURCE
    src/file_ops.h
    src/file_ops.cc
)

add_library(vrfs_lib STATIC ${VRFS_SOURCE})
target_include_directories(vrfs_lib PUBLIC ${FUSE3_INCLUDE_DIR})
target_link_libraries(vrfs_lib Threads::Threads ${FUSE3_LIBRARY})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vrfs_lib PUBLIC -Wall -Wextra -Werror -pedantic)
    target_compile_options(vrfs_lib PRIVATE -O3)
endif()

add_executable(vrfs src/main.cc)
target_link_libraries(vrfs vrfs_lib)
add_executable(vrfs_benchmark src/benchmark.cc)
target_link_libraries(vrfs_benchmark vrfs_lib)
add_executable(vrfs_verify src/verify.cc)
target_link_libraries(vrfs_verify vrfs_lib)
