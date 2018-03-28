#include <iostream>
#include <map>
#include <functional>
#include <thread>
#include <vector>
#include <memory>

#include <spdlog/spdlog.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json.hpp>

#include "websocket_server.hpp"
int main(int argc, char *argv[])
{
	spdlog::stdout_color_mt("main");
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [%l] %v");
	blockgo::websocket_server game;
	try
	{
		spdlog::get("main")->info("Server started at :9002");
		game.run(9002);
	}
	catch (websocketpp::exception const & e)
	{
		spdlog::get("main")->critical("{}", e.what());
	}
	catch (...)
	{
		spdlog::get("main")->critical("other exception");
	}
	return 0;
}
