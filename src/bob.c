#include <errno.h>
#include <stdlib.h>

#include "config.h"
#include "file.h"
#include "private.h"
#include "varint.h"

struct BOB* bob_create(struct BOBConfig *cfg, const char *path) {
  int lerrno;
  if (path == NULL) {
    lerrno = EINVAL;
    goto badarg;
  }
  struct BOB *result = malloc(sizeof(*result));
  if (result == NULL) {
    lerrno = ENOMEM;
    goto nomem;
  }
  result->len = 0;
  result->offset = 0;
  result->data = NULL;
  result->file = bob_file_create(cfg, path);
  if (result->file == NULL) {
    lerrno = errno;
    goto nofile;
  }
  return result;

nofile:
  free(result);
nomem:
badarg:
  errno = lerrno;
  return NULL;
}

struct BOB* bob_open(const char *path) {
  int lerrno;
  if (path == NULL) {
    lerrno = EINVAL;
    goto badarg;
  }
  struct BOB *result = malloc(sizeof(*result));
  if (result == NULL) {
    lerrno = ENOMEM;
    goto nomem;
  }
  result->offset = 0;
  result->file = bob_file_open(path);
  if (result->file == NULL) {
    lerrno = errno;
    goto nofile;
  }
  result->data = bob_file_parse(result->file, &result->len);
  if (result->len == (size_t) -1) {
    lerrno = errno;
    goto noparse;
  }
  return result;

noparse:
  bob_file_close(result->file);
nofile:
  free(result);
nomem:
badarg:
  errno = lerrno;
  return NULL;
}

int bob_close(struct BOB *bob) {
  if (bob == NULL) {
    errno = EBADF;
    return -1;
  }
  struct BOBFile *file = bob->file;
  free(bob->data);
  free(bob);
  return bob_file_close(file);
}

int bob_set(struct BOB* bob, size_t len, const void *data) {
  int lerrno;
  if ((bob == NULL) || ((data == NULL) && (len > 0))) {
    lerrno = EINVAL;
    goto badargs;
  }
  /* FIXME: implement diff */
  uint8_t vbuf[10];
  size_t vlen = bob_varint_encode(vbuf, len);
  size_t newlen = 1u + vlen + len;
  uint8_t *newdata = malloc(newlen);
  if (newdata == NULL) {
    lerrno = ENOMEM;
    goto nomem;
  }
  newdata[0] = BOB_BLOCKID_REWRITE;
  memcpy(newdata + 1, vbuf, vlen);
  memcpy(newdata + 1 + vlen, data, len);
  off_t cue_space_left = bob_file_cue_remaining(bob->file);
  if (cue_space_left < 0) {
    lerrno = errno;
    goto nocuespace;
  }
  off_t start_off = 0; /* start offset of new cue block, if any */
  if (cue_space_left < (off_t) newlen) {
    start_off = bob_file_new_cue(bob->file);
    if (start_off < 0) {
      lerrno = errno;
      goto nonewcue;
    }
  }
  if (bob_file_write(bob->file, newdata, newlen) != 0) {
    lerrno = errno;
    goto nowrite;
  }
  if (bob_file_write_commit(bob->file) != 0) {
    lerrno = errno;
    goto nowrite;
  }
  free(bob->data);
  bob->len = newlen;
  bob->offset = newlen - len;
  bob->data = newdata;
  if (start_off > 0) {
    return bob_file_zap(bob->file, start_off);
  }
  return 0;

nowrite:
nonewcue:
nocuespace:
  free(newdata);
nomem:
badargs:
  errno = lerrno;
  return -1;
}

int bob_flush(struct BOB *bob) {
  if (bob == NULL) {
    errno = EINVAL;
    return -1;
  }
  return bob_file_flush(bob->file);
}

const void* bob_current(struct BOB *bob, size_t *len) {
  if (len == NULL) {
    errno = EINVAL;
    return NULL;
  }
  if (bob == NULL) {
    *len = (size_t) -1;
    errno = EINVAL;
    return NULL;
  }
  *len = bob->len - bob->offset;
  return bob->data + bob->offset;
}
