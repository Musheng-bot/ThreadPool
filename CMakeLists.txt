cmake_minimum_required(VERSION 3.22.1)
project(ThreadPool VERSION 2.0 LANGUAGES CXX)

# 设置C++标准并强制要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展，确保标准一致性

# 定义线程池库
add_library(ThreadPool SHARED
		ThreadPool.h
		ThreadPool.cpp
)

#add_executable(main main.cpp ThreadPool.cpp)
