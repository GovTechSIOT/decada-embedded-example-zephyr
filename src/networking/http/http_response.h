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
#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <string>
#include <net/http_client.h>

class HttpResponse
{
public:
	std::string get_body(void) const;
	void append_body(http_response* resp);

private:
	std::string body_ = "";
	int idx_ = 0;
};

#endif // _HTTP_RESPONSE_H_