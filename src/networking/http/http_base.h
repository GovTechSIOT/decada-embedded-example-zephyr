/*******************************************************************************************************
 * Copyright (c) 2021 Government Technology Agency of Singapore (GovTech)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 * See the License for the specific language governing permissions and limitations under the License.
 *******************************************************************************************************/
#ifndef _HTTP_BASE_H_
#define _HTTP_BASE_H_

#include <vector>
#include <net/socket.h>
#include "http_response.h"

class HttpBase
{
public:
	HttpBase(const std::string& url, int port);
	virtual ~HttpBase(void);

	void add_header(const std::string& header_name, const std::string& header_value);
	bool send_request(enum http_method method, std::string payload = "");

	std::string get_response_body(void) const;

protected:
	std::string ipaddr_;
	std::string hostname_;
	std::string host_;
	int port_;
	int sock_;

private:
	void parse_url(const std::string& url);

	virtual bool setup_socket(sockaddr_in* addr) = 0;
	bool connect_socket(void);

	uint8_t recv_buf_[512];
	HttpResponse resp_;
	std::vector<std::string> headers_;
	const char** header_ptrs_;

	std::string endpoint_;
};

#endif // _HTTP_BASE_H_