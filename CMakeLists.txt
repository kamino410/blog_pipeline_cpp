cmake_minimum_required(VERSION 3.1)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(pipeline_test CXX)

add_executable(naive naive.cpp)
target_link_libraries(naive PRIVATE Threads::Threads)

add_executable(double_buffering double_buffering.cpp)
target_link_libraries(double_buffering PRIVATE Threads::Threads)

add_executable(thread_pool thread_pool.cpp)
target_link_libraries(thread_pool PRIVATE Threads::Threads pthread)

