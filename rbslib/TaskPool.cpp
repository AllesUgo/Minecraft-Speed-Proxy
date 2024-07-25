#include "TaskPool.h"

RbsLib::Thread::TaskPool::TaskPool(int num)noexcept :keep_task_num(num) {}

void RbsLib::Thread::TaskPool::Run(std::function<void()> func)
{
	std::unique_lock<std::mutex> lock(this->task_lock);
	this->task_list.push(func);
	lock.unlock();
	if (this->watting_task_num > 0)
	{
		this->getter.notify_one();
	}
	else
	{
		/*Need to start new thread*/
		auto& mutex = this->task_lock;
		auto& getter = this->getter;
		auto& running = this->running_task_num;
		auto& watting = this->watting_task_num;
		auto& iscancel = this->is_cancel;
		auto& max = this->keep_task_num;
		auto& task_list = this->task_list;
		auto& threads_list = this->threads_handle;

		lock.lock();
		std::thread* t = new std::thread([&max, &mutex, &getter, &running, &watting, &iscancel, &task_list, &threads_list]() {
			std::unique_lock<std::mutex> lock(mutex);
			watting += 1;
			while (iscancel == false)
			{
				getter.wait(lock, [&task_list, &iscancel]() {return (!task_list.empty()) || iscancel; });
				if (iscancel)
				{
					watting -= 1;
					break;
				}
				auto func = task_list.front();
				task_list.pop();
				watting -= 1;
				running += 1;
				lock.unlock();
				func();
				lock.lock();
				running -= 1;
				watting += 1;
				if (running + watting > max)
				{
					watting -= 1;
					break;
				}
			}
			if (!iscancel)
			{
				std::thread* ptr;
				for (auto it : threads_list)
				{
					if (it->get_id() == std::this_thread::get_id())
					{
						ptr = it;
					}
				}
				if (ptr->joinable() && !iscancel) ptr->detach();
				threads_list.remove(ptr);
				delete ptr;
			}
			});

		this->threads_handle.push_back(t);
		lock.unlock();
	}
}

RbsLib::Thread::TaskPool::~TaskPool(void)
{
	std::unique_lock<std::mutex> lock(this->task_lock);
	this->is_cancel = true;
	lock.unlock();
	this->getter.notify_all();
	for (auto it : this->threads_handle)
	{
		if (it->joinable())
		{
			it->join();
		}
		delete it;
	}
}

