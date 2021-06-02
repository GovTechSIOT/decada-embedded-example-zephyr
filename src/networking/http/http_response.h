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
	int idx_ = 0;
};

#endif // _HTTP_RESPONSE_H_