#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <utz/utz.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DATETIME_TIMESTAMP_STR_LEN 25

typedef utz_datetime_t DateTime;

typedef struct {
    DateTime dt;
    uint16_t millis; /**< Millisecond: 0-999 */
} DateTimeMs;

typedef struct {
    DateTime dt; ///< Local time
    utz_offset_t offset; ///< Offset from UTC
} LocalTime;

/** Convert DateTime to UNIX timestamp
 * 
 * @warning    Mind timezone when perform conversion
 *
 * @param      datetime  The datetime (UTC)
 *
 * @return     UNIX Timestamp in seconds from UNIX epoch start
 */
time_t datetime_datetime_to_timestamp(const DateTime* datetime);

/** Convert DateTime to UNIX timestamp in milliseconds
 *
 * @warning    Mind timezone when perform conversion
 *
 * @param      datetime  The datetime (UTC)
 *
 * @return     UNIX Timestamp in milliseconds from UNIX epoch start
 */
time_t datetime_datetime_to_timestamp_ms(const DateTimeMs* datetime);

/** Convert UNIX timestamp to DateTime
 *
 * @warning    Mind timezone when perform conversion
 *
 * @param[in]  timestamp  UNIX Timestamp in seconds from UNIX epoch start
 * @param[out] datetime   The datetime (UTC)
 */
DateTime datetime_timestamp_to_datetime(time_t timestamp);

/** Convert UNIX timestamp in milliseconds to DateTime
 *
 * @warning    Mind timezone when perform conversion
 *
 * @param[in]  timestamp  UNIX Timestamp in milliseconds from UNIX epoch start
 * @param[out] datetime   The datetime (UTC)
 */
DateTimeMs datetime_timestamp_ms_to_datetime(time_t timestamp);

/** Gets the number of days in the year according to the Gregorian calendar.
 *
 * @param year Input year.
 *
 * @return number of days in `year`.
 */
uint16_t datetime_get_days_per_year(uint16_t year);

/** Check if a year a leap year in the Gregorian calendar.
 *
 * @param year Input year.
 *
 * @return true if `year` is a leap year.
 */
bool datetime_is_leap_year(uint16_t year);

/** Get the number of days in the month.
 *
 * @param leap_year true to calculate based on leap years
 * @param month month to check, where 1 = January
 * @return the number of days in the month
 */
uint8_t datetime_get_days_per_month(bool leap_year, uint8_t month);

/** Format ISO 8601 timestamp: YYYY-MM-DDThh:mm:ss±hh:mm
 *
 * @param[in] lt local time.
 * @param[out] buf string buffer. Must hold at least (DATETIME_TIMESTAMP_STR_LEN+1) bytes.
 */
void datetime_format_timestamp(const LocalTime* lt, char* buf);

/** Parse ISO 8601 timestamp.
 *
 * Timestamp must be in following format:
 *  [DATE][TIME][ZONE]
 * The following DATE formats are supported:
 *  YYYY-MM-DD
 *  YYYYMMDD
 * The following TIME formats are supported:
 *  Thh:mm:ss
 *  Thhmmss
 * The following ZONE formats are supported:
 *  Z - UTC
 *  ±hh:mm
 *  ±hhmm
 *
 * @param[in] str timestamp string.
 * @param[out] result timestamp (UTC).
 * @return true on success.
 */
bool datetime_parse_timestamp(const char* str, DateTime* result);

#ifdef __cplusplus
}
#endif
