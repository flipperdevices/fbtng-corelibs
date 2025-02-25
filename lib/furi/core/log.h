/**
 * @file log.h
 * Furi Logging system
 */
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriLogLevelDefault = 0,
    FuriLogLevelNone = 1,
    FuriLogLevelError = 2,
    FuriLogLevelWarn = 3,
    FuriLogLevelInfo = 4,
    FuriLogLevelDebug = 5,
    FuriLogLevelTrace = 6,
} FuriLogLevel;

#define _FURI_LOG_CLR(clr)  "\033[0;" clr "m"
#define _FURI_LOG_CLR_RESET "\033[0m"

#define _FURI_LOG_CLR_BLACK  "30"
#define _FURI_LOG_CLR_RED    "31"
#define _FURI_LOG_CLR_GREEN  "32"
#define _FURI_LOG_CLR_BROWN  "33"
#define _FURI_LOG_CLR_BLUE   "34"
#define _FURI_LOG_CLR_PURPLE "35"

#define _FURI_LOG_CLR_E _FURI_LOG_CLR(_FURI_LOG_CLR_RED)
#define _FURI_LOG_CLR_W _FURI_LOG_CLR(_FURI_LOG_CLR_BROWN)
#define _FURI_LOG_CLR_I _FURI_LOG_CLR(_FURI_LOG_CLR_GREEN)
#define _FURI_LOG_CLR_D _FURI_LOG_CLR(_FURI_LOG_CLR_BLUE)
#define _FURI_LOG_CLR_T _FURI_LOG_CLR(_FURI_LOG_CLR_PURPLE)

typedef void (*FuriLogHandlerCallback)(const uint8_t* data, size_t size, void* context);

typedef struct {
    FuriLogHandlerCallback callback;
    void* context;
} FuriLogHandler;

/** Initialize logging */
void furi_log_init(void);

/** Add log TX callback
 *
 * @param[in]  handler  The callback and its context
 *
 * @return     true on success, false otherwise
 */
bool furi_log_add_handler(FuriLogHandler handler);

/** Remove log TX callback
 *
 * @param[in]  handler  The callback and its context
 *
 * @return     true on success, false otherwise
 */
bool furi_log_remove_handler(FuriLogHandler handler);

/** Transmit data through log IO callbacks
 *
 * @param[in]  data  The data
 * @param[in]  size  The size
 */
void furi_log_tx(const uint8_t* data, size_t size);

/** Transmit data through log IO callbacks
 *
 * @param[in]  data  The data, null-terminated C-string
 */
void furi_log_puts(const char* data);

/** Transmit data as uint32 through log IO callbacks
 *
 * @param[in]  data  The data
 */
void furi_log_putu32(uint32_t data);

/** Transmit data as hex through log IO callbacks
 *
 * @param[in]  data  The data
 */
void furi_log_puthex32(uint32_t data);

/** Print log record
 * 
 * @param level 
 * @param tag 
 * @param format 
 * @param ... 
 */
void furi_log_print_format(FuriLogLevel level, const char* tag, const char* format, ...)
    _ATTRIBUTE((__format__(__printf__, 3, 4)));

/** Print log record
 * 
 * @param level 
 * @param format 
 * @param ... 
 */
void furi_log_print_raw_format(FuriLogLevel level, const char* format, ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/** Set log level
 *
 * @param[in]  level  The level
 */
void furi_log_set_level(FuriLogLevel level);

/** Get log level
 *
 * @return     The furi log level.
 */
FuriLogLevel furi_log_get_level(void);

/** Log level to string
 *
 * @param[in]  level  The level
 * @param[out] str    String representation of the level
 *
 * @return     True if success, False otherwise
 */
bool furi_log_level_to_string(FuriLogLevel level, const char** str);

/** Log level from string
 *
 * @param[in]  str    The string
 * @param[out] level  The level
 * 
 * @return     True if success, False otherwise
 */
bool furi_log_level_from_string(const char* str, FuriLogLevel* level);

/** Log methods
 *
 * @param      tag     The application tag
 * @param      format  The format
 * @param      ...     VA Args
 */
#define FURI_LOG_E(tag, format, ...) \
    furi_log_print_format(FuriLogLevelError, tag, format, ##__VA_ARGS__)
#define FURI_LOG_W(tag, format, ...) \
    furi_log_print_format(FuriLogLevelWarn, tag, format, ##__VA_ARGS__)
#define FURI_LOG_I(tag, format, ...) \
    furi_log_print_format(FuriLogLevelInfo, tag, format, ##__VA_ARGS__)
#define FURI_LOG_D(tag, format, ...) \
    furi_log_print_format(FuriLogLevelDebug, tag, format, ##__VA_ARGS__)
#define FURI_LOG_T(tag, format, ...) \
    furi_log_print_format(FuriLogLevelTrace, tag, format, ##__VA_ARGS__)

/** Log methods
 *
 * @param      format  The raw format 
 * @param      ...     VA Args
 */
#define FURI_LOG_RAW_E(format, ...) \
    furi_log_print_raw_format(FuriLogLevelError, format, ##__VA_ARGS__)
#define FURI_LOG_RAW_W(format, ...) \
    furi_log_print_raw_format(FuriLogLevelWarn, format, ##__VA_ARGS__)
#define FURI_LOG_RAW_I(format, ...) \
    furi_log_print_raw_format(FuriLogLevelInfo, format, ##__VA_ARGS__)
#define FURI_LOG_RAW_D(format, ...) \
    furi_log_print_raw_format(FuriLogLevelDebug, format, ##__VA_ARGS__)
#define FURI_LOG_RAW_T(format, ...) \
    furi_log_print_raw_format(FuriLogLevelTrace, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
