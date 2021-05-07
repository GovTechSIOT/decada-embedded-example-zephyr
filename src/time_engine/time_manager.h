#ifndef _TIME_MANAGER_H
#define _TIME_MANAGER_H

#include "time_engine/time_engine.h"

/*
 * TimeManager should only be included in one thread responsible
 * for periodically syncing the RTC with the SNTP server.
 * 
 * The TimeEngine class is otherwise sufficient to handle datetime operations.
 */

class TimeManager : public TimeEngine
{
public:
	bool sync_sntp_rtc(void);
};

#endif // _TIME_MANAGER_H