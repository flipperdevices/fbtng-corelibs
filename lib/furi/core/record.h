/**
 * @file record.h
 * @brief Atomic reference-counted storage.
 *
 * The FuriRecord API provides a way of storing arbitrary data using a global
 * key-value storage and a synchronisation mechanism for producers and consumers.
 *
 * All of the below functions are thread-safe, but care must be taken
 * regarding the function call order (create/destroy, open/close).
 *
 * Data producers should use @ref furi_record_create to make their data available
 * to consumers and @ref furi_record_create to revoke consumer access to it.
 *
 * ```C
 * int producer_thread() {
 *     MyData* data_ptr = my_data_alloc();
 *
 *     // Make data_ptr available under "my_data" record
 *     furi_record_create("my_data", data_ptr);
 *
 *     // The record lifetime goes here
 *
 *     // Make data_ptr unavailable to consumers
 *     // Note: this function will block until all consumers
 *     // have called furi_record_close() (see below)
 *     furi_record_destroy("my_data");
 *
 *     // It is now safe to actually free the data
 *     my_data_free(data_ptr);
 * }
 * ```
 *
 * Data consumers should first use @ref furi_record_open or @ref furi_record_open_ex
 * in order to gain access to the data required. After a record has been opened,
 * it is guaranteed to be valid until a @ref furi_record_close call.
 *
 * ```C
 * int consumer_thread() {
 *     // Note: this function will block until
 *     // the producer thread calls furi_record_create()
 *     MyData* data_ptr = furi_record_open("my_data");
 *
 *     use_data(data_ptr);
 *
 *    // Release the data
 *    furi_record_close("my_data");
 *
 *    // WRONG: data_ptr may no longer be valid at this point
 *    use_data(data_ptr);
 * }
 *
 * ```
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "core_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the record storage.
 *
 * @warning For internal use only. User code should NOT call this.
 *
 * This function is called during startup by the system initialisation procedure.
 */
void furi_record_init(void);

/**
 * @brief Check if the record exists.
 *
 * @param[in] name name of the record to check
 *
 * @returns @c true if the record exists, @c false otherwise
 */
bool furi_record_exists(const char* name);

/**
 * @brief Create a record with the given name and data.
 *
 * After the record creation, pointer to @p data will be available
 * to consumers via the @p name string key.
 *
 * @warning Record names MUST be unique at any point in time.
 *
 * @param[in] name name of the record to create
 * @param[in] data pointer to an arbitrary value
 */
void furi_record_create(const char* name, void* data);

/**
 * @brief Destroy the record under the given name.
 *
 * @pre The record MUST be created by @ref furi_record_create.
 *
 * Blocks the calling thread until the record has been released by of all its users.
 *
 * Consumers calling `furi_record_open()` will be blocked until
 * the record is created again via `furi_record_create()`.
 *
 * @warning It is only safe to destroy a record from the same thread
 *          that created it using `furi_record_create()`
 *
 * @param[in] name name of the record to be destroyed
 */
void furi_record_destroy(const char* name);

/**
 * Open a record under the given name.
 *
 * Blocks the calling thread indefinitely until the requested record becomes available.
 *
 * The return value is guaranteed to be non-@c NULL. Additionally, it is guaranteed
 * to remain valid (by contract) until a call to `furi_record_close()`.
 *
 * @note Consumer threads MUST always have a paired furi_record_close() call
 *       once they are done working with the record data, unless they never exit
 *       (e.g. service threads may not need to ever close a record)
 *
 * @param[in] name name of the record to be opened
 *
 * @returns pointer to the record data
 */
FURI_RETURNS_NONNULL void* furi_record_open(const char* name);

/**
 * @brief Open a record under a given name, extended version.
 *
 * Same as @ref furi_record_open, but will fail if the requested record
 * has not been made available by the producer within a specified timeout.
 *
 * @param[in] name name of the record
 * @param[in] timeout timeout to wait for the record, in ticks
 *
 * @return pointer to the record data or @c NULL if timeout occurred
 */
void* furi_record_open_ex(const char* name, uint32_t timeout);

/**
 * @brief Close a record under the given name.
 *
 * @pre The record MUST be opened by @ref furi_record_open or @ref furi_record_open_ex.
 *
 * After a call to this function, the data pointer acquired via `furi_record_open()`
 * MUST NOT be used.
 *
 * @param[in] name name of the record to be closed
 */
void furi_record_close(const char* name);

#ifdef __cplusplus
}
#endif
