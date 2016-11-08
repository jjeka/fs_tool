#ifndef UTILS
#define UTILS

#include <string>
#include <algorithm>
#include <cstring>

using namespace std::string_literals;

std::string& ltrim(std::string &s);
std::string& rtrim(std::string &s);
std::string& trim(std::string &s);

std::string to_string_time(int n);

#endif // UTILS
