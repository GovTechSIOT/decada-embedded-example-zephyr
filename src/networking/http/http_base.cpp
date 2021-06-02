#include <logging/log.h>
LOG_MODULE_REGISTER(http_base, LOG_LEVEL_DBG);

#include "http_base.h"
#include "networking/dns/dns_lookup.h"

#define HTTP_REQUEST_PROTOCOL ("HTTP/1.1")
#define HTTP_TIMEOUT	      (5 * MSEC_PER_SEC)

void response_cb(struct http_response* recv_resp,
		 enum http_final_call final_data, void* user_data);

HttpBase::HttpBase(std::string url, int port) : port_(port)
{
	parse_url(url);

	/* Resolve hostname */
	DnsLookup dns_lookup(hostname_.c_str());
	ipaddr_ = dns_lookup.get_ipaddr();
}

HttpBase::~HttpBase(void)
{
	/* Close socket */
	if (sock_ >= 0) {
		close(sock_);
	}

	/* Free allocated memory */
	delete[] header_ptrs_;
}

/**
 * @brief	Adds a message header to the request
 * @author	Lee Tze Han
 * @param       header_name	Message header name
 * @param       header_value	Message header value
 */
void HttpBase::add_header(std::string header_name, std::string header_value)
{
	/* Form header line (c.f. RFC2616 4.2) */
	std::string header_line = header_name + ":" + header_value + HTTP_CRLF;
	headers_.push_back(header_line);
}

/**
 * @brief	Sends a HTTP(S) request to the specified endpoint
 * @author	Lee Tze Han
 * @param       method  HTTP method (GET/POST)
 * @param       payload The payload to be included in the request
 * @return	Success status
 * @note	This method blocks until a HTTP response is received
 */
bool HttpBase::send_request(http_method method, std::string payload)
{
	struct http_request req;
	memset(&req, 0, sizeof(req));

	req.protocol = HTTP_REQUEST_PROTOCOL;
	req.method = method;
	req.url = endpoint_.c_str();
	req.host = host_.c_str();
	req.response = response_cb;
	req.recv_buf = recv_buf_;
	req.recv_buf_len = sizeof(recv_buf_);

	/* Set headers if present */
	if (headers_.size() > 0) {
		header_ptrs_ = new const char*[headers_.size() + 1];

		size_t i;
		for (i = 0; i < headers_.size(); i++) {
			header_ptrs_[i] = headers_[i].c_str();
		}
		/* List needs to be NULL-terminated */
		header_ptrs_[i] = NULL;
		req.optional_headers = header_ptrs_;
	}

	/* Set message body if present */
	if (payload.length() > 0) {
		req.payload = payload.c_str();
		req.payload_len = payload.length();
	}

	/* Socket setup and connection */
	if (!connect_socket()) {
		return false;
	}

	/* 
	 * Pass calling HttpRequest context through user_data 
	 * Note: This call blocks until a response is received
	 */
	int rc = http_client_req(sock_, &req, HTTP_TIMEOUT, &resp_);
	if (rc < 0) {
		LOG_WRN("Failed to send HTTP request: %d", rc);
		return false;
	}

	return true;
}

/**
 * @brief	Returns parsed HTTP response body
 * @author	Lee Tze Han
 * @return	Response body string
 */
std::string HttpBase::get_response_body(void)
{
	return resp_.get_body();
}

/**
 * @brief	Parses URL into hostname and API endpoint
 * @author	Lee Tze Han
 * @param       url     URL string to be parsed
 */
void HttpBase::parse_url(std::string url)
{
	/* Find end of scheme */
	int scheme_len = url.find("//") + 3;

	int idx = url.find_first_of('/', scheme_len);
	if (idx != -1) {
		/* Split URL into hostname and endpoint */
		hostname_ = url.substr(scheme_len - 1, idx - scheme_len + 1);
		endpoint_ = url.substr(idx);
	}
	else {
		hostname_ = url.substr(scheme_len - 1);
		endpoint_ = "/";
	}

	/* Hostname may contain port */
	idx = hostname_.find_first_of(':');
	if (idx != -1) {
		/* Use port number from URL instead */
		int port = stol(hostname_.substr(idx + 1));
		LOG_INF("Using port %d parsed from URL", port);

		port_ = port;
		hostname_ = hostname_.substr(0, idx);
	}

	host_ = hostname_ + ":" + std::to_string(port_);

	LOG_DBG("Parsed hostname: '%s'", hostname_.c_str());
	LOG_DBG("Parsed endpoint: '%s'", endpoint_.c_str());
}

/**
 * @brief	Establish connection on created socket
 * @author	Lee Tze Han
 * @return      Success status
 */
bool HttpBase::connect_socket(void)
{
	/* Configure socket according to scheme */
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	if (!setup_socket(&addr)) {
		LOG_WRN("Failed to setup socket");
		return false;
	};

	/* Connection using socket */
	int rc = connect(sock_, (struct sockaddr*)&addr, sizeof(sockaddr_in));
	if (rc < 0) {
		LOG_WRN("Failed to connect to %s: %d", ipaddr_.c_str(), -errno);
		return false;
	}

	return true;
}

/**
 * @brief	Callback for response to HTTP request
 * @author	Lee Tze Han
 * @param       recv_resp       HTTP response information
 * @param       final_data      Indicates if data buffer contains all the data or there is more to come
 * @param       user_data       User data specified in http_client_req
 */
void response_cb(struct http_response* recv_resp,
		 enum http_final_call final_data, void* user_data)
{
	HttpResponse* resp = (HttpResponse*)user_data;

	/* Copy response into buffer */
	resp->append_body(recv_resp);
}