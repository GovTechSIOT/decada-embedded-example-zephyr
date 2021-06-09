#ifndef _CONVERSIONS_H_
#define _CONVERSIONS_H_

#include <string>

std::string hash_sha256(std::string input);
std::string int_to_string(int v);
char* StringToChar(const std::string& str);

#endif // _CONVERSIONS_H_