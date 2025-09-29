#include "ThreadPool.h"
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

ThreadPool::ThreadPool(const std::size_t size) :
	init_thread_size_(std::min(size, max_thread_size_))
{
}

ThreadPool::~ThreadPool(){
	//一定要终止
	is_running_ = false;
	not_empty_.notify_all();
	for (const auto &[id, th] : threads_) {
		if (th->joinable()) {
			th->join();
		}
	}
}

void ThreadPool::setPoolMode(const PoolMode mode) {
	pool_mode_ = mode;
}

bool ThreadPool::isRunning() const {
	return is_running_;
}



void ThreadPool::start() {
	is_running_.store(true);
	if (pool_mode_ == PoolMode::CACHED) {
		empty_thread_size_ = std::min(std::max<size_t>(init_thread_size_, tasks_.size()), max_thread_size_);
		thread_size_.store(empty_thread_size_);
	}
	else {
		thread_size_ = init_thread_size_;
	}
	for (size_t i = 0; i < thread_size_.load(); ++i) {
		auto id = getThreadId();
		auto th = std::make_unique<std::thread>([this, id]() { threadFunc(id); });
		if (pool_mode_ == PoolMode::CACHED) {
			th->detach();
		}
		threads_.emplace(id, std::move(th));
	}
}



void ThreadPool::threadFunc(const std::size_t thread_id) {
	while (is_running_) {
		Task task = [](){};
		if (pool_mode_ == PoolMode::FIXED) {
			std::unique_lock<std::mutex> lock(task_mtx_);
			not_empty_.wait(lock,[this](){
				return !this->tasks_.empty() || !this->isRunning();
			});
			if (!tasks_.empty()) {
				task = std::move(tasks_.front());
				tasks_.pop();
				--empty_thread_size_;
			}
		}
		else {
			std::unique_lock<std::mutex> lock(task_mtx_);
			const std::cv_status ret = not_empty_.wait_for(lock, 1s);
			if (ret == std::cv_status::timeout && thread_size_ > init_thread_size_){
				threads_.erase(thread_id);
				--thread_size_;
				--empty_thread_size_;
				break;
			}
			if (!tasks_.empty()) {
				task = std::move(tasks_.front());
				tasks_.pop();
				--empty_thread_size_;
			}
		}
		try {
			task();
		}
		catch (const std::exception &err) {
			std::cout << "Something failed while executing task: " << err.what() << std::endl;
		}
		++empty_thread_size_;
		std::this_thread::sleep_for(10ms);
	}
}

std::size_t ThreadPool::getThreadId() {
	static std::size_t thread_id = 0;
	return ++thread_id; //thread_id的修改只出现在submitTask函数中和第一次启动时，所以不用考虑线程安全
}


