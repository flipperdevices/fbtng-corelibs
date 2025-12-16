#include "rwlock.h"
#include "semaphore.h"
#include "check.h"

struct FuriRwLock {
    FuriSemaphore* read_semaphore;
    FuriSemaphore* write_semaphore;
    uint32_t readers_count;
};

FuriRwLock* furi_rwlock_alloc(void) {
    FuriRwLock* rwlock = malloc(sizeof(*rwlock));

    rwlock->read_semaphore = furi_semaphore_alloc(1, 1);
    rwlock->write_semaphore = furi_semaphore_alloc(1, 1);
    rwlock->readers_count = 0;

    return rwlock;
}

void furi_rwlock_free(FuriRwLock* instance) {
    furi_check(instance);
    furi_check(instance->readers_count == 0);

    furi_semaphore_free(instance->read_semaphore);
    furi_semaphore_free(instance->write_semaphore);

    free(instance);
}

FuriStatus furi_rwlock_acquire_read(FuriRwLock* instance, uint32_t timeout) {
    furi_check(instance);

    FuriStatus acquire_status = furi_semaphore_acquire(instance->read_semaphore, timeout);
    furi_check(instance->readers_count < INT_MAX);

    if(acquire_status == FuriStatusOk) {
        do {
            if(instance->readers_count == 0) {
                acquire_status = furi_semaphore_acquire(instance->write_semaphore, timeout);
                if(acquire_status != FuriStatusOk) {
                    break;
                }
            }

            instance->readers_count++;
        } while(false);

        furi_semaphore_release(instance->read_semaphore);
    }

    return acquire_status;
}

FuriStatus furi_rwlock_release_read(FuriRwLock* instance) {
    furi_check(instance);

    furi_semaphore_acquire(instance->read_semaphore, FuriWaitForever);
    furi_check(instance->readers_count > 0);

    instance->readers_count--;

    FuriStatus release_status = FuriStatusOk;
    if(instance->readers_count == 0) {
        release_status = furi_semaphore_release(instance->write_semaphore);
    }

    furi_semaphore_release(instance->read_semaphore);

    return release_status;
}

FuriStatus furi_rwlock_acquire_write(FuriRwLock* instance, uint32_t timeout) {
    furi_check(instance);

    return furi_semaphore_acquire(instance->write_semaphore, timeout);
}

FuriStatus furi_rwlock_release_write(FuriRwLock* instance) {
    furi_check(instance);

    return furi_semaphore_release(instance->write_semaphore);
}
