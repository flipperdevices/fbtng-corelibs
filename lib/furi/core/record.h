/**
 * @file record.h
 * Furi: record API
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "core_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize record storage For internal use only.
 */
void furi_record_init(void);

/** Check if record exists
 *
 * @param      name  record name
 * @note       Thread safe. Create and destroy must be executed from the same
 *             thread.
 */
bool furi_record_exists(const char* name);

/** Create record
 *
 * @param      name  record name
 * @param      data  data pointer
 * @note       Thread safe. Create and destroy must be executed from the same
 *             thread.
 */
void furi_record_create(const char* name, void* data);

/** Destroy record
 *
 * @param      name  record name
 *
 * Blocks the calling thread until the record has been released by all its users.
 * Threads calling `furi_record_open()` will be blocked until
 * the record is created again via `furi_record_create()`.
 *
 * @note       Thread safe. Create and destroy must be executed from the same
 *             thread.
 */
void furi_record_destroy(const char* name);

/** Open record
 *
 * @param      name  record name
 *
 * @return     pointer to the record
 * @note       Thread safe. Open and close must be executed from the same
 *             thread. Suspends caller thread till record is available
 */
FURI_RETURNS_NONNULL void* furi_record_open(const char* name);

/** Open record or fail after a timeout
 *
 * @param      name  record name
 * @param      timeout timeout in ticks
 *
 * @return     pointer to the record or @c NULL in case of failure
 * @note       Thread safe. Open and close must be executed from the same
 *             thread. Suspends caller thread till record is available or
 *             until the timeout has expired
 */
void* furi_record_open_ex(const char* name, uint32_t timeout);

/** Close record
 *
 * @param      name  record name
 * @note       Thread safe. Open and close must be executed from the same
 *             thread.
 */
void furi_record_close(const char* name);

#ifdef __cplusplus
}
#endif
