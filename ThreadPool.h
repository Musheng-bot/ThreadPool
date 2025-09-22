#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <unordered_map>
#include <atomic>


enum class PoolMode : std::uint8_t{
	FIXED,
	CACHED
};

class ThreadPool{
	public:
		ThreadPool();
		explicit ThreadPool(std::size_t);
		~ThreadPool();
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		template<class F, typename ...Args>
		auto submitTask(F&& func, Args&& ... args) -> std::future<std::result_of_t<F(Args...)>>;

		void setPoolMode(PoolMode mode);
		bool isRunning() const;

		void start();


	private:
		std::unordered_map<std::size_t, std::unique_ptr<std::thread>> threads_;
		std::queue<std::function<void()>> tasks_;

		const std::size_t max_thread_size_ = std::thread::hardware_concurrency();
		std::size_t init_thread_size_ = 4;
		std::size_t thread_size_ = 0;
		std::atomic<std::size_t> empty_thread_size_{ init_thread_size_ };

		static std::size_t id_count_;

		PoolMode pool_mode_ = PoolMode::FIXED;
		std::atomic<bool> is_running_{false};

		std::mutex task_mtx_;
		std::condition_variable not_empty_;

		[[noreturn]] void threadFunc(std::size_t thread_id);
};

template <class F, typename ... Args>
auto ThreadPool::submitTask(F&& func, Args&&... args) -> std::future<std::result_of_t<F(Args...)>>
{
	using return_type = std::result_of_t<F(Args...)>;
	
	auto new_func = std::bind(std::forward<F>(func), std::forward<Args>(args)...);

	auto p_task = std::make_shared<std::packaged_task<return_type()>>(new_func);
	{
		std::unique_lock<std::mutex> lock(task_mtx_);
		tasks_.emplace( [p_task]() { (*p_task)(); } );
	}
	not_empty_.notify_one();
	std::future<return_type> res = p_task->get_future();

	if (pool_mode_ == PoolMode::CACHED && 
		is_running_ &&
		tasks_.size() > empty_thread_size_ && 
		thread_size_ < max_thread_size_  ) {

		auto th = std::make_unique<std::thread>([this]() { threadFunc(id_count_); });
		th->detach();
		threads_.emplace(id_count_, std::move(th));
		++thread_size_;
		++empty_thread_size_;
		++id_count_;
	}


	return std::move(res);
}
#endif
