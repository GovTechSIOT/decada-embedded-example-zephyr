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
#ifndef _HTTPS_REQUEST_H_
#define _HTTPS_REQUEST_H_

#include <string>
#include "http_base.h"

#define HTTPS_PORT 443

class HttpsRequest : public HttpBase
{
public:
	HttpsRequest(std::string url, int port = HTTPS_PORT);

private:
	bool setup_socket(sockaddr_in* addr) override;
};

#endif // _HTTPS_REQUEST_H_