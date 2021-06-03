#include <sstream>
#include "conversions.h"

/**
 *  @brief  Converts an int-type to string.
 *  @author Lau Lee Hong
 *  @param  i (int)integer to be converted (std::string)integer
 *  @return Integer of string format
 */
std::string int_to_string(int v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}