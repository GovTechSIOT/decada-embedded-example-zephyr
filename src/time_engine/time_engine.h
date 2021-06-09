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
#ifndef _TIME_ENGINE_H_
#define _TIME_ENGINE_H_

#include <string>
#include <zephyr.h>

class TimeEngine
{
public:
	int64_t get_timestamp(void);
	std::string get_timestamp_s_str(void);
	std::string get_timestamp_ms_str(void);

private:
	struct tm get_rtc_datetime(int64_t* timestamp_ptr);

protected:
	void update_rtc_time(uint64_t timestamp);
};

#endif // _TIME_ENGINE_H_