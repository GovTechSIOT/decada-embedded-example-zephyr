#ifndef _HTTPS_REQUEST_H_
#define _HTTPS_REQUEST_H_

#include <string>
#include "http_base.h"

#define HTTPS_PORT 443

class HttpsRequest : public HttpBase
{
public:
	HttpsRequest(std::string url, const char* ca_certs,
		     int port = HTTPS_PORT);

private:
	bool setup_socket(sockaddr_in* addr) override;

	const char* ca_certs_;
};

#endif // _HTTPS_REQUEST_H_