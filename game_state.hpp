#ifndef __GAME_STATE_HPP__
#define __GAME_STATE_HPP__

#include <thread>
#include <string>
#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>
#include <boost/process.hpp>

namespace blockgo
{

namespace bp = boost::process;
class game_state
{
	std::unique_ptr<bp::child> handler;

	bp::ipstream reader;
	bp::opstream writer;
public:

	game_state()
	{
		spdlog::stdout_color_mt("game_state");
		handler.reset(
			new bp::child{"./ai-project/BlockGo/BlockGoStatic", "web",
					       bp::std_out > reader,
       	                   bp::std_in  < writer,
	                       bp::std_err > bp::null
	    });
	}

	~game_state()
	{
		handler->terminate();
	}

	std::string send_stdin(std::string const &s, bool expect_return = true)
	{
		spdlog::get("game_state")->info("sending: {}", s);
		writer << s << std::endl;
		spdlog::get("game_state")->info("sent: {}", s);
		try
		{
			if (expect_return)
			{
				spdlog::get("game_state")->info("expecting output...");
				std::string result;
				std::getline(reader, result);
				reader.clear();
				spdlog::get("game_state")->info("output get: {}", result);
				return result;
			}
		}
		catch (std::system_error const & exc)
		{
			spdlog::get("game_state")->error("std::system_error: {}", exc.what());
		}
		return "";
	}
};

} // namespace blockgo
#endif //__GAME_STATE_HPP__
