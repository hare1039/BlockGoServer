#include <iostream>
#include <map>
#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <cassert>

#include <spdlog/spdlog.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json.hpp>

#include "websocket_server.hpp"
int main(int argc, char *argv[])
{
	spdlog::stdout_color_mt("main");
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [%t] [%l] %v");
#ifdef NDEBUG
	spdlog::set_level(spdlog::level::info);
#else
	spdlog::set_level(spdlog::level::trace);
#endif
	blockgo::websocket_server game;
	try
	{
		int port = 9002;
		if (argc == 2)
			port = std::stoi(argv[1]);
		spdlog::get("main")->info("Server started at :" + std::to_string(port));
		game.run(port);
	}
	catch (websocketpp::exception const & e)
	{
		spdlog::get("main")->critical("{}", e.what());
	}
	catch (std::exception const & e)
	{
		spdlog::get("main")->critical("other exception: {}", e.what());
	}
	return 0;
}
