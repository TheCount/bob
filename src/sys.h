#ifndef BOB_SYS_H_
#define BOB_SYS_H_

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * bob_sys_close wraps close(2).
 */
static inline int bob_sys_close(int fd) {
  for (;;) {
    int rc = close(fd);
    if ((rc != 0) && (errno == EINTR)) {
      continue;
    }
    return rc;
  }
}

/**
 * bob_sys_fallocate wraps fallocate(2).
 */
static inline int bob_sys_fallocate(int fd, int mode, off_t offset, off_t len) {
  for (;;) {
    int rc = fallocate(fd, mode, offset, len);
    if ((rc != 0) && (errno == EINTR)) {
      continue;
    }
    return rc;
  }
}

/**
 * bob_sys_fstatvfs wraps fstatvfs(3).
 */
static inline int bob_sys_fstatvfs(int fd, struct statvfs *buf) {
  for (;;) {
    int rc = fstatvfs(fd, buf);
    if ((rc != 0) && (errno == EINTR)) {
      continue;
    }
    return rc;
  }
}

/**
 * bob_sys_fsync wraps fsync(2).
 */
static inline int bob_sys_fsync(int fd) {
  for (;;) {
    int rc = fsync(fd);
    if ((rc != 0) && (errno == EINTR)) {
      continue;
    }
    return rc;
  }
}

/**
 * bob_sys_lseek wraps lseek(2).
 */
static inline off_t bob_sys_lseek(int fd, off_t offset, int whence) {
  for (;;) {
    off_t result = lseek(fd, offset, whence);
    if ((result < 0) && (errno == EINTR)) {
      continue;
    }
    return result;
  }
}

/**
 * bob_sys_open wraps open(2).
 */
static inline int bob_sys_open(const char *pathname, int flags) {
  for (;;) {
    int fd = open(pathname, flags);
    if ((fd == -1) && (errno == EINTR)) {
      continue;
    }
    return fd;
  }
}

/**
 * bob_sys_unlink wraps unlink(2).
 */
static inline int bob_sys_unlink(const char *pathname) {
  for (;;) {
    int rc = unlink(pathname);
    if ((rc != 0) && (errno == EINTR)) {
      continue;
    }
    return rc;
  }
}

/**
 * bob_sys_write wraps write(2).
 */
static inline int bob_sys_write(int fd, const void *buf, size_t count) {
  while (count > 0) {
    ssize_t written = write(fd, buf, count);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    buf = (const char *) buf + written;
    count -= (size_t) written;
  }
  return 0;
}

/**
 * bob_sys_read wraps read(2).
 */
static inline ssize_t bob_sys_read(int fd, void *buf, size_t count) {
  for (;;) {
    ssize_t rd = read(fd, buf, count);
    if ((rd < 0) && (errno == EINTR)) {
      continue;
    }
    return rd;
  }
}

/**
 * bob_sys_fallocate_next pre-allocates file space from the current file
 * position.
 *
 * @param fd File descriptor on which to allocate space.
 * @param len The number of bytes to allocate.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_sys_fallocate_next(int fd, off_t len) {
  off_t cur = bob_sys_lseek(fd, 0, SEEK_CUR);
  if (cur < 0) {
    return -1;
  }
  return bob_sys_fallocate(fd, FALLOC_FL_KEEP_SIZE, cur, len);
}

#endif
