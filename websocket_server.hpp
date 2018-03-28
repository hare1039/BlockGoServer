#ifndef  __WEBSOCKET_SERVER_HPP__
#define  __WEBSOCKET_SERVER_HPP__

#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>
#include <exception>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <spdlog/spdlog.h>
#include "game_state.hpp"


namespace blockgo
{

constexpr
long long int hash(const char* str, int h = 0)
{
	return (!str[h] ? 5381 : (hash(str, h+1)*33) ^ str[h] );
}
constexpr
long long int operator "" _(char const* p, size_t)
{
	return hash(p);
}

class websocket_server
{
public:
	websocket_server()
	{
		spdlog::stdout_color_mt("websocket");
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
	         std::unique_ptr<blockgo::game_state>,
	         std::owner_less<websocketpp::connection_hdl>> game;

	void on_open(websocketpp::connection_hdl hdl)
	{
		game.emplace(hdl, new blockgo::game_state);
	}
	void on_fail  (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->error("Connection failed: {}", hdl.lock().get());
        game.erase(hdl);
	}
	void on_close (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->info("Closing connection: {}", hdl.lock().get());
        game.erase(hdl);
	}
	void on_message (websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg)
	{
		try
		{
			auto command = nlohmann::json::parse(msg->get_payload());
			server.send(hdl, parse_cmd(hdl, command), msg->get_opcode());
		}
		catch (websocketpp::lib::error_code const &e)
		{
			spdlog::get("websocket")->error("on message failed because: {}", e.message());
		}
		catch (nlohmann::json::parse_error const &e)
		{
			spdlog::get("websocket")->error("on message failed because: {}", e.what());
		}
	}

	std::string parse_cmd(websocketpp::connection_hdl const &hdl, nlohmann::json const & json)
	{
		spdlog::get("websocket")->info("{}", json.dump(4));
		try
		{
			switch (hash(json["cmd"].get<std::string>().c_str()))
			{
			case "start"_:
			{
				if (json["left"] == "MCTS" && json["right"] == "human")
				{
					return game[hdl]->send_stdin("4");
				}
				else if (json["left"] == "human" && json["right"] == "MCTS")
				{
					game[hdl]->send_stdin("3", /* expect return */ false);
				}
				return (nlohmann::json{
					{"cmd", "status"},
					{"status", "ok"},
					{"why", ""}
				}).dump();
			}

			case "transfer"_:
			{
				int x      = json["x"];
				int y      = json["y"];
				int type   = json["stone"];
				int rotate = json["rotate"];
				std::stringstream command;

				// converting json commands to fed into BlockGo
				command << type;
				std::fill_n(std::ostream_iterator<char>(command), x, 'd');
				std::fill_n(std::ostream_iterator<char>(command), y, 's');
				std::fill_n(std::ostream_iterator<char>(command), rotate, 'c');
				command << 'y';

				try
				{
					auto res = nlohmann::json::parse(game[hdl]->send_stdin(command.str()));
					res["cmd"] = "transfer";
					res["x"] = res["x"].get<int>() - 1;
					res["y"] = res["y"].get<int>() - 1;
					return res.dump();
				}
				catch (nlohmann::json::parse_error const &e)
				{
					spdlog::get("websocket")->error("process output json failed because: {}", e.what());
					return R"json({
                        "cmd": "status",
                        "status": "err",
                        "why": "unable to parse returned json"
                    })json";
				}
                return R"json({
                    "cmd": "status",
                    "status": "err",
                    "why": "unexpected exception happened"
                })json";
			}

			case "end"_:
			{
				break;
			}

			case "debug"_:
			{
				return game[hdl]->send_stdin(json["data"]);
				break;
			}

			default:
				spdlog::get("websocket")->error("[parse cmd] AAAAAAh no one gets here!\n");
				break;
			}
		}
		catch (nlohmann::json::exception const &e)
		{
			spdlog::get("websocket")->error("json.cmd parse failed. What: {} \ne-id: {2} \n", e.what(), e.id);
		}
		return "";
	}
};

}// namespace blockgo
#endif //__WEBSOCKET_SERVER_HPP__
