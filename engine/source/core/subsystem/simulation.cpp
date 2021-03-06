#include "simulation.h"
#include <algorithm>
#include <thread>

namespace core
{
	bool Simulation::initialize()
	{
		_timestep = duration::zero();
		_last_frame_timepoint = clock::now();
		_launch_timepoint = clock::now();

		if (_max_inactive_fps == 0)
			_max_inactive_fps = std::max(_max_inactive_fps, _max_fps);

		return true;
	}

	void Simulation::dispose()
	{

	}

	void Simulation::run_one_frame()
	{
		// perform waiting loop if maximum fps set
		auto max_fps = _max_fps;
		if (max_fps > 0)
			max_fps = std::min(_max_inactive_fps, max_fps);

		if (max_fps > 0)
		{
			duration target_duration = std::chrono::milliseconds(1000) / max_fps;
			duration eplased = clock::now() - _last_frame_timepoint;
			for (;; )
			{
				eplased = clock::now() - _last_frame_timepoint;
				if (eplased > target_duration)
					break;

				if (target_duration - eplased > duration(1))
					std::this_thread::sleep_for((target_duration - eplased) - duration(1));
			}
		}

		duration eplased = clock::now() - _last_frame_timepoint;
		_last_frame_timepoint = clock::now();

		// if fps lower than minimum, clamp eplased time
		if (_min_fps > 0)
		{
			duration target_duration = std::chrono::milliseconds(1000) / _min_fps;
			if (eplased > target_duration)
				eplased = target_duration;
		}

		// perform time step smoothing
		if (_smoothing_step > 0)
		{
			_timestep = duration::zero();
			_previous_timesteps.push_back(eplased);
			if (_previous_timesteps.size() > _smoothing_step)
			{
				auto begin = _previous_timesteps.begin();
				_previous_timesteps.erase(begin, begin + _previous_timesteps.size() - _smoothing_step);
				for (auto step : _previous_timesteps)
					_timestep += step;
				_timestep /= _previous_timesteps.size();
			}
			else
				_timestep = _previous_timesteps.back();
		}
		else
		{
			_timestep = eplased;
		}

		++_frame;
	}
	
	void Simulation::set_min_fps(unsigned fps)
	{
		_min_fps = std::max(fps, (unsigned)0);
	}

	void Simulation::set_max_fps(unsigned fps)
	{
		_max_fps = std::max(fps, (unsigned)0);
	}

	void Simulation::set_max_inactive_fps(unsigned fps)
	{
		_max_inactive_fps = std::max(fps, (unsigned)0);
	}

	void Simulation::set_time_smoothing_step(unsigned step)
	{
		_smoothing_step = step;
	}

	Simulation::duration Simulation::get_time_since_launch() const
	{
		return clock::now() - _launch_timepoint;
	}

	unsigned Simulation::get_fps() const
	{
		auto dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(_timestep).count();
		return static_cast<unsigned>(dt == 0.0f ? 0 : 1000.0f / dt);
	}

	std::chrono::duration<float> Simulation::get_delta_time() const
	{
		auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(_timestep);
		return dt;
	}
}