#ifndef _TIME_ENGINE_H_
#define _TIME_ENGINE_H_

#include <zephyr.h>

class TimeEngine
{
public:
	int64_t get_timestamp(void);
	int64_t get_timestamp_ms(void);

private:
	struct tm get_rtc_datetime(int64_t* timestamp_ptr);

protected:
	void update_rtc_time(uint64_t timestamp);
};

#endif // _TIME_ENGINE_H_