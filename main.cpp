#include <iostream>
#include <map>
#include <functional>
#include <thread>
#include <vector>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json.hpp>

#include "game_state.hpp"
using server = websocketpp::server<websocketpp::config::asio>;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

using message_ptr = server::message_ptr;
using json = nlohmann::json;

std::map<websocketpp::connection_hdl,
         game_state,
         std::owner_less<websocketpp::connection_hdl>> game;
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg)
{
	std::cout << "on_message called with hdl: " << hdl.lock().get()
	          << " and message: " << msg->get_payload()
	          << std::endl;
	game[hdl] = game_state{};
	try
	{
		auto command = json::parse(msg->get_payload());		
		std::cout << command.dump(4) << std::endl;
		try
		{
			s->send(hdl, game[hdl].launch_game_app(), msg->get_opcode());
		}
		catch (const websocketpp::lib::error_code& e)
		{
			std::cout << "Echo failed because: " << e
			          << "(" << e.message() << ")" << std::endl;
		}
	}
	catch (nlohmann::json::parse_error const &e)
	{
		std::cerr << e.what() << std::endl;
		return;
	}
}

int main()
{
	server echo_server;
	try
	{
//		// Set logging settings
//		echo_server.set_access_channels(websocketpp::log::alevel::all);
//		echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

		
		echo_server.init_asio();
		echo_server.set_message_handler(std::bind(&on_message, &echo_server, ::_1, ::_2));

		echo_server.listen(9002);
		echo_server.start_accept();
		echo_server.run();
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
