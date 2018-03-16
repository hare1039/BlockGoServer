#ifndef __GAME_STATE_HPP__
#define __GAME_STATE_HPP__

#include <thread>
#include <string>
#include <iostream>
#include <sstream>

#include <Poco/PipeStream.h>
#include <Poco/Pipe.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>

class game_state
{
	Poco::ProcessHandle *handler = nullptr;
	Poco::Pipe in, out;

	Poco::PipeInputStream  reader;
	Poco::PipeOutputStream writer;
public:

	game_state(): reader(out),
	              writer(in)
	{
		// There are huge template error when I use std::unique_ptr, so I just switch to raw pointer now.
		handler = new Poco::ProcessHandle{Poco::Process::launch("cat", {"-"}, &in, &out, nullptr)};
	}

	~game_state()
	{
		Poco::Process::kill(*handler);
		if (handler)
			delete handler;
	}

	std::string launch_game_app()
	{
		writer << "catting" << std::endl;
		try
		{
			std::string s;
			reader >> s;
			return s;
		}
		catch (Poco::SystemException& exc)
		{
			std::cout << exc.displayText() << std::endl;
		}
		return "";

	}
};

#endif //__GAME_STATE_HPP__
