#include <logging/log.h>
LOG_MODULE_REGISTER(persist_store, LOG_LEVEL_DBG);

#include "persist_store.h"
#include <device.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>
#include "conversions/conversions.h"

static struct nvs_fs fs;

#define STORAGE_NODE DT_NODE_BY_FIXED_PARTITION_LABEL(storage)
#define FLASH_NODE DT_MTD_FROM_FIXED_PARTITION(STORAGE_NODE)

typedef const int KeyName;

/* List of id key used in NVS */
namespace PersistKey
{
        KeyName SW_VER =                                        1;
        KeyName SSL_CLIENT_CERTIFICATE =                        2;
        KeyName SSL_CLIENT_CERTIFICATE_SERIAL_NUMBER =          3;
        KeyName SSL_PRIVATE_KEY =                               4; 
}

// Forward declarations of helper functions
void write_key(KeyName key, const std::string& val);
void write_key(KeyName key, const int val);      // override for int
std::string read_key(KeyName key);

////////////////////////////////////////////////////////////////////
//
//   Public functions for Initializing persistent storage
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Initialize persistent storage.
 *  @author Lau Lee Hong
 */
void init_persist_storage(void)
{
        int rc = 0;
	struct flash_pages_info info;
	const struct device *flash_dev;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at FLASH_AREA_OFFSET(storage)
	 */
	flash_dev = DEVICE_DT_GET(FLASH_NODE);
	if (!device_is_ready(flash_dev)) {
		LOG_DBG("Flash device %s is not ready\n", flash_dev->name);
		return;
	}
	fs.offset = FLASH_AREA_OFFSET(storage);
	rc = flash_get_page_info_by_offs(flash_dev, fs.offset, &info);
	if (rc) {
		LOG_DBG("Unable to get page info\n");
		return;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_init(&fs, flash_dev->name);
	if (rc) {
		LOG_WRN("Flash Init failed\n");
		return;
	}

        LOG_INF("Persistent Storage initialized");
}

////////////////////////////////////////////////////////////////////
//
//   Public functions for writing to persistent storage
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Writes current os version to flash memory.
 *  @author Lau Lee Hong
 *  @param  sw_ver Operating systems's version to be stored
 */
void write_sw_ver(const std::string sw_ver)
{
        write_key(
                PersistKey::SW_VER,
                sw_ver
        );

        return;
}

/**
 *  @brief  Writes client certificate to flash memory.
 *  @author Lau Lee Hong
 *  @param  cert ssl client cert received from server
 */
void write_client_certificate(const std::string cert)
{
        write_key(
                PersistKey::SSL_CLIENT_CERTIFICATE,
                cert
        );
        
        return;
}

/**
 *  @brief  Writes client certificate serial number to flash memory.
 *  @author Lau Lee Hong
 *  @param  cert_sn certificate serial number received from decada
 */
void write_client_certificate_serial_number(const std::string cert_sn)
{
        write_key(
                PersistKey::SSL_CLIENT_CERTIFICATE_SERIAL_NUMBER,
                cert_sn
        );

        return;
}

/**
 *  @brief  Writes client private key (in PEM format) to flash memory.
 *  @author Lau Lee Hong
 *  @param  private_key client private key in PEM format
 */
void write_client_private_key(const std::string private_key)
{
        write_key(
                PersistKey::SSL_PRIVATE_KEY,
                private_key
        );

        return;
}

////////////////////////////////////////////////////////////////////
//
//   Public functions for reading from persistent storage
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Reads a previously stored software version from flash memory.
 *  @author Lau Lee hong
 *  @return software version in persistent storage
 */
std::string read_sw_ver(void)
{
        std::string sw_ver = read_key(PersistKey::SW_VER);
        
        return sw_ver;
}

/**
 *  @brief  Reads the client certificate from flash memory.
 *  @author Lau Lee Hong
 *  @return client certificate in PEM format
 */
std::string read_client_certificate(void)
{
        std::string client_cert = read_key(PersistKey::SSL_CLIENT_CERTIFICATE);
        
        return client_cert;
}

/**
 *  @brief  Reads the client certificate serial number from flash memory.
 *  @author Lau Lee Hong
 *  @return client certificate serial number
 */
std::string read_client_certificate_serial_number(void)
{
        std::string sn = read_key(PersistKey::SSL_CLIENT_CERTIFICATE_SERIAL_NUMBER);
        
        return sn;
}

/**
 *  @brief  Reads the client private key (PEM) from flash memory.
 *  @author Lau Lee Hong
 *  @return client private key in PEM format
 */
std::string read_client_private_key(void)
{
        std::string priv_key = read_key(PersistKey::SSL_PRIVATE_KEY);
        
        return priv_key;
}

////////////////////////////////////////////////////////////////////
//
//   Helper functions for interfacing with global NVS API
//
////////////////////////////////////////////////////////////////////

/**
 *  @brief  Writes a key-value pair to flash memory.
 *  @author Lau Lee Hog
 *  @param  key Id value in NVS
 *  @param  val String value to be stored under key
 */
void write_key(KeyName key, const std::string& val)
{
        LOG_DBG("Writing key %d with value %s", key, val.c_str());
        
        const char* val_cstr = val.c_str();

        int rc = nvs_write(&fs, key, val_cstr, strlen(val_cstr));
        if (rc < 0) {
                LOG_WRN("Failed to set key (returned %d)", rc);
        }
    
        LOG_DBG("Write OK");

        return;
}

/**
 *  @brief  Integer overload for WriteKey.
 *  @author Lau Lee Hong
 *  @param  key Id value in NVS
 *  @param  val Integer value to be stored under key
 */
void write_key(KeyName key, const int val)
{
    write_key(key, int_to_string(val));
}

/**
 *  @brief  Get value of key-value pair from flash memory.
 *  @author Lau Lee Hong
 *  @param  key NVS id
 *  @return val C++ string stored under key
 */
std::string read_key(KeyName key)
{
        LOG_DBG("Reading key %d", key);

        /* buffer is one char larger to accomodate null terminator */
        const int readout_buffer_size = 2048;
        char* buffer[readout_buffer_size+1];
        memset(buffer, 0, readout_buffer_size + 1);

        int rc = nvs_read(&fs, key, &buffer, readout_buffer_size);
        if (rc > 0) {
                LOG_DBG("Id: %d, Data: %s", key, std::string((const char*) buffer).c_str());
        } else {
                LOG_WRN("Failed to read key (returned %d)", rc);
                return std::string();
        }
        
        return std::string((const char*) buffer);
}