#ifndef BOB_CONFIG_H_
#define BOB_CONFIG_H_

#include "bob.h"

/**
 * @name Configuration IDs
 *
 * These IDs are used to serialise the configuration.
 *
 * @{
 */
#define BOB_CONFID_END 0
#define BOB_CONFID_BLOCK_SIZE 1
#define BOB_CONFID_CUE_SIZE 2
/** @} */

struct BOBConfig {
  /* See documentation in bob.h for the meaning of these parameters. */
  unsigned long blocksize;
  unsigned long cuesize;
};

#endif
