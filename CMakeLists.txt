cmake_minimum_required(VERSION 2.8)

project(vrfs)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_path(FUSE3_INCLUDE_DIR fuse3/fuse.h REQUIRED)
find_library(FUSE3_LIBRARY fuse3 REQUIRED)

add_executable(vrfs src/main.cc)
target_include_directories(vrfs PUBLIC ${FUSE3_INCLUDE_DIR})
target_link_libraries(vrfs Threads::Threads ${FUSE3_LIBRARY})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vrfs PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

