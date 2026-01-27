#include "datetime.h"
#include <furi.h>
#include <ctype.h>

#define TAG "DateTime"

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY    (SECONDS_PER_HOUR * 24)
#define MONTHS_COUNT       12
#define EPOCH_START_YEAR   1970

static const uint8_t datetime_days_per_month[2][MONTHS_COUNT] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

static const uint16_t datetime_days_per_year[] = {365, 366};

time_t datetime_datetime_to_timestamp(const DateTime* datetime) {
    furi_check(datetime);

    time_t timestamp = 0;
    uint8_t years = 0;
    uint8_t leap_years = 0;

    for(uint16_t y = EPOCH_START_YEAR; y < datetime->year; y++) {
        if(datetime_is_leap_year(y)) {
            leap_years++;
        } else {
            years++;
        }
    }

    timestamp += ((years * datetime_days_per_year[0]) + (leap_years * datetime_days_per_year[1])) *
                 SECONDS_PER_DAY;

    bool leap_year = datetime_is_leap_year(datetime->year);

    for(uint8_t m = 1; m < datetime->month; m++) {
        timestamp += datetime_get_days_per_month(leap_year, m) * SECONDS_PER_DAY;
    }

    timestamp += (datetime->dayofmonth - 1) * SECONDS_PER_DAY;
    timestamp += datetime->hour * SECONDS_PER_HOUR;
    timestamp += datetime->minute * SECONDS_PER_MINUTE;
    timestamp += datetime->second;

    return timestamp;
}

time_t datetime_datetime_to_timestamp_ms(const DateTimeMs* datetime) {
    time_t timestamp = datetime_datetime_to_timestamp(&datetime->dt);
    return 1000 * timestamp + datetime->millis;
}

DateTime datetime_timestamp_to_datetime(time_t timestamp) {
    time_t days = timestamp / SECONDS_PER_DAY;
    time_t seconds_in_day = timestamp % SECONDS_PER_DAY;

    uint16_t year = EPOCH_START_YEAR;

    while(days >= datetime_get_days_per_year(year)) {
        days -= datetime_get_days_per_year(year);
        year++;
    }

    uint8_t month = 1;
    while(days >=
          datetime_get_days_per_month(datetime_is_leap_year(year), month)) {
        days -=
            datetime_get_days_per_month(datetime_is_leap_year(year), month);
        month++;
    }

    uint16_t hour = seconds_in_day / SECONDS_PER_HOUR;
    uint16_t minute = (seconds_in_day % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
    uint16_t second = seconds_in_day % SECONDS_PER_MINUTE;

    return (DateTime){
        .date = utz_date_init(year, month, days + 1),
        .time = {
            .hour = hour,
            .minute = minute,
            .second = second
        }
    };
}

DateTimeMs datetime_timestamp_ms_to_datetime(time_t timestamp) {
    DateTime dt = datetime_timestamp_to_datetime(timestamp / 1000);
    return (DateTimeMs){
        .dt = dt,
        .millis = timestamp % 1000
    };
}

uint16_t datetime_get_days_per_year(uint16_t year) {
    return datetime_days_per_year[datetime_is_leap_year(year) ? 1 : 0];
}

bool datetime_is_leap_year(uint16_t year) {
    return (((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0);
}

uint8_t datetime_get_days_per_month(bool leap_year, uint8_t month) {
    return datetime_days_per_month[leap_year ? 1 : 0][month - 1];
}

void datetime_format_timestamp(const LocalTime *lt, char *buf) {
    char offset_sign = '+';
    uint8_t offset_h = 0;
    uint8_t offset_m = 0;
    // In offset_t only hours can be negative, minutes are always positive.
    // For example, offset of -1:15 (-75 min) will be encoded as {-2,45}.
    if(lt->offset.hours >= 0) {
        offset_h = lt->offset.hours;
        offset_m = lt->offset.minutes;
    } else {
        offset_sign = '-';
        if(lt->offset.minutes == 0) {
            offset_h = -lt->offset.hours;
        } else {
            offset_h = -lt->offset.hours - 1;
            offset_m = 60 - lt->offset.minutes;
        }
    }
    sprintf(buf, "%04hu-%02hhu-%02hhuT%02hhu:%02hhu:%02hhu%c%02hhu:%02hhu",
        lt->dt.year,
        lt->dt.month,
        lt->dt.dayofmonth,
        lt->dt.hour,
        lt->dt.minute,
        lt->dt.second,
        offset_sign,
        offset_h,
        offset_m);
}

static bool parse_int(const char **str, size_t width, unsigned int *result) {
    unsigned int r = 0;
    while(width > 0 && **str) {
        int c = **str;
        if(isdigit(c)) {
            r = r * 10 + c - '0';
        } else {
            return false;
        }
        width -= 1;
        *str += 1;
    }
    *result = r;
    return true;
}

/** Parse date in ISO 8601 format.
 * Accepted: YYYY-MM-DD or YYYYMMDD
 */
static bool parse_date(const char** str, utz_date_t *result) {
    unsigned int y = 0, m = 0, d = 0;
    bool hyphens = false;

    if(!parse_int(str, 4, &y)) {
        return false;
    }
    if(**str == '-') {
        hyphens = true;
        *str += 1;
    }
    if(!parse_int(str, 2, &m)) {
        return false;
    }
    if(**str == '-' && !hyphens) {
        return false;
    }
    *str += hyphens ? 1 : 0;
    if(!parse_int(str, 2, &d)) {
        return false;
    }
    return utz_date_init_checked(y, m, d, result);
}

/** Parse time in ISO 8601 format.
 * Accepted: Thh:mm:ss or Thhmmss
 */
static bool parse_time(const char** str, utz_time_t *result) {
    unsigned int h = 0, m = 0, s = 0;

    if(**str != 'T') {
        return false;
    }
    *str += 1;

    bool hyphens = false;

    if(!parse_int(str, 2, &h)) {
        return false;
    }
    if(**str == ':') {
        hyphens = true;
        *str += 1;
    }
    if(!parse_int(str, 2, &m)) {
        return false;
    }
    if(**str == ':' && !hyphens) {
        return false;
    }
    *str += hyphens ? 1 : 0;
    if(!parse_int(str, 2, &s)) {
        return false;
    }

    return utz_time_init_checked(h, m, s, result);
}

/** Parse timezone offset in ISO 8601 format.
 * Accepted: Z, ±hh:mm, ±hhmm, ±hh
 */
static bool parse_offset(const char* str, utz_offset_t *result) {
    bool negative = false;
    unsigned int h = 0, m = 0;

    if(*str == 'Z') {
        // UTC
        str += 1;
    } else {
        // sign
        if(*str == '-') {
            negative = true;
        } else if(*str != '+') {
            return false;
        }
        str += 1;

        // hours
        if(!parse_int(&str, 2, &h)) {
            return false;
        }
        if(*str == ':') {
            str += 1;
        }
        if(*str != 0) {
            // minutes
            if(!parse_int(&str, 2, &m)) {
                return false;
            }
        }
    }
    if(*str == 0) {
        *result = utz_offset_init(negative, h, m);
        return true;
    } else {
        return false;
    }
}

bool datetime_parse_timestamp(const char* str, DateTime *result) {
    utz_datetime_t dt;
    if(!parse_date(&str, &dt.date)) {
        return false;
    }
    if(!parse_time(&str, &dt.time)) {
        return false;
    }
    utz_offset_t offset;
    if(!parse_offset(str, &offset)) {
        return false;
    }

    *result = utz_udatetime_sub(&dt, &offset);

    return true;
}
