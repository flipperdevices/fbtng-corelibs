/**
 * @file rwlock.h
 * FuriRwLock - Read-Write Lock implementation
 *
 * Allows multiple concurrent readers or a single writer.
 */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriRwLock FuriRwLock;

/**
 * @brief Allocate a read-write lock
 *
 * @return pointer to FuriRwLock instance
 */
FuriRwLock* furi_rwlock_alloc(void);

/**
 * @brief Free a read-write lock
 *
 * @param instance The pointer to FuriRwLock instance
 */
void furi_rwlock_free(FuriRwLock* instance);

/**
 * @brief Acquire read lock (shared access)
 *
 * Multiple threads can hold read locks simultaneously.
 * Blocks if a write lock is held.
 *
 * @param instance The pointer to FuriRwLock instance
 * @param timeout The timeout in ticks
 *
 * @return The furi status
 */
FuriStatus furi_rwlock_acquire_read(FuriRwLock* instance, uint32_t timeout);

/**
 * @brief Release read lock
 *
 * @param instance The pointer to FuriRwLock instance
 *
 * @return The furi status
 */
FuriStatus furi_rwlock_release_read(FuriRwLock* instance);

/**
 * @brief Acquire write lock (exclusive access)
 *
 * Only one thread can hold a write lock.
 * Blocks if any read or write locks are held.
 *
 * @param instance The pointer to FuriRwLock instance
 * @param timeout The timeout in ticks
 *
 * @return The furi status
 */
FuriStatus furi_rwlock_acquire_write(FuriRwLock* instance, uint32_t timeout);

/**
 * @brief Release write lock
 *
 * @param instance The pointer to FuriRwLock instance
 *
 * @return The furi status
 */
FuriStatus furi_rwlock_release_write(FuriRwLock* instance);

#ifdef __cplusplus
}
#endif
