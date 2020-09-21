#ifndef BOB_PRIVATE_H_
#define BOB_PRIVATE_H_

#include <stddef.h>

#include "bob.h"
#include "file.h"

/**
 * @name Block identifiers
 *
 * @{
 */

/**
 * BOB_BLOCKID_REWRITE means the data is rewritten from scratch.
 */
#define BOB_BLOCKID_REWRITE 0x01u
/** @} */

struct BOB {
  /** len is the length of the current data. */
  size_t len;

  /** offset is the offset into @c data where the raw data begins. */
  size_t offset;

  /** data points to the current data. May be a null pointer if len is zero. */
  void *data;

  /* file represents the file. */
  struct BOBFile *file;
};

#endif
