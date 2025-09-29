cmake_minimum_required(VERSION 3.22.1)
project(ThreadPool VERSION 1.0)
set(CXX_STANDARD 14)

add_library(
    ThreadPool SHARED
ThreadPool.h
ThreadPool.cpp)

add_executable(main main.cpp)
target_link_libraries(main PUBLIC ThreadPool)
