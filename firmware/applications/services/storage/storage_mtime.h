/**
 * @file storage_mtime.h
 * R0N1N: a file's modification time. Kept out of the SDK headers (not in
 * api_symbols.csv) -- it's for firmware services such as the R0N1N Capture
 * Timeline, and FileInfo itself can't grow without breaking existing apps.
 */
#pragma once

#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Get the last modification time of the file at `path` as a UNIX timestamp.
 *
 * Unlike storage_common_timestamp(), which reports the last change anywhere
 * on the storage, this is the file's own time from the filesystem.
 *
 * @param storage pointer to a storage API instance
 * @param path pointer to a zero-terminated string containing the path
 * @param timestamp pointer to the value to fill
 * @return FSE_OK on success, any other error code on failure
 */
FS_Error storage_common_mtime(Storage* storage, const char* path, uint32_t* timestamp);

#ifdef __cplusplus
}
#endif
