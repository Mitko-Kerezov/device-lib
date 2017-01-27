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

bool contains(const std::string& text, const std::string& search_string, int start_position)
{
	std::string::size_type index_of_documents = text.find(search_string, start_position);
	return index_of_documents != std::string::npos;
}
