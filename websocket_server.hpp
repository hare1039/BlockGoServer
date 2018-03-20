#ifndef  __WEBSOCKET_SERVER_HPP__
#define  __WEBSOCKET_SERVER_HPP__

#include <iostream>
#include <map>
#include <exception>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "game_state.hpp"

namespace blockgo
{
	
class websocket_server
{
public:
	websocket_server()
	{
		server.init_asio();
		server.set_message_handler ([this](websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg){this->on_message(hdl, msg);});
		server.set_open_handler    ([this](websocketpp::connection_hdl hdl){this->on_open(hdl);});
		server.set_fail_handler    ([this](websocketpp::connection_hdl hdl){this->on_fail(hdl);});
		server.set_close_handler   ([this](websocketpp::connection_hdl hdl){this->on_close(hdl);});
	}
	void run(int on_port)
	{
		server.listen(on_port);
		server.start_accept();
		server.run();
	}
private:
	websocketpp::server<websocketpp::config::asio> server;
	std::map<websocketpp::connection_hdl,
	         std::shared_ptr<blockgo::game_state>,
	         std::owner_less<websocketpp::connection_hdl>> game;
	
	void on_open(websocketpp::connection_hdl hdl)
	{
		game.emplace(hdl, new blockgo::game_state);
	}
	void on_fail  (websocketpp::connection_hdl hdl)
	{
		std::cout << "connection failed: " << hdl.lock().get() << std::endl; 
        game.erase(hdl);
	}
	void on_close (websocketpp::connection_hdl hdl)
	{		        
		std::cout << "Closing connection: " << hdl.lock().get() << std::endl; 
        game.erase(hdl);
	}
	void on_message (websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg)
	{
		std::cout << "on_message called with hdl: " << hdl.lock().get()
		          << " and message: " << msg->get_payload()
		          << std::endl;

		try
		{
			auto command = nlohmann::json::parse(msg->get_payload());
			std::cout << command.dump(4) << std::endl;
			try
			{
				server.send(hdl, game[hdl]->send_stdin("hi"), msg->get_opcode());
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
	std::shared_ptr<blockgo::game_state> get_game_from(websocketpp::connection_hdl hdl)
	{
		auto it = game.find(hdl);
        
		if (it == game.end())
			throw std::invalid_argument("No data avaliable for session");
        
		return it->second;
	}
};

}// namespace blockgo
#endif //__WEBSOCKET_SERVER_HPP__
