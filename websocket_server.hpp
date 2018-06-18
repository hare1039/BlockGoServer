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
#include "enable_spdlog.hpp"
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
					  , public enable_spdlog<websocket_server>
{
	struct game_attrbute : public std::enable_shared_from_this<game_attrbute>
	{
		std::shared_ptr<blockgo::game_ctrl> game_ptr;
		nlohmann::json                      json;

		game_attrbute(websocket_server & ws, websocketpp::connection_hdl hdl):
			game_ptr{new blockgo::game_ctrl{ws, hdl}},
			json{}
		{
			spdlog()->trace("game_attrbute construct");
		    game_ptr->start_read();
		}

		~game_attrbute()
		{
			game_ptr->stop();
		}
	};

public:
	websocket_server()
	{
		spdlog::stdout_color_mt("websocket");
		server.init_asio();

		server.set_message_handler ([this](websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg){this->on_message(hdl, msg);});
		server.set_open_handler    ([this](websocketpp::connection_hdl hdl){this->on_open(hdl);});
		server.set_fail_handler    ([this](websocketpp::connection_hdl hdl){this->on_fail(hdl);});
		server.set_close_handler   ([this](websocketpp::connection_hdl hdl){this->on_close(hdl);});
#ifdef NDEBUG
		server.set_access_channels(websocketpp::log::alevel::none);
#else
		server.set_access_channels(websocketpp::log::alevel::all);
#endif
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
			 std::shared_ptr<game_attrbute>,
			 std::owner_less<websocketpp::connection_hdl>> game;

	void on_open  (websocketpp::connection_hdl hdl)
	{
		spdlog()->info("Opening connection: {}", hdl.lock().get());
		game.emplace(hdl, new game_attrbute{*this, hdl});
	}
	void on_fail  (websocketpp::connection_hdl hdl)
	{
		spdlog()->error("Connection failed: {}", hdl.lock().get());
        game.erase(hdl);
	}
	void on_close (websocketpp::connection_hdl hdl)
	{
		spdlog()->info("Closing connection: {}", hdl.lock().get());
        game.erase(hdl);
	}
	void on_message (websocketpp::connection_hdl hdl, decltype(server)::message_ptr msg)
	{
		try
		{
		    spdlog()->info("get msg: {}", msg->get_payload());
			auto command = nlohmann::json::parse(msg->get_payload());
			ask_block_go(hdl, command);
		}
		catch (websocketpp::lib::error_code const &e)
		{
			spdlog()->error("on message failed because: {}", e.message());
		}
		catch (nlohmann::json::parse_error const &e)
		{
			spdlog()->error("on message failed because: {}", e.what());
		}
	}

	void send(websocketpp::connection_hdl const &hdl, std::string s) override
	{
		spdlog()->trace("sending by hdl: {}", s);

		// special strings
		if (s == "")
		{
			server.send(hdl, (nlohmann::json{
				{"cmd", "status"},
				{"status", "ok"},
				{"why", ""}
			}).dump(), websocketpp::frame::opcode::text);
			return;
		}
		else if (s == "eof")
		{
			server.send(hdl, (nlohmann::json{
                {"cmd", "status"},
                {"status", "err"},
                {"why", "Game process terminated."}
            }).dump(), websocketpp::frame::opcode::text);
			return;
		}

		try
		{
			auto res = nlohmann::json::parse(s);
			if (res.find("winner") != res.end())
			{
				server.send(hdl, (nlohmann::json{
					{"cmd", "status"},
					{"status", "end"},
					{"why", s}
				}).dump(), websocketpp::frame::opcode::text);
			}
			else
			{
				// adjust coordinate from BlockGo to web
				int x = res.at("x").get<int>() - 1;
				int y = res.at("y").get<int>() - 1;
				int rotate = (res.at("rotate").get<int>() + 3) % 4;
				res["cmd"]       = "transfer";
				res.at("x")      = x;
				res.at("y")      = y;
				res.at("rotate") = rotate;
				res["player"]    = nlohmann::json({
					{"current", game.at(hdl)->json.at("player").at("next")},
					{"next", PLAYER_TYPE::NONE},
				});

				server.send(hdl, res.dump(), websocketpp::frame::opcode::text);
				spdlog()->trace("sent: {}", s);
			}
		}
		catch (nlohmann::json::parse_error const &e)
		{
			spdlog()->debug("trying to revert map");
			// try revert to (1, 1)
			if (game.at(hdl)->json.count("handled") == 0 || not game.at(hdl)->json.at("handled"))
			{
				server.send(hdl, (nlohmann::json{
					{"cmd", "status"},
					{"status", "err"},
					{"why", s},
					{"origin", game.at(hdl)->json}
				}).dump(), websocketpp::frame::opcode::text);

				int x      = game.at(hdl)->json["x"];
				int y      = game.at(hdl)->json["y"];
				int rotate = game.at(hdl)->json["rotate"];
				game.at(hdl)->json["handled"] = true;
				std::stringstream command;
				std::fill_n(std::ostream_iterator<char>(command), 4 - rotate, 'c');
				std::fill_n(std::ostream_iterator<char>(command), x, 'a');
				std::fill_n(std::ostream_iterator<char>(command), y, 'w');
				game.at(hdl)->game_ptr->send_stdin(command.str());
			}
		}
		catch (nlohmann::json::exception const &e)
		{
			using namespace std::literals;
			spdlog()->error("json error: {}", e.what());
            server.send(hdl, (nlohmann::json{
                {"cmd", "status"},
                {"status", "err"},
                {"why", "unexpect json error: "s + e.what()}
            }).dump(), websocketpp::frame::opcode::text);
		}
		catch (std::out_of_range const &e)
		{
			spdlog()->error("hdl {} have gone. skipping.", hdl.lock().get());
		}
	}

	void ask_block_go(websocketpp::connection_hdl const &hdl, nlohmann::json const & json)
	{
		blockgo::game_ctrl &blockgo = *(game.at(hdl)->game_ptr);
		game.at(hdl)->json = json;
		spdlog()->debug("{}", json.dump(4));
		try
		{
			switch (hash(json.at("cmd").get<std::string>().c_str()))
			{
			case "start"_:
			{
				auto start_player = json.at("left").get<int>();
				if (start_player != PLAYER_TYPE::HUMAN)
				{
					blockgo.send_stdin(std::to_string(start_player));
					game.at(hdl)->json["player"] = nlohmann::json({
						{"current", start_player},
						{"next", PLAYER_TYPE::NONE},
					});
				}

				break;
			}
			case "transfer"_:
			{
				int x      = json.at("x");
				int y      = json.at("y");
				int type   = json.at("stone");
				int rotate = json.at("rotate");
				PLAYER_TYPE::player_type player = json.at("player").at("current");
				std::stringstream command;

				// converting json commands to feed into BlockGo

				spdlog()->trace("game attr: {}", game.at(hdl)->json.dump(4));
				if (player == PLAYER_TYPE::HUMAN)
				{
					command << player << type;
					std::fill_n(std::ostream_iterator<char>(command), rotate, 'c');
					std::fill_n(std::ostream_iterator<char>(command), x, 'd');
					std::fill_n(std::ostream_iterator<char>(command), y, 's');
					command << 'y';
					if (json.at("player").at("next") != PLAYER_TYPE::HUMAN)
						command << json.at("player").at("next");
				}
				else
				{
					command << json.at("player").at("current");
				}

				blockgo.send_stdin(command.str());
				break;
			}

			case "end"_:
				break;

			case "debug"_:
				blockgo.send_stdin(json.at("data"));
				break;

			default:
				spdlog()->error("[parse cmd] AAAhhh... no one gets here!\n");
				break;
			}
		}
		catch (nlohmann::json::exception const &e)
		{
			spdlog()->error("json.cmd parse failed. What: {} \ne-id: {}\n", e.what(), e.id);
		}
	}
};

}// namespace blockgo
#endif //__WEBSOCKET_SERVER_HPP__
