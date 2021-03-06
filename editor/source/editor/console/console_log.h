#pragma once

#include "core/logging/logging.h"
#include <core/console/console.h>
#include <string>
#include <deque>
class ConsoleLog : public logging::sinks::base_sink<std::mutex>, public Console
{
public:
	using entries_t = std::deque<std::pair<std::string, logging::level::level_enum>>;

	//-----------------------------------------------------------------------------
	//  Name : _sink_it ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	void _sink_it(const logging::details::log_msg& msg) override;

	//-----------------------------------------------------------------------------
	//  Name : flush ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	void flush() override;

	//-----------------------------------------------------------------------------
	//  Name : get_items ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	entries_t get_items();

	//-----------------------------------------------------------------------------
	//  Name : clearLog ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	void clearLog();
	
	//-----------------------------------------------------------------------------
	//  Name : get_pending_entries ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	inline int get_pending_entries() const { return _pending; }

	//-----------------------------------------------------------------------------
	//  Name : set_pending_entries ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	inline void set_pending_entries(bool val) { _pending = val; }
private:
	///
	entries_t _entries;
	///
	int _pending = 0;
	///
	static const std::size_t _max_size = 50;
};