#include "StringHelper.h"

#include <sstream>
#include <iomanip>

std::string to_hex(const std::string & s)
{
	std::ostringstream ret;

	for (size_t i = 0; i < s.length(); ++i)
		ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int)s[i];

	return ret.str();
}
