#include <logging/log.h>
LOG_MODULE_REGISTER(https_request, LOG_LEVEL_DBG);

#include <net/tls_credentials.h>
#include "https_request.h"

#define HTTPS_SCHEME ("https://")
#define CA_CERT_TAG  1

HttpsRequest::HttpsRequest(std::string url, const char* ca_certs, int port) :
	HttpBase(url, port), ca_certs_(ca_certs)
{
	/* Sanity check for URL scheme */
	if (url.rfind(HTTPS_SCHEME, 0) == std::string::npos) {
		LOG_WRN("Expected HTTPS scheme");
	}
}

/**
 * @brief	Configure socket for HTTPS request
 * @author	Lee Tze Han
 * @param	addr	Pointer to socket address to be configured
 * @return	Success status
 * @note	Current implementation only supports IPv4
 */
bool HttpsRequest::setup_socket(sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port_);
	inet_pton(AF_INET, ipaddr_.c_str(), &addr->sin_addr);

	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (sock_ < 0) {
		LOG_WRN("Failed to create socket: %d", -errno);
		return false;
	}

	/* Configure socket options */
	sec_tag_t sec_tag_list[] = { CA_CERT_TAG };

	int rc = setsockopt(sock_, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			    sizeof(sec_tag_list));
	if (rc < 0) {
		LOG_WRN("Failed to set socket option TLS_SEC_TAG_LIST: %d",
			-errno);
		return false;
	}

	rc = setsockopt(sock_, SOL_TLS, TLS_HOSTNAME, hostname_.c_str(),
			hostname_.size());
	if (rc < 0) {
		LOG_WRN("Failed to set socket option TLS_HOSTNAME: %d", -errno);
		return false;
	}

	/* Set recognized CA certificates */
	rc = tls_credential_add(CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				ca_certs_, strlen(ca_certs_) + 1);
	if (rc < 0) {
		LOG_WRN("Failed to set CA certificates: %d", -errno);
		return false;
	}

	return true;
}