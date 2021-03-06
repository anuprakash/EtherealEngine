#include "app.h"
#include "engine.h"
#include "rendering/render_window.h"

namespace runtime
{

	App::App() : _exitcode(0)
	{}


	void App::setup()
	{
		auto engine = core::add_subsystem<Engine>();
	}

	void App::start()
	{
		auto engine = core::get_subsystem<Engine>();

		sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
		desktop.width = 1280;
		desktop.height = 720;
		auto main_window = std::make_shared<RenderWindow>(
			desktop,
			"App",
			sf::Style::Default);
		

		if(!engine->start(main_window))
		{
			_exitcode = -1;
			return;
		}
	}

	void App::stop()
	{
		auto engine = core::get_subsystem<Engine>();
		engine->destroy_windows();
	}

	int App::run()
	{
		core::details::initialize();

		setup();
		if (_exitcode != 0)
		{
			core::details::dispose();
			return _exitcode;
		}

		start();
		if (_exitcode != 0)
		{
			core::details::dispose();
			return _exitcode;
		}

		auto engine = core::get_subsystem<Engine>();
		while (engine->is_running())
			engine->run_one_frame();

		stop();

		core::details::dispose();
		return _exitcode;
	}

	void App::quit_with_error(const std::string& message)
	{
		APPLOG_ERROR(message.c_str());
		quit(-1);
	}

	void App::quit(int exitcode)
	{
		auto engine = core::add_subsystem<Engine>();
		engine->set_running(false);
		_exitcode = exitcode;
	}

}