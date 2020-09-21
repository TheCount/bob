#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "varint.h"

static const char *const VALID = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01";
static const char *const INVALID = "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02";
static const char *const LONG = "\x80\x00";

static void test_varint(uint64_t n) {
  uint8_t buf[10];
  uint64_t back;
  size_t size = bob_varint_encode(buf, n);
  assert(size <= 10);
  int count = 0;
  size_t i;
  for (i = 0; i != sizeof(buf); ++i) {
    count = bob_varint_decode(&back, buf[i], count);
    assert(count >= 0);
    if (count == 0) {
      break;
    }
  }
  assert(back == n);
  assert(i < size);
}

int main(void) {
  /* Test edge cases */
  assert(bob_varint_decode(NULL, 42, 10) < 0);
  uint64_t n;
  int count = 0;
  for (int i = 0;; ++i) {
    count = bob_varint_decode(&n, VALID[i], count);
    assert(count >= 0);
    if (count == 0) {
      break;
    }
  }
  assert(n == UINT64_MAX);
  count = 0;
  for (int i = 0;; ++i) {
    count = bob_varint_decode(&n, INVALID[i], count);
    printf("i = %d, count = %u, n = %lu\n", i, count, n);
    if (count < 0) {
      break;
    }
    assert(count > 0);
    assert(i < 10);
  }
  count = 0;
  for (int i = 0;; ++i) {
    count = bob_varint_decode(&n, LONG[i], count);
    if (count < 0) {
      break;
    }
    assert(count > 0);
    assert(i < 2);
  }
  /* Test small numbers */
  for (uint64_t i = 0; i != 65536; ++i) {
    test_varint(i);
  }
  /* Test large numbers */
  for (uint64_t i = UINT64_MAX; i != UINT64_MAX - 65536; --i) {
    test_varint(i);
  }
  /* Test random numbers */
  srand(time(NULL));
  for (int i = 0; i != 65536; ++i) {
    n = (uint64_t) rand();
    n ^= ((uint64_t) rand()) << 24;
    n ^= ((uint64_t) rand()) << 48;
    test_varint(n);
  }
}
