cmake_minimum_required(VERSION 3.22.1)
project(ThreadPool VERSION 1.0)
set(CXX_STANDARD 11)

add_library(
    ThreadPool SHARED
ThreadPool.h
ThreadPool.cpp)
