#include <logging/log.h>
LOG_MODULE_REGISTER(http_response, LOG_LEVEL_DBG);

#include "http_response.h"

/**
 * @brief	Appends message body from http_response
 * @author	Lee Tze Han
 * @param	resp    http_response object
 */
void HttpResponse::append_body(http_response* resp)
{
	/* 
         * Buffer in http_response is in the message body only if resp->body_found
         * is set to 1 (c.f. on_body() in http_client.c)
         */
	if (resp->body_found) {
		/*
                 * Response is received in chunks into resp->recv_buf, but the message
                 * body may not start at resp->recv_buf[0] due to headers. If resp->body_start
                 * is NULL, the entire buffer can be copied over. Otherwise, copy starting from
                 * resp->body_start. The buffer is not guaranteed to be NULL-terminated.
                 */

		if (resp->body_start) {
			int offset = (resp->body_start - resp->recv_buf) /
				     sizeof(resp->recv_buf[0]);

			body_.append((char*)resp->body_start,
				     resp->data_len - offset);
		}
		else {
			body_.append((char*)resp->recv_buf, resp->data_len);
		}
	}
}

std::string HttpResponse::get_body(void)
{
	return body_;
}