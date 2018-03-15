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

std::string run()
{
	int rc;
	try
	{
		Poco::Pipe outPipe;
		Poco::ProcessHandle ph = Poco::Process::launch("echo", std::vector<std::string>{"hi"}, 0, &outPipe, 0);
		rc = ph.id();
		Poco::PipeInputStream istr(outPipe);

		std::stringstream ss;
		Poco::StreamCopier::copyStream(istr, ss);
		return ss.str();
	}
	catch (Poco::SystemException& exc)
	{
		std::cout << exc.displayText() << std::endl;
		return "";		        
	}
	return "";
}



class game_state
{
public:
	std::thread th;
	game_state()
	{
		
	}

	std::string launch_game_app()
	{
		return run();
	}
};

#endif //__GAME_STATE_HPP__
