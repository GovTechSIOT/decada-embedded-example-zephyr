#ifndef _TIME_ENGINE_H_
#define _TIME_ENGINE_H_

#include <string>
#include <zephyr.h>

class TimeEngine
{
public:
	int64_t get_timestamp(void);
	std::string get_timestamp_ms_str(void);

private:
	struct tm get_rtc_datetime(int64_t* timestamp_ptr);

protected:
	void update_rtc_time(uint64_t timestamp);
};

#endif // _TIME_ENGINE_H_