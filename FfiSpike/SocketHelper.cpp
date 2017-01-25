#include "SocketHelper.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#endif

#ifndef _WIN32
#include <sys/socket.h>
#endif

#include "PlistCpp/Plist.hpp"
#include "PlistCpp/PlistDate.hpp"
#include "PlistCpp/include/boost/any.hpp"


int send_message(const char* message, SOCKET socket, long long length)
{
	LengthEncodedMessage length_encoded_message = get_message_with_encoded_length(message, length);
	return send(socket, length_encoded_message.message, length_encoded_message.length, 0);
}

std::map<std::string, boost::any> receive_message(SOCKET socket)
{
	std::map<std::string, boost::any> dict;
	char *buffer = new char[4];
	int bytes_read = recv(socket, buffer, 4, 0);
	unsigned long res = ntohl(*((unsigned long*)buffer));
	delete[] buffer;
	buffer = new char[res];
	bytes_read = recv(socket, buffer, res, 0);
	Plist::readPlist(buffer, res, dict);
	delete[] buffer;
	return dict;
}

LengthEncodedMessage get_message_with_encoded_length(const char* message, long long length)
{
	if (length < 0)
		length = strlen(message);

	unsigned long message_len = length + 4;
	char *length_encoded_message = new char[message_len];
	unsigned long packed_length = htonl(length);
	size_t packed_length_size = 4;
	memcpy(length_encoded_message, &packed_length, packed_length_size);
	memcpy(length_encoded_message + packed_length_size, message, length);
	return{ length_encoded_message, message_len };
}
