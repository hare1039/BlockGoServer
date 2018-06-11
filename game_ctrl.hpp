#ifndef __GAME_CTRL_HPP__
#define __GAME_CTRL_HPP__

#include <thread>
#include <string>
#include <iostream>
#include <sstream>
#include <queue>
#include <mutex>
#include <memory>
#include <cassert>

#include <spdlog/spdlog.h>
#include <boost/process.hpp>

#include "websocket_server.hpp"

namespace blockgo
{

namespace PLAYER_TYPE {enum player_type { NONE, HUMAN, MCTS };}
namespace bp = boost::process;
class websocket_server_base
{
public:
	virtual ~websocket_server_base() {}
	virtual void send(websocketpp::connection_hdl const &, std::string) = 0;
};

class game_ctrl : public std::enable_shared_from_this<game_ctrl>
{
	std::unique_ptr<bp::child> handler;

	// ref to parent websocket server to send async message
	websocket_server_base & ws;
	websocketpp::connection_hdl hdl;

	bp::async_pipe ipipe, opipe;
	boost::asio::streambuf read_buf;
	std::queue<std::string> write_msg_queue;
    std::mutex write_msg_queue_mtx;
public:
	game_ctrl(websocket_server_base & wsp,
	          websocketpp::connection_hdl h,
	          boost::asio::io_service & io): ws(wsp), hdl(h), ipipe(io), opipe(io)
	{
		if (not spdlog::get("game_ctrl") /* found */)
			spdlog::stdout_color_mt("game_ctrl");

		spdlog::get("game_ctrl")->trace("hdl: {}", hdl.lock().get());
		handler.reset(
			new bp::child{"./ai-project/BlockGo/BlockGoStatic", "web",
#ifdef NDEBUG
                           bp::std_out > bp::null,
#else
					       bp::std_out > stderr,
#endif
       	                   bp::std_in  < opipe,
	                       bp::std_err > ipipe
	    });
	}

	~game_ctrl()
	{
		handler->terminate();
	}

	void start_read()
	{
		launch_read_pipe();
	}

	void stop()
	{
		handler->terminate();
	}

	void send_stdin(std::string const &s)
	{
		spdlog::get("game_ctrl")->trace("queueing: {}", s);
		bool write_in_progress = not write_msg_queue.empty();
		{
		    std::lock_guard<std::mutex> guard(write_msg_queue_mtx);
		    if (s.back() != '\n')
			    write_msg_queue.push(s + "\n");
		    else
			    write_msg_queue.push(s);
		}
		if (not write_in_progress)
			launch_write_pipe();
		spdlog::get("game_ctrl")->trace("launched: {}", s);
	}

private:
	void launch_read_pipe()
	{
		spdlog::get("game_ctrl")->info("launch read pipe");
		spdlog::get("game_ctrl")->trace("with hdl {}", hdl.lock().get());
		auto self(shared_from_this());
	    boost::asio::async_read_until(
			this->ipipe,
			read_buf,
			'\n',
			[this, self](boost::system::error_code const &error, std::size_t)
			{
				if (not error)
				{
					std::istream is(&read_buf);
					std::string line;
					std::getline(is, line);
					spdlog::get("game_ctrl")->debug("async read pipe and send: {}", line);
					spdlog::get("game_ctrl")->trace("hdl: {}", hdl.lock().get());
					ws.send(hdl, line);
				    launch_read_pipe();
				}
				else if (error == boost::asio::error::eof)
				{
					spdlog::get("game_ctrl")->info("read EOF. Closing...");
				}
				else
				{
					spdlog::get("game_ctrl")->error("read err: {}", error.message());
				}
			}
	    );
	}
	void launch_write_pipe()
	{
		spdlog::get("game_ctrl")->debug("launch write pipe");
		auto self(shared_from_this());
		boost::asio::async_write(
			this->opipe,
			boost::asio::buffer(write_msg_queue.front()),
			[this, self] (boost::system::error_code const &error, std::size_t)
			{
				if (not error)
				{
				    {
						std::lock_guard<std::mutex> guard(write_msg_queue_mtx);
						spdlog::get("game_ctrl")->debug("wrote pipe: {}", write_msg_queue.front());
						write_msg_queue.pop();
				    }

				    if (not write_msg_queue.empty())
						launch_write_pipe();
				}
				else
				{
					spdlog::get("game_ctrl")->error("write pipe err: {}", error.message());
				}
			}
		);
	}
};

} // namespace blockgo
#endif //__GAME_CTRL_HPP__
