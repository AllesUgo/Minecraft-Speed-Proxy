#pragma once
#include <queue>
#include <list>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
namespace RbsLib::Thread
{
	class TaskPool
	{
	private:
		std::atomic_uint64_t  keep_task_num;
		std::atomic_uint64_t running_task_num = 0, watting_task_num = 0;
		std::queue<std::function<void()>> task_list;
		std::condition_variable getter;
		std::mutex task_lock;
		std::list<std::thread*> threads_handle;
		std::atomic_bool is_cancel = false;
	public:
		TaskPool(int num = 0)noexcept;/*default is no keep thread*/
		TaskPool(const TaskPool&) = delete;
		void Run(std::function<void()> func);
		~TaskPool(void);
	};
}