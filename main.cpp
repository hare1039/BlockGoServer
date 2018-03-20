#include <iostream>
#include <map>
#include <functional>
#include <thread>
#include <vector>
#include <memory>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json.hpp>

#include "game_state.hpp"
#include "websocket_server.hpp"
int main()
{
	blockgo::websocket_server game;
	try
	{
		game.run(9002);
	}
	catch (websocketpp::exception const & e)
	{
		std::cout << e.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "other exception" << std::endl;
	}
}
