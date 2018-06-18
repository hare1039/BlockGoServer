#ifndef __ENABLE_SPDLOG_HPP__
#define __ENABLE_SPDLOG_HPP__

#include <boost/core/demangle.hpp>
#include <spdlog/spdlog.h>

namespace blockgo
{

template <typename T>
class enable_spdlog
{
	static
	std::string& name()
	{
		static std::string n{boost::core::demangle(typeid(T).name())};
		return n;
	} 
	
public:
	enable_spdlog()
	{
		if (not spdlog::get(name()))
			spdlog::stdout_color_mt(name());
	}
	
	static inline
	auto spdlog()
	{
		return spdlog::get(name());
	}
};

} // namespace blockgo
#endif // __ENABLE_SPDLOG_HPP__
