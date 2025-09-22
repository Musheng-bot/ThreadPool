#include "ThreadPool.h"

using namespace std;

size_t ThreadPool::id_count_ = 0;

ThreadPool::ThreadPool() = default;

ThreadPool::ThreadPool(const std::size_t size) : init_thread_size_(size)
{
}

ThreadPool::~ThreadPool()
{
	is_running_ = false;
	not_empty_.notify_all();
	for (const auto &pair : threads_)
	{
		if (pair.second->joinable())
		{
			pair.second->join();
		}
	}
}

void ThreadPool::setPoolMode(PoolMode mode)
{
	pool_mode_ = mode;
}

bool ThreadPool::isRunning() const
{
	return is_running_;
}



void ThreadPool::start()
{
	is_running_ = true;
	if (pool_mode_ == PoolMode::CACHED) {
		init_thread_size_ = std::max<size_t>(init_thread_size_, tasks_.size());
		init_thread_size_ = std::min(init_thread_size_, max_thread_size_);
		empty_thread_size_ = init_thread_size_;
	}
	for (size_t i = 0; i < init_thread_size_; ++i) {
		auto th = std::make_unique<thread>([this]() { threadFunc(id_count_); });
		th->detach();
		threads_.emplace(id_count_, std::move(th));
		++thread_size_;
		++id_count_;
	}
}



[[noreturn]] void ThreadPool::threadFunc(const std::size_t thread_id)
{
	for (;;) {
		function<void()> task;
		if (pool_mode_ == PoolMode::FIXED) {
			std::unique_lock<mutex> lock(task_mtx_);
			not_empty_.wait(lock,[this](){
				return !this->tasks_.empty() || !this->isRunning();
			});
			if (!tasks_.empty())
			{
				task = std::move(tasks_.front());
				tasks_.pop();
				--empty_thread_size_;
			}
		}
		else {
			std::unique_lock<mutex> lock(task_mtx_);
			while (this->tasks_.empty()) {
				const cv_status ret = not_empty_.wait_for(lock, chrono::seconds(1));
				if (ret == cv_status::timeout && thread_size_ > init_thread_size_)
				{
					threads_.erase(thread_id);
					--thread_size_;
					--empty_thread_size_;
					break;
				}
			}
			task = std::move(tasks_.front());
			tasks_.pop();
			--empty_thread_size_;
		}
		task();
		++empty_thread_size_;
	}
}


