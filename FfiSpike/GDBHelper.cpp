#include "GDBHelper.h"
#include "StringHelper.h"
#include <sstream>
#include <iomanip>

std::string get_gdb_message(std::string message)
{
	size_t sum = 0;
	std::stringstream result;
	for (char& c : message)
		sum += c;

	sum &= 0xff;

	result << "$" << message << "#" << std::hex << sum;
	return result.str();
}

int gdb_send_message(std::string message, SOCKET socket, long long length)
{
	return send_message(get_gdb_message(message), socket, length);
}

void init(std::string& executable, SOCKET socket)
{
	gdb_send_message("QStartNoAckMode", socket);
	send_message("+", socket);
	gdb_send_message("QEnvironmentHexEncoded:", socket);
	gdb_send_message("QSetDisableASLR:1", socket);
	std::stringstream result;
	result << "A" << executable.size() * 2 << ",0," << to_hex(executable);
	gdb_send_message(result.str(), socket);
}

void run_application(std::string& executable, SOCKET socket)
{
	init(executable, socket);
	gdb_send_message("qLaunchSuccess", socket);
	gdb_send_message("vCont;c", socket);
}

void stop_application(std::string & executable, SOCKET socket)
{
	init(executable, socket);
	gdb_send_message("k", socket);
}
