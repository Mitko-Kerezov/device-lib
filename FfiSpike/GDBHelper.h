#pragma once

#include "SocketHelper.h"
#include <string>

std::string get_gdb_message(std::string message);
int gdb_send_message(std::string message, SOCKET socket, long long length = -1);
void init(std::string& executable, SOCKET socket);
void run_application(std::string& executable, SOCKET socket);
void stop_application(std::string& executable, SOCKET socket);