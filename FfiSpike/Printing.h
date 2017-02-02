#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif

#include "json.hpp"
#include <string>

void print(const char* str);
void print(const nlohmann::json& message);
void print_error(const char *message, std::string device_identifier, std::string method_id, int code =
#ifdef _WIN32
	GetLastError()
#else
	kAMDNotFoundError
#endif
);