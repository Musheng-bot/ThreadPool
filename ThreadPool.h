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

// 线程池模式
enum class PoolMode : std::uint8_t{
	FIXED,	// 固定线程数
	CACHED	// 动态调整线程数
};

class ThreadPool{
	public:
		using Task = std::function<void()>;
		using size_t = std::size_t;
		using atomic_size_t = std::atomic<size_t>;
		/**
		 *
		 * @param size 初始线程数，默认为 4
		 */
		explicit ThreadPool(std::size_t size = 4);
		~ThreadPool();


		/**
		 * 四个构造函数，禁止了移动和拷贝
		 */
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;


		/**
		 *
		 * @tparam F 函数类型
		 * @tparam Args 接收参数类型
		 * @param func 函数对象本身，推荐使用std::function封装
		 * @param args 运行需要接收的参数
		 * @return 一个future对象，在得到结果后会返回
		 */
		template<class F, typename ...Args>
		auto submitTask(F&& func, Args&& ... args) -> std::future<std::result_of_t<F(Args...)>>;

		void setPoolMode(PoolMode mode);
		bool isRunning() const;

		void start();


	private:

		/**
		 * threads_: 使用哈希表记录线程，一个id对应一个线程
		 * tasks_: 储存任务队列
		 * 操作时需注意多线程安全，均需加锁
		 */
		std::unordered_map<std::size_t, std::unique_ptr<std::thread>> threads_;
		std::queue<std::function<void()>> tasks_;


		/**
		 * max_thread_size_ 最大线程数，取决于你的电脑配置
		 * init_thread_size_ 默认为4, 由用户指定，不会超过最大线程数
		 * thread_size_ 实时记录当前拥有的线程数
		 * empty_thread_size_ 记录当前的空闲线程数
		 * 非常量的参数是有多线程竞争问题的，故而需要使用原子操作
		 */
		const size_t max_thread_size_ = std::thread::hardware_concurrency();
		const size_t init_thread_size_;
		atomic_size_t thread_size_ = 0;
		atomic_size_t empty_thread_size_{ init_thread_size_ };

		/**
		 * pool_mode_: 线程池模式
		 * is_running_: 线程池是否正在运行
		 */
		PoolMode pool_mode_ = PoolMode::FIXED;
		std::atomic<bool> is_running_{false};


		/**
		 * task_mtx_: 任务队列锁，控制任务队列的线程安全
		 * not_empty: 任务对列非空的条件变量，用于及时通知线程认领任务
		 */
		std::mutex task_mtx_;
		std::condition_variable not_empty_;

		/**
		 *
		 * @param thread_id 线程ID，是我们自己分配的
		 */
		void threadFunc(std::size_t thread_id);

		/**
		 *
		 * @return 线程ID，是我们自己分配的
		 */
		static size_t getThreadId();
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
	std::future<return_type> res = p_task->get_future();

	if (pool_mode_ == PoolMode::CACHED && 
		is_running_ &&
		tasks_.size() > empty_thread_size_ && 
		thread_size_ < max_thread_size_  ) {

		auto id = getThreadId();
		auto th = std::make_unique<std::thread>([this, id]() { threadFunc(id); });
		th->detach();
		threads_.emplace(id, std::move(th));
		++thread_size_;
		++empty_thread_size_;
	}
	else {
		//如果说不是动态调整模式下需要创建新线程处理，就直接让一个空闲进程来处理
		not_empty_.notify_one();
	}


	return std::move(res);
}
#endif
