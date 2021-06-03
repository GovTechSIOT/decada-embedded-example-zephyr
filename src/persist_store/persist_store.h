#ifndef _PERSIST_STORE_H_
#define _PERSIST_STORE_H_

#include <zephyr.h>
#include <string>

void init_persist_storage(void);

void write_sw_ver(const std::string sw_ver);
void write_client_certificate(const std::string cert);
void write_client_certificate_serial_number(const std::string cert_sn);
void write_client_private_key(const std::string private_key);

std::string read_sw_ver(void);
std::string read_client_certificate(void);
std::string read_client_certificate_serial_number(void);
std::string read_client_private_key(void);

#endif // _PERSIST_STORE_H_