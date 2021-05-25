#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <string>
#include "http_base.h"

#define HTTP_PORT 80

class HttpRequest : public HttpBase
{
public:
	HttpRequest(std::string addr, int port = HTTP_PORT);

private:
	bool setup_socket(sockaddr_in* addr) override;
};

#endif // _HTTP_REQUEST_H_