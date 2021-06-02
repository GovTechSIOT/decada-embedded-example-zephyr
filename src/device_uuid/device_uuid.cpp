#include "device_uuid.h"
#include <iomanip>
#include <sstream>

/**
 *  @brief  Returns a unique device ID
 *  @author Lee Tze Han
 *  @return Device UUID (24-character hex string)
 */
std::string read_device_uuid(void)
{
	std::stringstream ss;
	ss << std::hex;

	/* Format each byte to 8 char hexadecimal representation */
	ss << std::setw(8) << std::setfill('0');

	/*
         * CONFIG_UUID_ADDRESS contains the address pointing to the 96-bit factory flashed UID.
         * This macro should be configured for each target in their respective configuration files.
         */

	/* bits  0 - 31 (offset 0x00) */
	ss << *(uint32_t*)(CONFIG_UUID_ADDRESS);
	/* bits 32 - 63 (offset 0x04) */
	ss << *(uint32_t*)(CONFIG_UUID_ADDRESS + 0x04);
	/* bits 64 - 95 (offset 0x08) */
	ss << *(uint32_t*)(CONFIG_UUID_ADDRESS + 0x08);

	return ss.str();
}