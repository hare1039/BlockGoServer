#include <iostream>
#include <map>
#include <functional>
#include <thread>
#include <vector>
#include <memory>

#include <boost/log/trivial.hpp>

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
		BOOST_LOG_TRIVIAL(info) << "Server started at :9002" << std::endl;
		game.run(9002);
	}
	catch (websocketpp::exception const & e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what() << std::endl;
	}
	catch (...)
	{
		BOOST_LOG_TRIVIAL(error) << "other exception" << std::endl;
	}
	return 0;
}
