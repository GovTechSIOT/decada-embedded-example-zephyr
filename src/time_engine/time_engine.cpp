#include <logging/log.h>
LOG_MODULE_REGISTER(time_engine, LOG_LEVEL_DBG);

#include <time.h>
#include "networking/dns/dns_lookup.h"
#include "time_engine.h"
#include "user_config.h"

/**
 * @brief	Get Unix epoch timestamp
 * @author	Lee Tze Han
 * @return	Current timestamp
 */
int64_t TimeEngine::get_timestamp(void)
{
	int64_t timestamp = -1;
	get_rtc_datetime(&timestamp);

	return timestamp;
}

/**
 * @brief	Get Unix epoch timestamp in milliseconds
 * @author	Lee Tze Han
 * @return	Current timestamp
 * @note	This does not provide millisecond resolution but pads the timestamp for convenience
 */
int64_t TimeEngine::get_timestamp_ms(void)
{
	return (get_timestamp() * 1000);
}

#if defined(CONFIG_BOARD_MANUCA_DK_REVB)
/* Current implementation for SOC STM32F767ZI but should be compatible with STM32 SOCs */
#include <stm32_ll_rtc.h>

/**
 * @brief	Get datetime from RTC
 * @author	Lee Tze Han
 * @param	timestamp_ptr	Pointer to store Unix epoch timestamp. Set to NULL if unneeded
 * @return	tm struct containing time in broken-down representation
 */
struct tm TimeEngine::get_rtc_datetime(int64_t* timestamp_ptr)
{
	struct tm time = {
		/*
		 * tm_sec:	Number of seconds after the minute (0-59), with (60) reserved for leap seconds
		 * RTC second:  Similarly defined, but without (60)
		 */
		.tm_sec = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC)),

		/*
		 * tm_min:	Number of minutes after the hour (0-59)
		 * RTC minute:  Same definition
		 */
		.tm_min = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC)),

		/*
		 * tm_hour:	Number of hours after midnight (0-23)
		 * RTC hour: 	Similarly defined if RTC time format is set to LL_RTC_TS_TIME_FORMAT_PM
		 * 		Otherwise, returns a value in the range (1-12)
		 */
		.tm_hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC)),

		/*
		 * tm_mday:	Day of the month (1-31)
		 * RTC day:	Same definition
		 */
		.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC)),

		/*
		 * tm_mon:	Number of months since January (0-11)
		 * RTC month:	Months start from January (1) to December (12)
		 */
		.tm_mon =
			__LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC)) - 1,

		/*
		 * tm_year:	Number of years since 1900
		 * RTC year:	Last two digits of year
		 * 
		 * The conversion tm_year = (RTC year)+100 holds for years 2000 to 2099
		 */
		.tm_year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC)) +
			   100,

		/* Value is ignored */
		.tm_wday = 0,

		/* Value is ignored */
		.tm_yday = 0,

		/* Daylight savings is not in effect */
		.tm_isdst = 0
	};

	/* 
	 * Calculate tm_wday and tm_yday, and convert to time_t 
	 *
	 * NB:	Zephyr provides timeutil_timegm() and timeutil_timegm64() transformations but these
	 * 	do not update the tm_wday and tm_yday fields unlike mktime(). However, mktime() relies on
	 * 	timezone information. For Zephyr v2.5.0, this fallbacks to UTC, which is desired.
	 */

	int64_t timestamp = mktime(&time);
	if (timestamp == -1) {
		LOG_WRN("Failure in timeutil_timegm64");
	}

	/* Store timestamp in given pointer */
	if (timestamp_ptr) {
		*timestamp_ptr = timestamp;
	}

	return time;
}

/**
 * @brief	Set the current timestamp of the RTC
 * @author	Lee Tze Han
 * @param	timestamp    Unix epoch timestamp (seconds since 00:00:00 UTC 1st Jan 1970)
 * @note        Updating the RTC using this function loses the millisecond resolution
 */
void TimeEngine::update_rtc_time(uint64_t timestamp)
{
	/* Convert POSIX epoch timestamp to calendar time */
	time_t time = timestamp;
	tm info;
	if (!gmtime_r(&time, &info)) {
		/* Null pointer indicates failure in conversion */
		LOG_WRN("Failed to convert POSIX epoch timestamp - RTC is not updated");
		return;
	};

	LL_RTC_DateTypeDef rtc_date = {
		/*
                 * WeekDay:	Day of the week starting from Monday (1) to Sunday (7) 
		 * tm_wday:	Similarly defined, but starting from Sunday (0) to Saturday (6)	
                 */
		.WeekDay = (uint8_t)(info.tm_wday == 0 ? 7 : info.tm_wday),

		/*
		 * Month:	Months start from January (1) to December (12)
		 * tm_mon:	Number of months since January (0-11)
		 */
		.Month = (uint8_t)(info.tm_mon + 1),

		/*
		 * Day:		Day of the month (1-31)
		 * tm_mday:	Same definition
		 */
		.Day = (uint8_t)info.tm_mday,

		/*
		 * Year:	Last two digits of year
		 * tm_year:	Number of years since 1900
		 * 
		 * The conversion Year = tm_year-100 holds for years 2000 to 2099
		 */
		.Year = (uint8_t)(info.tm_year - 100),
	};

	LL_RTC_TimeTypeDef rtc_time = {
		/* 
                 * Using the expected time format of 24-hour time 
		 */
		.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24,

		/*
		 * Hours: 	Number of hours after midnight (0-23)
		 * tm_hour:	Same definition
		 */
		.Hours = (uint8_t)info.tm_hour,

		/*
		 * Minutes:	Number of minutes after the hour (0-59)
		 * tm_min:  	Same definition
		 */
		.Minutes = (uint8_t)info.tm_min,

		/*
		 * Seconds:  	Number of seconds after the minute (0-59)
		 * tm_sec:	Similarly defined, with (60) reserved for leap seconds
		 * 		Calls to gmtime_r will not return a leap second
		 */
		.Seconds = (uint8_t)info.tm_sec
	};

	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_date) != 0 ||
	    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_time) != 0)
	{
		LOG_WRN("Failed to update RTC");
	}
	else {
		char buffer[256];
		strftime(buffer, sizeof(buffer), "%c", &info);
		LOG_INF("RTC Updated: %s", buffer);
	}
}

#else
#error "TimeEngine requires RTC driver dependent code that may not be compatible with the current implementation"
#endif