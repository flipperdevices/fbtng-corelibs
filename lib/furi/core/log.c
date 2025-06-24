#include "log.h"
#include "check.h"
#include "mutex.h"
#include <furi_hal.h>
#include <m-list.h>

LIST_DEF(FuriLogHandlersList, FuriLogHandler, M_POD_OPLIST)

#define FURI_LOG_LEVEL_DEFAULT FuriLogLevelInfo

typedef struct {
    FuriLogLevel log_level;
    FuriMutex* mutex;
    FuriLogHandlersList_t tx_handlers;
} FuriLogParams;

static FuriLogParams furi_log = {0};

typedef struct {
    const char* str;
    FuriLogLevel level;
} FuriLogLevelDescription;

static const FuriLogLevelDescription FURI_LOG_LEVEL_DESCRIPTIONS[] = {
    {"default", FuriLogLevelDefault},
    {"none", FuriLogLevelNone},
    {"error", FuriLogLevelError},
    {"warn", FuriLogLevelWarn},
    {"info", FuriLogLevelInfo},
    {"debug", FuriLogLevelDebug},
    {"trace", FuriLogLevelTrace},
};

void furi_log_init(void) {
    // Set default logging parameters
    furi_log.log_level = FURI_LOG_LEVEL_DEFAULT;
    furi_log.mutex = furi_mutex_alloc(FuriMutexTypeRecursive);
    FuriLogHandlersList_init(furi_log.tx_handlers);
}

bool furi_log_add_handler(FuriLogHandler handler) {
    furi_check(handler.callback);

    bool ret = true;

    furi_check(furi_mutex_acquire(furi_log.mutex, FuriWaitForever) == FuriStatusOk);

    FuriLogHandlersList_it_t it;
    FuriLogHandlersList_it(it, furi_log.tx_handlers);
    while(!FuriLogHandlersList_end_p(it)) {
        if(memcmp(FuriLogHandlersList_ref(it), &handler, sizeof(FuriLogHandler)) == 0) {
            ret = false;
        } else {
            FuriLogHandlersList_next(it);
        }
    }

    if(ret) {
        FuriLogHandlersList_push_back(furi_log.tx_handlers, handler);
    }

    furi_mutex_release(furi_log.mutex);

    return ret;
}

bool furi_log_remove_handler(FuriLogHandler handler) {
    bool ret = false;

    furi_check(furi_mutex_acquire(furi_log.mutex, FuriWaitForever) == FuriStatusOk);

    FuriLogHandlersList_it_t it;
    FuriLogHandlersList_it(it, furi_log.tx_handlers);
    while(!FuriLogHandlersList_end_p(it)) {
        if(memcmp(FuriLogHandlersList_ref(it), &handler, sizeof(FuriLogHandler)) == 0) {
            FuriLogHandlersList_remove(furi_log.tx_handlers, it);
            ret = true;
        } else {
            FuriLogHandlersList_next(it);
        }
    }

    furi_mutex_release(furi_log.mutex);

    return ret;
}

void furi_log_tx(const uint8_t* data, size_t size) {
    if(!FURI_IS_ISR()) {
        furi_check(furi_mutex_acquire(furi_log.mutex, FuriWaitForever) == FuriStatusOk);
    } else {
        if(furi_mutex_get_owner(furi_log.mutex)) return;
    }

    FuriLogHandlersList_it_t it;
    FuriLogHandlersList_it(it, furi_log.tx_handlers);
    while(!FuriLogHandlersList_end_p(it)) {
        FuriLogHandlersList_ref(it)->callback(data, size, FuriLogHandlersList_ref(it)->context);
        FuriLogHandlersList_next(it);
    }

    if(!FURI_IS_ISR()) furi_mutex_release(furi_log.mutex);
}

void furi_log_puts(const char* data) {
    furi_check(data);
    furi_log_tx((const uint8_t*)data, strlen(data));
}

void furi_log_putu32(uint32_t data) {
    char tmp_str[] = "4294967295";
    itoa(data, tmp_str, 10);
    furi_log_puts(tmp_str);
}

void furi_log_puthex32(uint32_t data) {
    char tmp_str[] = "0xFFFFFFFF";
    itoa(data, tmp_str, 16);
    furi_log_puts(tmp_str);
}

void furi_log_print_format(FuriLogLevel level, const char* tag, const char* format, ...) {
    do {
        if(level > furi_log.log_level) {
            break;
        }

        if(furi_mutex_acquire(furi_log.mutex, furi_kernel_is_running() ? FuriWaitForever : 0) !=
           FuriStatusOk) {
            break;
        }
        FuriString* string = furi_string_alloc();

        const char* color = _FURI_LOG_CLR_RESET;
        const char* log_letter = " ";
        switch(level) {
        case FuriLogLevelError:
            color = _FURI_LOG_CLR_E;
            log_letter = "E";
            break;
        case FuriLogLevelWarn:
            color = _FURI_LOG_CLR_W;
            log_letter = "W";
            break;
        case FuriLogLevelInfo:
            color = _FURI_LOG_CLR_I;
            log_letter = "I";
            break;
        case FuriLogLevelDebug:
            color = _FURI_LOG_CLR_D;
            log_letter = "D";
            break;
        case FuriLogLevelTrace:
            color = _FURI_LOG_CLR_T;
            log_letter = "T";
            break;
        default:
            break;
        }

        // Timestamp
        furi_string_printf(
            string, "%lu %s[%s][%s] " _FURI_LOG_CLR_RESET, furi_get_tick(), color, log_letter, tag);
        furi_log_puts(furi_string_get_cstr(string));
        furi_string_reset(string);

        va_list args;
        va_start(args, format);
        furi_string_vprintf(string, format, args);
        va_end(args);

        furi_log_puts(furi_string_get_cstr(string));
        furi_string_free(string);

        furi_log_puts("\r\n");

        furi_mutex_release(furi_log.mutex);
    } while(0);
}

void furi_log_print_raw_format(FuriLogLevel level, const char* format, ...) {
    if(level <= furi_log.log_level &&
       furi_mutex_acquire(furi_log.mutex, FuriWaitForever) == FuriStatusOk) {
        FuriString* string;
        string = furi_string_alloc();
        va_list args;
        va_start(args, format);
        furi_string_vprintf(string, format, args);
        va_end(args);

        furi_log_puts(furi_string_get_cstr(string));
        furi_string_free(string);

        furi_mutex_release(furi_log.mutex);
    }
}

void furi_log_set_level(FuriLogLevel level) {
    furi_check(level <= FuriLogLevelTrace);

    if(level == FuriLogLevelDefault) {
        level = FURI_LOG_LEVEL_DEFAULT;
    }
    furi_log.log_level = level;
}

FuriLogLevel furi_log_get_level(void) {
    return furi_log.log_level;
}

bool furi_log_level_to_string(FuriLogLevel level, const char** str) {
    for(size_t i = 0; i < COUNT_OF(FURI_LOG_LEVEL_DESCRIPTIONS); i++) {
        if(level == FURI_LOG_LEVEL_DESCRIPTIONS[i].level) {
            *str = FURI_LOG_LEVEL_DESCRIPTIONS[i].str;
            return true;
        }
    }
    return false;
}

bool furi_log_level_from_string(const char* str, FuriLogLevel* level) {
    for(size_t i = 0; i < COUNT_OF(FURI_LOG_LEVEL_DESCRIPTIONS); i++) {
        if(strcmp(str, FURI_LOG_LEVEL_DESCRIPTIONS[i].str) == 0) {
            *level = FURI_LOG_LEVEL_DESCRIPTIONS[i].level;
            return true;
        }
    }
    return false;
}

static void
    furi_log_dump_line(FuriString* string, uint32_t addr, const uint8_t* data, size_t len) {
    furi_string_printf(string, "\t%08lX: ", addr);

    char text_line[17] = {0};

    for(uint8_t i = 0; i < 16; i++) {
        if(i < len) {
            furi_string_cat_printf(string, "%02X ", data[i]);
            if((data[i] < 0x20) || (data[i] >= 0x7F)) {
                text_line[i] = '.';
            } else {
                text_line[i] = data[i];
            }
        } else {
            furi_string_cat_printf(string, "   ");
        }
    }

    furi_string_cat_printf(string, "| %s\r\n", text_line);
}

void furi_log_hexdump(FuriLogLevel level, const char* tag, const uint8_t* data, size_t len) {
    if(level > furi_log.log_level) {
        return;
    }

    furi_log_print_format(level, tag, "addr: %08lX len: %u", (uint32_t)data, len);

    if(furi_mutex_acquire(furi_log.mutex, furi_kernel_is_running() ? FuriWaitForever : 0) !=
       FuriStatusOk) {
        return;
    }
    FuriString* string = furi_string_alloc();

    uint32_t addr = 0;

    for(addr = 0; addr < len; addr += 16) {
        size_t line_len = len - addr;
        if(line_len > 16) {
            line_len = 16;
        }
        furi_log_dump_line(string, addr, &data[addr], line_len);
        furi_log_puts(furi_string_get_cstr(string));
    }
    furi_log_puts("\r\n");

    furi_string_free(string);

    furi_mutex_release(furi_log.mutex);
}
