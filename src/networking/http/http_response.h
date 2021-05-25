#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <string>
#include <net/http_client.h>

class HttpResponse
{
public:
	std::string get_body(void);
	void append_body(http_response* resp);

private:
	std::string body_ = "";
};

#endif // _HTTP_RESPONSE_H_