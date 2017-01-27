#pragma once
#include <string>

std::string to_hex(const std::string& s);

bool contains(const std::string& text, const std::string& search_string, int start_position = 0);
