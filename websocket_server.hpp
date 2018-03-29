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
#include "game_ctrl.hpp"


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

class websocket_server: public websocket_server_base
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
	         std::unique_ptr<blockgo::game_ctrl>,
	         std::owner_less<websocketpp::connection_hdl>> game;

	std::map<websocketpp::connection_hdl,
	         std::unique_ptr<std::thread>,
	         std::owner_less<websocketpp::connection_hdl>> thread_pool;

	void on_open  (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->info("Opening connection: {}", hdl.lock().get());
		auto io = std::make_unique<boost::asio::io_service>();
		spdlog::get("websocket")->trace("game.emplace");
		game.emplace(hdl, new blockgo::game_ctrl(*this, hdl, *io));
		spdlog::get("websocket")->trace("thread_pool.emplace");
		thread_pool.emplace(hdl, new std::thread{[io = std::move(io)]{io->run();}});
	}
	void on_fail  (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->error("Connection failed: {}", hdl.lock().get());
        game.erase(hdl);
        spdlog::get("websocket")->error("joining thread: {}", hdl.lock().get());
        thread_pool[hdl]->join();
        thread_pool.erase(hdl);
	}
	void on_close (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->info("Closing connection: {}", hdl.lock().get());
        game.erase(hdl);
        spdlog::get("websocket")->trace("joining thread: {}", hdl.lock().get());
        thread_pool[hdl]->join();
        thread_pool.erase(hdl);
	}
	void on_message (websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg)
	{
		try
		{
			auto command = nlohmann::json::parse(msg->get_payload());
			ask_block_go(*game[hdl], command);
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

	void send(websocketpp::connection_hdl const &hdl, std::string s) override
	{
		spdlog::get("websocket")->trace("sending by hdl: {}", s);
		if (s == "")
		{
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "ok"},
				{"why", ""}
			}).dump(), websocketpp::frame::opcode::text);
		}
		try
		{
			auto res = nlohmann::json::parse(s);
			res["cmd"]    = "transfer";
			res["x"]      = res["x"].get<int>() - 1;
			res["y"]      = res["y"].get<int>() - 1;
			res["rotate"] = 4 - (res["rotate"].get<int>() % 4);
			server.send(hdl, res.dump(), websocketpp::frame::opcode::text);
		}
		catch (nlohmann::json::parse_error const &e)
		{
			spdlog::get("websocket")->error("process output json failed because: {}", e.what());
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "err"},
				{"why", "unable to parse returned json"}
			}).dump(), websocketpp::frame::opcode::text);
		}
		catch (nlohmann::json::exception const &e)
		{
			spdlog::get("websocket")->error("json error: {}", e.what());
            server.send(hdl, (nlohmann::json{
                {"cmd", "status"},
                {"status", "err"},
                {"why", "unknown json error"}
            }).dump(), websocketpp::frame::opcode::text);
		}
		spdlog::get("websocket")->trace("sent: {}", s);
	}

	void ask_block_go(blockgo::game_ctrl &blockgo, nlohmann::json const & json)
	{
		spdlog::get("websocket")->debug("{}", json.dump(4));
		try
		{
			switch (hash(json["cmd"].get<std::string>().c_str()))
			{
			case "start"_:
			{
				if (json["left"] == "MCTS" && json["right"] == "human")
				{
					blockgo.send_stdin("4");
				}
				else if (json["left"] == "human" && json["right"] == "MCTS")
				{
					blockgo.send_stdin("3");
				}
				break;
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

				blockgo.send_stdin(command.str());
				break;
			}

			case "end"_:
				break;

			case "debug"_:
				blockgo.send_stdin(json["data"]);
				break;

			default:
				spdlog::get("websocket")->error("[parse cmd] AAAhhh... no one gets here!\n");
				break;
			}
		}
		catch (nlohmann::json::exception const &e)
		{
			spdlog::get("websocket")->error("json.cmd parse failed. What: {} \ne-id: {}\n", e.what(), e.id);
		}
	}
};

}// namespace blockgo
#endif //__WEBSOCKET_SERVER_HPP__
