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
		server.set_access_channels(websocketpp::log::alevel::all);
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
	         std::owner_less<websocketpp::connection_hdl>> game_pool;

	std::map<websocketpp::connection_hdl,
             nlohmann::json,
             std::owner_less<websocketpp::connection_hdl>> game_attrbute;

	void on_open  (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->info("Opening connection: {}", hdl.lock().get());
		auto io = std::make_unique<boost::asio::io_service>();
		spdlog::get("websocket")->trace("game.emplace");
		game.emplace(hdl, new blockgo::game_ctrl(*this, hdl, *io));
		spdlog::get("websocket")->trace("game_pool.emplace");
		game_pool.emplace(hdl, new std::thread{[io = std::move(io)]{io->run();}});
		game_attrbute[hdl] = nlohmann::json{};
	}
	void on_fail  (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->error("Connection failed: {}", hdl.lock().get());
        game.erase(hdl);
        spdlog::get("websocket")->error("joining thread: {}", hdl.lock().get());

		if (game_pool.find(hdl) != game_pool.end())
		{
			game_pool[hdl]->join();
			game_pool.erase(hdl);
		}
		game_attrbute.erase(hdl);
	}
	void on_close (websocketpp::connection_hdl hdl)
	{
		spdlog::get("websocket")->info("Closing connection: {}", hdl.lock().get());
        game.erase(hdl);
        spdlog::get("websocket")->trace("joining thread: {}", hdl.lock().get());
        game_pool[hdl]->join();
        game_pool.erase(hdl);
        game_attrbute.erase(hdl);
	}
	void on_message (websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg)
	{
		try
		{
		    spdlog::get("websocket")->trace("get msg: {}", msg->get_payload());
			auto command = nlohmann::json::parse(msg->get_payload());
			ask_block_go(hdl, command);
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

		// special strings
		if (s == "")
		{
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "ok"},
				{"why", ""}
			}).dump(), websocketpp::frame::opcode::text);
		}
		else if (s == "P1 win" || s == "P2 win" || s == "DRAW")
		{
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "end"},
				{"why", s}
			}).dump(), websocketpp::frame::opcode::text);
			return;
		}


		try
		{
			auto res = nlohmann::json::parse(s);
			res["cmd"]    = "transfer";
			res["x"]      = res["x"].get<int>() - 1;
			res["y"]      = res["y"].get<int>() - 1;
			res["rotate"] = (res["rotate"].get<int>() + 3) % 4;
			server.send(hdl, res.dump(), websocketpp::frame::opcode::text);
		}
		catch (nlohmann::json::parse_error const &e)
		{
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "err"},
				{"why", s},
				{"origin", game_attrbute[hdl]}
			}).dump(), websocketpp::frame::opcode::text);

			// try revert to (1, 1)
			if (game_attrbute[hdl].count("stone") != 0)
			{
				spdlog::get("websocket")->debug("trying to revert map");
				int x      = game_attrbute[hdl]["x"];
				int y      = game_attrbute[hdl]["y"];
				int rotate = game_attrbute[hdl]["rotate"];
				std::stringstream command;
				std::fill_n(std::ostream_iterator<char>(command), 4 - rotate, 'c');
				std::fill_n(std::ostream_iterator<char>(command), x, 'a');
				std::fill_n(std::ostream_iterator<char>(command), y, 'w');
				game[hdl]->send_stdin(command.str());
				game_attrbute[hdl] = nlohmann::json{};
			}
		}
		catch (nlohmann::json::exception const &e)
		{
			using namespace std::literals;
			spdlog::get("websocket")->error("json error: {}", e.what());
            server.send(hdl, (nlohmann::json{
                {"cmd", "status"},
                {"status", "err"},
                {"why", "unexpect json error: "s + e.what()}
            }).dump(), websocketpp::frame::opcode::text);
		}
		spdlog::get("websocket")->trace("sent: {}", s);
	}

	void ask_block_go(websocketpp::connection_hdl const &hdl, nlohmann::json const & json)
	{
		blockgo::game_ctrl &blockgo = *game[hdl];
		game_attrbute[hdl] = json;
		spdlog::get("websocket")->debug("{}", json.dump(4));
		try
		{
			switch (hash(json.at("cmd").get<std::string>().c_str()))
			{
			case "start"_:
			{
				if      (json.at("left") == "MCTS"  && json.at("right") == "human")
					blockgo.send_stdin("4");
				else if (json.at("left") == "human" && json.at("right") == "MCTS")
					blockgo.send_stdin("3");
				else if (json.at("left") == "MCTS"  && json.at("right") == "MCTS")
					blockgo.send_stdin("2");
				else if (json.at("left") == "human" && json.at("right") == "human")
					blockgo.send_stdin("1");
				break;
			}
			case "transfer"_:
			{
				int x      = json.at("x");
				int y      = json.at("y");
				int type   = json.at("stone");
				int rotate = json.at("rotate");
				std::stringstream command;

				// converting json commands to fed into BlockGo

				command << type;
				std::fill_n(std::ostream_iterator<char>(command), rotate, 'c');
				std::fill_n(std::ostream_iterator<char>(command), x, 'd');
				std::fill_n(std::ostream_iterator<char>(command), y, 's');
				command << 'y';

				blockgo.send_stdin(command.str());
				break;
			}

			case "end"_:
				break;

			case "debug"_:
				blockgo.send_stdin(json.at("data"));
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
