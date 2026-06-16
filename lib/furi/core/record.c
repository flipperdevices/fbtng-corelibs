#include "record.h"
#include "check.h"
#include "mutex.h"
#include "event_flag.h"

#include <m-dict.h>
#include <toolbox/m_cstr_dup.h>

#define FURI_RECORD_HOLDERS_MAX UINT16_MAX

typedef enum {
    FuriRecordFlagReady = 1UL << 0,
    FuriRecordFlagReleased = 1UL << 1,
} FuriRecordFlag;

typedef struct {
    FuriEventFlag* flags;
    FuriThread* owner;
    void* data;
    uint16_t holders_count;
    uint16_t pending_count;
} FuriRecordData;

DICT_DEF2(FuriRecordDataDict, const char*, M_CSTR_DUP_OPLIST, FuriRecordData, M_POD_OPLIST)

typedef struct {
    FuriMutex* mutex;
    FuriRecordDataDict_t records;
} FuriRecord;

static FuriRecord* furi_record = NULL;

static FuriRecordData* furi_record_get(const char* name) {
    return FuriRecordDataDict_get(furi_record->records, name);
}

static void furi_record_put(const char* name, const FuriRecordData* record_data) {
    FuriRecordDataDict_set_at(furi_record->records, name, *record_data);
}

static void furi_record_erase(const char* name, FuriRecordData* record_data) {
    furi_event_flag_free(record_data->flags);
    FuriRecordDataDict_erase(furi_record->records, name);
}

static void furi_record_reset(FuriRecordData* record_data) {
    furi_event_flag_clear(record_data->flags, FuriRecordFlagReady);
    record_data->owner = NULL;
    record_data->data = NULL;
    record_data->pending_count = record_data->holders_count;
    record_data->holders_count = 0;
}

void furi_record_init(void) {
    furi_record = malloc(sizeof(FuriRecord));
    furi_record->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    FuriRecordDataDict_init(furi_record->records);
}

static bool furi_record_data_wait_for_ready(const FuriRecordData* record_data, uint32_t timeout) {
    const uint32_t flags = furi_event_flag_wait(
        record_data->flags, FuriRecordFlagReady, FuriFlagWaitAny | FuriFlagNoClear, timeout);

    return (flags == FuriRecordFlagReady);
}

static void furi_record_data_wait_for_released(const FuriRecordData* record_data) {
    const uint32_t flags = furi_event_flag_wait(
        record_data->flags, FuriRecordFlagReleased, FuriFlagWaitAny, FuriWaitForever);

    furi_check(flags == FuriRecordFlagReleased);
}

static FuriRecordData* furi_record_data_create(const char* name) {
    const FuriRecordData new_record = {
        .flags = furi_event_flag_alloc(),
        .data = NULL,
        .holders_count = 0,
        .pending_count = 0,
    };

    furi_record_put(name, &new_record);

    return furi_record_get(name);
}

static FuriRecordData* furi_record_data_get_or_create(const char* name) {
    furi_check(furi_record);

    FuriRecordData* record_data = furi_record_get(name);
    if(!record_data) {
        record_data = furi_record_data_create(name);
    }

    return record_data;
}

static void furi_record_lock(void) {
    furi_check(furi_mutex_acquire(furi_record->mutex, FuriWaitForever) == FuriStatusOk);
}

static void furi_record_unlock(void) {
    furi_check(furi_mutex_release(furi_record->mutex) == FuriStatusOk);
}

bool furi_record_exists(const char* name) {
    furi_check(furi_record);
    furi_check(name);

    bool ret = false;

    furi_record_lock();
    FuriRecordData* record_data = furi_record_get(name);
    ret = record_data && (furi_event_flag_get(record_data->flags) & FuriRecordFlagReady);
    furi_record_unlock();

    return ret;
}

void furi_record_create(const char* name, void* data) {
    furi_check(furi_record);
    furi_check(name);

    furi_record_lock();

    // Get record data and fill it
    FuriRecordData* record_data = furi_record_data_get_or_create(name);
    furi_check(record_data->owner == NULL);
    furi_check(record_data->data == NULL);
    record_data->owner = furi_thread_get_current();
    record_data->data = data;
    furi_event_flag_set(record_data->flags, FuriRecordFlagReady);

    furi_record_unlock();
}

void furi_record_destroy(const char* name) {
    furi_check(furi_record);
    furi_check(name);

    bool should_wait = false;

    furi_record_lock();

    FuriRecordData* record_data = furi_record_get(name);
    furi_check(record_data);
    furi_check(record_data->owner == furi_thread_get_current());

    if(record_data->holders_count == 0) {
        furi_record_erase(name, record_data);
    } else {
        furi_record_reset(record_data);
        should_wait = true;
    }

    furi_record_unlock();

    if(should_wait) {
        furi_record_data_wait_for_released(record_data);

        furi_record_lock();

        if(record_data->holders_count == 0) {
            furi_record_erase(name, record_data);
        }

        furi_record_unlock();
    }
}

void* furi_record_open(const char* name) {
    return furi_record_open_ex(name, FuriWaitForever);
}

void* furi_record_open_ex(const char* name, uint32_t timeout) {
    furi_check(furi_record);
    furi_check(name);

    furi_record_lock();

    FuriRecordData* record_data = furi_record_data_get_or_create(name);
    furi_check(record_data->holders_count < FURI_RECORD_HOLDERS_MAX);
    record_data->holders_count++;

    furi_record_unlock();

    if(!furi_record_data_wait_for_ready(record_data, timeout)) {
        furi_record_lock();

        record_data->holders_count--;

        furi_record_unlock();
    }

    return record_data->data;
}

void furi_record_close(const char* name) {
    furi_check(furi_record);
    furi_check(name);

    furi_record_lock();

    FuriRecordData* record_data = furi_record_get(name);
    furi_check(record_data);

    if(record_data->pending_count > 0) {
        record_data->pending_count--;

        if(record_data->pending_count == 0) {
            furi_thread_flags_set(record_data->flags, FuriRecordFlagReleased);
        }

    } else if(record_data->holders_count > 0) {
        record_data->holders_count--;
    } else {
        furi_crash("Closed more times than opened");
    }

    furi_record_unlock();
}
