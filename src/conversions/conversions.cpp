#include <logging/log.h>
LOG_MODULE_REGISTER(conversions, LOG_LEVEL_DBG);

#include <sstream>
#include <mbedtls/sha256.h>
#include "conversions.h"

/**
 *  @brief	Generates SHA256 hash from input
 *  @author	Lee Tze Han
 *  @param	input	String to be hashed
 *  @return	SHA256 hash (Lowercase hex string)
 */
std::string hash_sha256(std::string input)
{
	unsigned char buf[32];
	int rc = mbedtls_sha256_ret((const unsigned char*)input.c_str(), input.size(), buf, 0);
	if (rc < 0) {
		LOG_WRN("mbedtls_sha256_ret failed: %d", rc);
		return "";
	}

	/* Convert byte buffer to hex string */
	char hash[32 * 2 + 1];
	for (int i = 0; i < 32; i++) {
		snprintf(&hash[i * 2], sizeof(hash) - (2 * i), "%02x", buf[i]);
	}
	hash[64] = '\0';

	return hash;
}

/**
 *  @brief	Converts an int-type to string.
 *  @author	Lau Lee Hong
 *  @param	i	(int)integer to be converted (std::string)integer
 *  @return	Integer of string format
 */
std::string int_to_string(int v)
{
	std::ostringstream oss;
	oss << v;
	return oss.str();
}