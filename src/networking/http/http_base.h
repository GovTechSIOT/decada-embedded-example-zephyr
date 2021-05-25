#ifndef _HTTP_BASE_H_
#define _HTTP_BASE_H_

#include <vector>
#include <net/socket.h>
#include "http_response.h"

class HttpBase
{
public:
	HttpBase(std::string url, int port);
	virtual ~HttpBase(void);

	void add_header(std::string header_name, std::string header_value);
	bool send_request(enum http_method method, std::string payload = "");

	std::string get_response_body(void);

protected:
	std::string ipaddr_;
	std::string hostname_;
	int port_;
	int sock_;

private:
	void parse_url(std::string url);

	virtual bool setup_socket(sockaddr_in* addr) = 0;
	bool connect_socket(void);

	uint8_t recv_buf_[512];
	HttpResponse resp_;
	std::vector<std::string> headers_;
	const char** header_ptrs_;

	std::string endpoint_;
};

#endif // _HTTP_BASE_H_