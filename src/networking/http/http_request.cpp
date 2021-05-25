#include <logging/log.h>
LOG_MODULE_REGISTER(http_request, LOG_LEVEL_DBG);

#include <net/http_client.h>
#include "http_request.h"

#define HTTP_SCHEME ("http://")

HttpRequest::HttpRequest(std::string url, int port) : HttpBase(url, port)
{
	/* Sanity check for URL scheme */
	if (url.rfind(HTTP_SCHEME, 0) == std::string::npos) {
		LOG_WRN("Expected HTTP scheme");
	}
}

/**
 * @brief	Configure socket for HTTP request
 * @author	Lee Tze Han
 * @param	addr 	Pointer to socket address to be configured
 * @return	Success status
 * @note        Current implementation only supports IPv4
 */
bool HttpRequest::setup_socket(sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port_);
	inet_pton(AF_INET, ipaddr_.c_str(), &addr->sin_addr);

	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_ < 0) {
		LOG_WRN("Failed to create socket: %d", -errno);
		return false;
	}

	return true;
}
