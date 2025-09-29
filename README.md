# ThreadPool

## 简单介绍

一个能实现固定线程数量或者是动态调整线程数量的线程池，实现简单，便于添加。

## 功能特点

- 支持两种线程池模式：
	- `FIXED`：固定线程数量，初始化后线程数量保持不变
	- `CACHED`：动态调整线程数量，根据任务数量自动增减线程（不超过最大线程数）
- 线程安全的任务提交机制
- 支持获取任务执行结果（通过`std::future`）
- 自动管理线程生命周期，析构时优雅退出

## 实现原理

1. 核心组件：
	- 线程容器：存储工作线程（`std::unordered_map`）
	- 任务队列：存储待执行任务（`std::queue`）
	- 同步机制：互斥锁（`std::mutex`）和条件变量（`std::condition_variable`）

2. 工作流程：
	- 初始化线程池并设置运行模式
	- 调用`start()`方法启动线程池
	- 通过`submitTask()`提交任务到任务队列
	- 工作线程从任务队列中获取任务并执行
	- 线程池析构时自动停止所有线程并等待任务完成

## 使用示例

```cpp
#include "ThreadPool.h"
#include <iostream>
#include <chrono>

int main() {
    // 创建线程池（默认模式为FIXED，初始线程数4）
    ThreadPool pool;
    
    // 也可以指定初始线程数
    // ThreadPool pool(8);
    
    // 设置为CACHED模式（可选）
    // pool.setPoolMode(PoolMode::CACHED);
    
    // 启动线程池
    pool.start();
    
    // 提交任务
    auto future1 = pool.submitTask([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 42;
    });
    
    auto future2 = pool.submitTask([](int a, int b) {
        return a + b;
    }, 10, 20);
    
    // 获取任务结果
    std::cout << "Result 1: " << future1.get() << std::endl;  // 输出42
    std::cout << "Result 2: " << future2.get() << std::endl;  // 输出30
    
    // 线程池会在析构时自动停止
    return 0;
}
```

## 接口说明

1. 构造函数：
	- `ThreadPool()`：默认构造，使用默认初始线程数（4）
	- `explicit ThreadPool(std::size_t size)`：指定初始线程数

2. 核心方法：
	- `void setPoolMode(PoolMode mode)`：设置线程池模式（`FIXED`或`CACHED`）
	- `void start()`：启动线程池
	- `template<class F, typename ...Args> auto submitTask(F&& func, Args&& ... args) -> std::future<std::result_of_t<F(Args...)>>`：提交任务并返回`future`对象用于获取结果
	- `bool isRunning() const`：判断线程池是否在运行

## 注意事项

- 线程池析构时会自动等待所有线程完成当前任务
- `CACHED`模式下最大线程数为系统硬件支持的并发线程数（`std::thread::hardware_concurrency()`）
- 提交的任务会被封装为`std::function`，支持任意可调用对象（函数、lambda、绑定表达式等）
- 任务执行异常会通过`std::future`传递，调用`get()`时会抛出
- 线程池为`CACHED`模式时不会强制`join`，而是会让线程`detach`。

## 编译说明

项目使用CMake进行构建，最低支持CMake 3.22.1版本，C++标准为C++17。

编译步骤：
```bash
mkdir build
cd build
cmake ..
make
```