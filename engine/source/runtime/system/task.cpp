#include "task.h"
#include "engine.h"
#include "core/common/assert.hpp"

namespace runtime
{

	bool TaskSystem::initialize()
	{
		if (_core == 0)
			_core = std::thread::hardware_concurrency() - 1;

		_core = std::max(_core, (uint32_t)1);
		_stop = false;
		for (uint32_t i = 0; i < _core; ++i)
		{
			_workers.emplace_back(thread_run, std::ref(*this), i + 1);
			_thread_indices.insert(std::make_pair(_workers.back().get_id(), i + 1));
		}

		_thread_main = std::this_thread::get_id();
		_thread_indices.insert(std::make_pair(_thread_main, 0));
		
		on_frame_begin.connect(this, &TaskSystem::execute_tasks_on_main);

		return true;
	}

	void TaskSystem::dispose()
	{
		on_frame_begin.disconnect(this, &TaskSystem::execute_tasks_on_main);

		{
			std::unique_lock<std::mutex> lock(_other_thread_tasks.mutex);
			_stop = true;
		}

		_condition.notify_all();
		for (auto& thread : _workers)
			thread.join();
	}

	core::Handle TaskSystem::create_internal(const std::string& name, std::function<void()> closure)
	{
		core::Handle handle;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			handle = _tasks.create();
		}
		if (handle)
		{
			Task* task = nullptr;
			{
				std::unique_lock<std::mutex> L(_tasks_mutex);
				task = _tasks.fetch(handle);
			}
			if (task)
			{
				task->closure = closure;
				task->jobs.store(1);
				task->name = name;
			}

			return handle;
		}

		return core::Handle();
	}

	core::Handle TaskSystem::create_as_child_internal(core::Handle parent, const std::string& name, std::function<void()> closure)
	{
		core::Handle handle;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			handle = _tasks.create();
		}
		if (handle)
		{
			Task* task = nullptr;
			{
				std::unique_lock<std::mutex> L(_tasks_mutex);
				task = _tasks.fetch(handle);
			}
			if (task)
			{
				task->closure = closure;
				task->jobs.store(1);
				task->name = name;
			}
			Task* ptask = nullptr;
			{
				std::unique_lock<std::mutex> L(_tasks_mutex);
				ptask = _tasks.fetch(parent);
			}
			if (ptask != nullptr)
			{
				uint32_t current_jobs = ptask->jobs++;
				if (current_jobs > 0) task->parent = parent;
				else ptask->jobs--;
			}
			return handle;
		}
		return core::Handle();
	}

	void TaskSystem::run(core::Handle handle, bool on_main_thread)
	{
		Task* task = nullptr;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			task = _tasks.fetch(handle);
		}
		Expects(task != nullptr && task->jobs.load() > 0);

		if (on_main_thread)
		{
			{
				std::unique_lock<std::mutex> L(_main_thread_tasks.mutex);
				_main_thread_tasks.tasks.push_back(handle);
			}
			unsigned index = get_thread_index();
			if (index == 0)
				execute_one(index, true, _main_thread_tasks.mutex, _main_thread_tasks.tasks);
		}
		else
		{
			{
				std::unique_lock<std::mutex> L(_other_thread_tasks.mutex);
				_other_thread_tasks.tasks.push_back(handle);
			}
			_condition.notify_one();
		}
		
	}

	void TaskSystem::execute_tasks_on_main(std::chrono::duration<float>)
	{
		unsigned index = get_thread_index();
		while (!_main_thread_tasks.tasks.empty())
		{
			execute_one(index, true, _main_thread_tasks.mutex, _main_thread_tasks.tasks);
		}
	}

	bool TaskSystem::is_completed(core::Handle handle)
	{
		Task* task = nullptr;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			task = _tasks.fetch(handle);
		}
		if (task == nullptr)
			return true;
		return task->jobs.load() == 0;
	}

	void TaskSystem::wait(core::Handle handle)
	{
		Task* task = nullptr;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			task = _tasks.fetch(handle);
		}
		if (task == nullptr)
			return;

		unsigned index = get_thread_index();
		while (!is_completed(handle))
		{		
			if(index != 0)
				std::this_thread::yield();
			
			execute_one(handle, index, false, _other_thread_tasks.mutex, _other_thread_tasks.tasks);
		}
	}

	void TaskSystem::finish(core::Handle handle)
	{
		Task* task = nullptr;
		{
			std::unique_lock<std::mutex> L(_tasks_mutex);
			task = _tasks.fetch(handle);
		}
		if (task == nullptr)
			return;

		// atomic decrement
		const uint32_t jobs = --task->jobs;
		if (jobs == 0)
		{
			finish(task->parent);

			// free captured reference and recycle task
			task->closure = nullptr;
			task->parent.invalidate(); // invalidate parent handle
			{
				std::unique_lock<std::mutex> L(_tasks_mutex);
				_tasks.free(handle);
			}
		}
	}

	bool TaskSystem::execute_one(unsigned index, bool wait, std::mutex& mtx, std::deque<core::Handle>& queue)
	{
		core::Handle handle;
		{
			std::unique_lock<std::mutex> L(mtx);
			if (wait)
				_condition.wait(L, [this, &queue] { return _stop || !queue.empty(); });

			if (_stop && queue.empty())
				return false;

			if (!wait && queue.empty())
				return true;

			handle = queue.front();
			queue.pop_front();
		}

		if (auto task = _tasks.fetch(handle))
		{
			if (on_task_start)
				on_task_start(index, task->name);

			if (task->closure)
				task->closure();

			finish(handle);

			if (on_task_stop)
				on_task_stop(index, task->name);
		}

		return true;
	}

	bool TaskSystem::execute_one(core::Handle handle, unsigned index, bool wait, std::mutex& mtx, std::deque<core::Handle>& queue)
	{
		{
			std::unique_lock<std::mutex> L(mtx);
			if (wait)
				_condition.wait(L, [this, &queue] { return _stop || !queue.empty(); });

			if (_stop && queue.empty())
				return false;

			if (!wait && queue.empty())
				return true;
			
			auto it = std::find(std::begin(queue), std::end(queue), handle);
			if (it != std::end(queue))
			{
				queue.erase(it);
			}
			else
			{
				return false;
			}
		}

		if (auto task = _tasks.fetch(handle))
		{
			if (on_task_start)
				on_task_start(index, task->name);

			if (task->closure)
				task->closure();

			finish(handle);

			if (on_task_stop)
				on_task_stop(index, task->name);
		}

		return true;
	}

	unsigned TaskSystem::get_thread_index() const
	{
		auto found = _thread_indices.find(std::this_thread::get_id());
		if (found != _thread_indices.end()) return found->second;
		return 0xFFFFFFFF;
	}

	void TaskSystem::thread_run(TaskSystem& scheduler, unsigned index)
	{
		if (scheduler.on_thread_start)
			scheduler.on_thread_start(index);

		for (;; )
		{
			if (!scheduler.execute_one(index, true, scheduler._other_thread_tasks.mutex, scheduler._other_thread_tasks.tasks))
				break;
		}

		if (scheduler.on_thread_stop)
			scheduler.on_thread_stop(index);
	}

}