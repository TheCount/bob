#ifndef BOB_VARINT_H_
#define BOB_VARINT_H_

#include <stdint.h>

/**
 * bob_varint_encode encodes an unsigned integer in varint format.
 *
 * @param dest Buffer to write varint to. Must be big enough to actually hold
 *  the varint encoding of @c n. A size of @c 10 is a safe choice.
 * @param n The integer to encode.
 *
 * @return The size of the encoded integer is returned (i. e., the number of
 *  bytes written to @c dest).
 */
static inline size_t bob_varint_encode(uint8_t *dest, uint64_t n) {
  uint8_t *ptr = dest;
  while (n >= 0x80) {
    *ptr++ = 0x80 | (n % 0x80);
    n /= 0x80;
  }
  *ptr++ = n;
  return ptr - dest;
}

/**
 * bob_varint_decode decodes an unsigned integer in varint format through
 * multiple steps.
 *
 * @param dest Pointer to where the result is written.
 * @param next Next byte of decoded varint.
 * @param count Call count. On the first call, count must be zero.
 *  Subsequent calls must use the return value of the previous call.
 *
 * @return On success, a non-negative value is returned. If the varint has been
 *  fully parsed, the return value is @c 0. Otherwise, the return value is the
 *  next argument to the @c count parameter.
 *  On error, @c -1 is returned.
 */
static inline int bob_varint_decode(
  uint64_t *dest, uint8_t next, int count
) {
  if (count == 0) {
    *dest = next % 0x80;
    return (next >> 7);
  }
  if (((count == 9) && (next > 1)) || (count > 9) || (count < 0)) {
    return -1;
  }
  if (next & 0x80) {
    *dest |= ((uint64_t) next % 0x80) << (count * 7);
    return count + 1;
  }
  if (next == 0) {
    /* invalid short form */
    return -1;
  }
  *dest |= (uint64_t) next << (count * 7);
  return 0;
}

#endif
