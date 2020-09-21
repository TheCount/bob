#ifndef BOB_H_
#define BOB_H_

/**
 * @mainpage BOB — binary object files
 *
 *
 * @section intro BOBs, what are they?
 *
 * BOBs are files which represent a single, contiguous block of binary data.
 * The file format is designed to be gentle on the storage medium (especially
 * flash storage with a limited number of write/erase cycles) if the binary data
 * is changed often. In particular, for small changes, only the changes are
 * appended to the original data.
 *
 * BOBs are ideal for small to medium size data (up to to hundreds of
 * kilobytes), or larger data which changes very little. For larger data which
 * changes a lot, you don't need BOBs. Just write out the data and let the
 * system's wear levelling algorithms do their job.
 *
 *
 * @section conf BOB configuration
 *
 * A BOB currently supports the following configuration parameters.
 * <ul>
 *  <li>Block size: the block size of the underlying file system. The default is
 *    zero, in which case an attempt will be made to determine the block size
 *    when the underlying file is created. If no block size can be determined,
 *    a value of 32768 will be assumed, which is safe for most filesystems.
 *    Note that a wrong block size may cause #bob_set to fail if the underlying
 *    file system refuses certain operations.
 *  <li>Cue size: number of data bytes after which data should be written out in
 *    full (rather than just storing changes). Writing the data out in full from
 *    time to time reduces loading times and reduces the risk of file
 *    corruption. The cue size should be a multiple of the block size. The
 *    default is zero, in which case the cue size will be selected to be
 *    1024 times the block size.
 * </ul>
 *
 *
 * @section threadsafe Thread safety
 *
 * The functions in this library can be safely called concurrently, as long as
 * the @c BOB and @c BOBConfig arguments are distinct and the memory regions
 * corresponding to passed pointer arguments do not overlap.
 *
 *
 * @section filesafe File safety
 *
 * It is the responsibility of the caller to ensure that a BOB file is opened
 * only once at any given time. The library does not attempt to "lock" any file,
 * nor does it use any other means to ensure exclusive access.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BOBConfig holds the configuration for a BOB.
 */
typedef struct BOBConfig *BOBConfig;

/**
 * bob_config_default creates a new BOB configuration with default values.
 *
 * @return On success, the newly allocated default configuration is returned.
 *  The caller is responsible for releasing the resources associated with the
 *  configuration with #bob_config_del once it is no longer needed.
 *  On error, a null pointer is returned, and @c errno is set to @c ENOMEM.
 *
 * @sa bob_config_del
 */
BOBConfig bob_config_default(void);

/**
 * bob_config_del frees the resources associated with a BOB configuration.
 *
 * @param cfg Configuration whose resources should be freed. The configuration
 *  must not be used afterwards.
 */
void bob_config_del(BOBConfig cfg);

/**
 * bob_config_set_blocksize sets the block size in a BOB config.
 *
 * See #conf for more information.
 *
 * @param cfg The configuration the block size of which is to be set.
 * @param size The new block size. A value of zero is permitted.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and errno is set appropriately.
 *
 * @sa conf, bob_config_get_blocksize
 */
int bob_config_set_blocksize(BOBConfig cfg, unsigned long size);

/**
 * bob_config_get_blocksize returns the block size in a BOB config.
 *
 * See #conf for more information.
 *
 * @param cfg The configuration the block size of which is to be returned.
 *  If this is a null pointer, the block size in the default configuration
 *  is returned.
 *
 * @return The requested block size is returned.
 *
 * @sa conf, bob_config_set_blocksize.
 */
unsigned long bob_config_get_blocksize(BOBConfig cfg);

/**
 * bob_config_set_cuesize sets the cue size in a BOB config.
 *
 * See #conf for more information.
 *
 * @param cfg The configuration the cue size of which is to be set.
 * @param size The new cue size. A value of zero is permitted.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and errno is set appropriately.
 *
 * @sa conf, bob_config_get_cuesize
 */
int bob_config_set_cuesize(BOBConfig cfg, unsigned long size);

/**
 * bob_config_get_cuesize returns the cue size in a BOB config.
 *
 * See #conf for more information.
 *
 * @param cfg The configuration the cue size of which is to be returned.
 *  If this is a null pointer, the cue size in the default configuration
 *  is returned.
 *
 * @return The requested cue size is returned.
 *
 * @sa conf, bob_config_set_cuesize.
 */
unsigned long bob_config_get_cuesize(BOBConfig cfg);

/**
 * BOB represents a binary object file.
 */
typedef struct BOB *BOB;

/**
 * bob_create creates a new, zero-sized BOB.
 *
 * @param cfg The configuration of the new BOB.
 *  If this is a null pointer, a default configuration will be used.
 * @param path The path at which the new BOB should be created.
 *
 * @return On success, a handle representing a new, zero-sized BOB is returned.
 *  The caller is responsible for closing the BOB with #bob_close once it is no
 *  longer needed.
 *  On error, a null pointer is returned, and @c errno is set appropriately.
 *  One possible error is @c EEXIST, if a file at the specified path already
 *  exists.
 *
 * @sa bob_open, bob_close
 */
BOB bob_create(BOBConfig cfg, const char *path);

/**
 * bob_open opens and reads an existing BOB file.
 *
 * @param path The path to the BOB file.
 *
 * @return On success, a handle representing the BOB is returned.
 *  The caller is responsible for closing the BOB with #bob_close once it is no
 *  longer needed.
 *  On error, a null pointer is returned, and @c errno is set appropriately.
 *
 * @sa bob_create, bob_close
 */
BOB bob_open(const char *path);

/**
 * bob_close closes a BOB.
 *
 * Data associated with the BOB is flushed to disk.
 *
 * @param bob The BOB to be closed.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and @c errno is set appropriately.
 *  Even on error, the BOB will be closed once this function returns.
 *
 * @sa bob_create, bob_open
 */
int bob_close(BOB bob);

/**
 * bob_set replaces the BOB data.
 *
 * @param bob The BOB data to be set.
 * @param len The length of the new data in bytes.
 * @param data Pointer to new data.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and @c errno is set appropriately.
 *
 * @sa bob_current, bob_flush
 */
int bob_set(BOB bob, size_t len, const void *data);

/**
 * bob_flush ensures the latest BOB data has been physically written to the
 * disk or storage device.
 *
 * Note that #bob_set already writes the data to the underlying file.
 * This function merely ensures that any cached file data is physically written
 * to disk or storage device.
 *
 * @param bob The BOB whose data is to be flushed to disk.
 *
 * @sa bob_set
 */
int bob_flush(BOB bob);

/**
 * bob_current returns the latest BOB data.
 *
 * @param bob The BOB whose data should be returned.
 * @param len Pointer to where the length of the returned data is to be written,
 *  in bytes.
 *
 * @return On success, a pointer to the current BOB data is returned, and the
 *  length of the data is stored in @c *len. The returned data must not be
 *  modified by the caller. The returned pointer is valid until the next call
 *  to #bob_set or #bob_close on @c bob.
 *  On error, a null pointer is returned, <code>(size_t) -1</code> is stored
 *  in @c *len, and @c errno is set appropriately.
 *
 * @sa bob_set
 */
const void* bob_current(BOB bob, size_t *len);

#ifdef __cplusplus
}
#endif

#endif
