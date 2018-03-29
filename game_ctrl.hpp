#ifndef __GAME_CTRL_HPP__
#define __GAME_CTRL_HPP__

#include <thread>
#include <string>
#include <iostream>
#include <sstream>
#include <queue>

#include <spdlog/spdlog.h>
#include <boost/process.hpp>

#include "websocket_server.hpp"

namespace blockgo
{


namespace bp = boost::process;
class websocket_server_base
{
public:
	virtual ~websocket_server_base() {}
	virtual void send(websocketpp::connection_hdl const &, std::string) = 0;
};

class game_ctrl
{
	std::unique_ptr<bp::child> handler;

	bp::async_pipe ipipe, opipe;
	boost::asio::streambuf read_buf;
	std::queue<std::string> write_msg_queue;
	// ref to parent websocket server to send async message
	websocket_server_base & ws;
	websocketpp::connection_hdl hdl;
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
//			new bp::child{"/bin/cat", "-",
					       bp::std_out > ipipe,
       	                   bp::std_in  < opipe,
//	                       bp::std_err > bp::null
	                       bp::std_err > stderr
	    });
		launch_read_pipe();
	}

	~game_ctrl()
	{
		spdlog::get("game_ctrl")->trace("deconstructing");
		handler->terminate();
	}

	void send_stdin(std::string const &s)
	{
		spdlog::get("game_ctrl")->trace("sending: {}", s);
		if (s.back() != '\n')
			write_msg_queue.push(s + "\n");
		else
			write_msg_queue.push(s);
		launch_write_pipe();
		spdlog::get("game_ctrl")->trace("launched: {}", s);
	}

private:
	void launch_read_pipe()
	{
		spdlog::get("game_ctrl")->info("launch read pipe");
		spdlog::get("game_ctrl")->trace("with hdl {}", hdl.lock().get());
	    boost::asio::async_read_until(
			this->ipipe,
			read_buf,
			'\n',
			[this](boost::system::error_code const &error, std::size_t)
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
				else
				{
					spdlog::get("game_ctrl")->error("write read err: {}", error.message());
				}
			}
	    );
	}
	void launch_write_pipe()
	{
		spdlog::get("game_ctrl")->debug("launch write pipe");
		boost::asio::async_write(
			this->opipe,
			boost::asio::buffer(write_msg_queue.front()),
			[this] (boost::system::error_code const &error, std::size_t)
			{
				if (not error)
				{
					spdlog::get("game_ctrl")->debug("wrote pipe: {}", write_msg_queue.front());
					write_msg_queue.pop();
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
