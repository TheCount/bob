#ifndef BOB_FILE_H_
#define BOB_FILE_H_

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "private.h"
#include "sys.h"
#include "varint.h"

/** DEFAULT_BLOCK_SIZE is the fallback file system block size to be used. */
#define DEFAULT_BLOCK_SIZE 32768ul

/** MIN_BLOCK_SIZE is the minimum allowed block size. */
#define MIN_BLOCK_SIZE 512ul

/** MAX_BLOCK_SIZE is the maximum allowed block size. */
#define MAX_BLOCK_SIZE (4ul * 1024ul * 1024ul)

/**
 * CUE_SIZE_MULTIPLIER is the default multiplier for the cue size.
 *
 * If no proper cue size is configured, the block size will be multiplied with
 * this multiplier to obtain the actual cue size.
 */
#define CUE_SIZE_MULTIPLIER 32ul

/** MAX_CUE_SIZE is the maximum possible cue size. */
#define MAX_CUE_SIZE (1024ul * 1024ul * 1024ul)

/** FILE_MAGIC marks the file as a BOB file. */
static const char FILE_MAGIC[4] = {'B', 'O', 'B', '\0'};

/**
 * BOBFile describes a BOB file.
 */
struct BOBFile {
  /** fd is the file descriptor. */
  int fd;

  /** blocksize is the assumed block size. */
  unsigned long blocksize;

  /** cuesize is the assumed cue size. */
  unsigned long cuesize;

  /**
   * buf is a buffer of size blocksize. It is used for buffering reads when
   * a file is parsed, and for buffering writes afterwards.
   */
  uint8_t *buf;

  /** pos is the current byte position into the buffer @c buf. */
  size_t pos;

  /**
   * written is used in two different situations. When opening and parsing an
   * existing file, written is the number of bytes written from the file into
   * @c buf. Once the file has been parsed and is henceforth only be used for
   * writing, written is the size of the initial segment of @c buf already
   * written to @c fd.
   */
  size_t written;
};

/**
 * bob_get_real_blocksize obtains the actual block size to be used in file
 * operations.
 *
 * When the configured block size looks invalid, bob_get_real_blocksize will
 * try to obtain the actual block size of the filesystem of @c fd. If all else
 * fails, a default block size is used.
 *
 * @param fd File descriptor to use to get block size if @c confblocksize is
 *  invalid.
 * @param confblocksize Block size to return, unless invalid.
 *
 * @return The actual block size to be used is returned.
 */
static inline unsigned long bob_get_real_blocksize(
  int fd, unsigned long confblocksize
) {
  if ((confblocksize >= MIN_BLOCK_SIZE) && (confblocksize <= MAX_BLOCK_SIZE)) {
    return confblocksize;
  }
  struct statvfs buf;
  if (bob_sys_fstatvfs(fd, &buf) != 0) {
    return DEFAULT_BLOCK_SIZE;
  }
  if ((buf.f_bsize < MIN_BLOCK_SIZE) || (buf.f_bsize > MAX_BLOCK_SIZE)) {
    return DEFAULT_BLOCK_SIZE;
  }
  return buf.f_bsize;
}

/**
 * bob_get_real_cuesize obtains the actual cue size to be used in file
 * operations.
 *
 * This function ensures that the cue size is a multiple of the block size,
 * and within certain bounds.
 *
 * @param blocksize The actual block size.
 * @param confcuesize The configured cue size.
 *
 * @return The actual cue size to be used is returned.
 */
static inline unsigned long bob_get_real_cuesize(
  unsigned long blocksize, unsigned long confcuesize
) {
  if (confcuesize < blocksize) {
    return blocksize * CUE_SIZE_MULTIPLIER;
  }
  if (confcuesize > MAX_CUE_SIZE) {
    confcuesize = MAX_CUE_SIZE;
  }
  return confcuesize - (confcuesize % blocksize);
}

/**
 * bob_file_initbuf initialises the file buffer of a BOB file.
 *
 * The file must already be open, and block size and cue size must be
 * properly set.
 *
 * @param file The BOB file.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_file_initbuf(struct BOBFile *file) {
  if (bob_sys_fallocate_next(file->fd, (off_t) file->blocksize) != 0) {
    return -1;
  }
  file->buf = malloc(file->blocksize);
  if (file->buf == NULL) {
    errno = ENOMEM;
    return -1;
  }
  file->pos = 0;
  file->written = 0;
  return 0;
}

/**
 * bob_file_write writes data to a BOB file buffer.
 *
 * This function does not call write(2) if the data still fits in the buffer.
 *
 * @param file BOB file.
 * @param buf Pointer to data to write.
 * @param count Number of bytes to write.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and @c errno is set appropriately.
 */
static inline int bob_file_write(
  struct BOBFile *file, const void *buf, size_t count
) {
  /* Simple case */
  if (file->pos + count <= file->blocksize) {
    memcpy(file->buf + file->pos, buf, count);
    file->pos += count;
    return 0;
  }
  /* Allocate extra filespace */
  off_t allocate = (off_t) (file->pos + count);
  if (allocate % (off_t) file->blocksize != 0) {
    allocate += (off_t) file->blocksize;
    allocate -= allocate % (off_t) file->blocksize;
  }
  if (bob_sys_fallocate_next(file->fd, allocate - (off_t) file->pos) != 0) {
    return -1;
  }
  /* write data */
  if (file->written != file->pos) {
    if (bob_sys_write(
      file->fd, file->buf + file->written, file->pos - file->written
    ) != 0) {
      return -1;
    }
  }
  size_t surplus = file->blocksize - file->pos;
  size_t numblocks = (count - surplus) / file->blocksize;
  size_t towrite = surplus + numblocks * file->blocksize;
  if (bob_sys_write(file->fd, buf, towrite) != 0) {
    return -1;
  }
  file->written = 0;
  memcpy(file->buf, buf + towrite, count - towrite);
  file->pos = count - towrite;
  return 0;
}

/**
 * bob_file_write_commit writes any unwritten data to the underlying file.
 *
 * @param file BOB file.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and @c errno is set appropriately.
 */
static inline int bob_file_write_commit(struct BOBFile *file) {
  if (file->written == file->pos) {
    return 0;
  }
  if (bob_sys_write(
    file->fd, file->buf + file->written, file->pos - file->written
  ) != 0) {
    return -1;
  }
  file->written = file->pos;
  return 0;
}

/**
 * bob_file_write_header writes the file header to the write buffer.
 *
 * @param file BOB file.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_file_write_header(struct BOBFile *file) {
  /* File magic */
  if (bob_file_write(file, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0) {
    return -1;
  }
  /* Configuration */
  uint8_t vbuf[10];
  size_t size = bob_varint_encode(vbuf, BOB_CONFID_BLOCK_SIZE);
  if (bob_file_write(file, vbuf, size) != 0) {
    return -1;
  }
  size = bob_varint_encode(vbuf, file->blocksize);
  if (bob_file_write(file, vbuf, size) != 0) {
    return -1;
  }
  size = bob_varint_encode(vbuf, BOB_CONFID_CUE_SIZE);
  if (bob_file_write(file, vbuf, size) != 0) {
    return -1;
  }
  size = bob_varint_encode(vbuf, file->cuesize);
  if (bob_file_write(file, vbuf, size) != 0) {
    return -1;
  }
  size = bob_varint_encode(vbuf, BOB_CONFID_END);
  if (bob_file_write(file, vbuf, size) != 0) {
    return -1;
  }
}

/**
 * bob_file_iseof checks whether the specified file is at EOF.
 *
 * @param file BOB file.
 *
 * @return On error, @c -1 is returned and @c errno is set appropriately.
 *  Otherwise, if the specified file is at EOF, @c 1 is returned.
 *  Otherwise, @c 0 is returned.
 */
static inline int bob_file_iseof(struct BOBFile *file) {
  if (file->pos != file->written) {
    return 0;
  }
  if (file->written == file->blocksize) {
    file->pos = 0;
    file->written = 0;
  }
  ssize_t rd = bob_sys_read(file->fd, file->buf + file->written,
    file->blocksize - file->written);
  if (rd < 0) {
    return -1;
  }
  if (rd == 0) {
    return 1;
  }
  file->written += (size_t) rd;
  return 0;
}

/**
 * bob_file_read reads @c count bytes of data into the specified buffer.
 *
 * @param file BOB file.
 * @param buf Buffer to write read data to.
 * @param count Number of bytes to read.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately. In
 *  particular, if the buffer cannot be filled due to EOF, @c errno will be set
 *  to EILSEQ.
 */
static inline int bob_file_read(struct BOBFile *file, void *buf, size_t count) {
  goto start;
  for (;;) {
    if (file->written == file->blocksize) {
      file->pos = 0;
      file->written = 0;
    }
    ssize_t rd = bob_sys_read(file->fd, file->buf + file->written,
      file->blocksize - file->written);
    if (rd < 0) {
      return -1;
    }
    if (rd == 0) {
      errno = EILSEQ;
      return -1;
    }
    file->written += (size_t) rd;
  start:
    if (file->pos + count <= file->written) {
      memcpy(buf, file->buf + file->pos, count);
      file->pos += count;
      return 0;
    }
    memcpy(buf, file->buf + file->pos, file->written - file->pos);
    buf += file->written - file->pos;
    count -= file->written - file->pos;
    file->pos = file->written;
  }
}

/**
 * bob_file_read_varint reads a varint from the underlying file.
 *
 * @param file BOB file.
 * @param n Location to store the read varint.
 *
 * @param On success, @c 0 is returned and the read integer is stored at @c *n.
 *  On error, @c -1 is returned, @c *n is unspecified, and @c errno is set
 *  appropriately.
 */
static inline int bob_file_read_varint(struct BOBFile *file, uint64_t *n) {
  int count = 0;
  do {
    uint8_t vbyte;
    if (bob_file_read(file, &vbyte, sizeof(vbyte)) != 0) {
      return -1;
    }
    count = bob_varint_decode(n, vbyte, count);
    if (count < 0) {
      errno = EILSEQ;
      return -1;
    }
  } while (count > 0);
  return 0;
}

/**
 * bob_file_read_header reads in a BOB header.
 *
 * bob_file_read_header assumes that the file pointer is positioned at the
 * start of the header. On success, the file pointer will be pointed just past
 * the header.
 *
 * @param file The BOB file to read the header from.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned, and @c errno is set appropriately.
 */
static inline int bob_file_read_header(struct BOBFile *file) {
  uint64_t n, blocksize = 0, cuesize = 0;
  for (;;) {
    if (bob_file_read_varint(file, &n) != 0) {
      return -1;
    }
    switch (n) {
    case BOB_CONFID_BLOCK_SIZE:
      if (bob_file_read_varint(file, &n) != 0) {
        return -1;
      }
      blocksize = n;
      break;
    case BOB_CONFID_CUE_SIZE:
      if (bob_file_read_varint(file, &n) != 0) {
        return -1;
      }
      cuesize = n;
      break;
    case BOB_CONFID_END:
      goto endfor;
    default:
      errno = EILSEQ;
      return -1;
    }
  }
  endfor:
  /* Check validity */
  if (
    (blocksize < MIN_BLOCK_SIZE) ||
    (blocksize > MAX_BLOCK_SIZE) ||
    (cuesize < blocksize) ||
    (cuesize % blocksize != 0)
  ) {
    errno = EILSEQ;
    return -1;
  }
  if (file->written > blocksize) {
    /* We're going to shrink the buffer, so rewind file accordingly */
    if (bob_sys_lseek(
      file->fd, (off_t) blocksize - (off_t) file->written, SEEK_CUR
    ) != 0) {
      return -1;
    }
    file->written = blocksize;
  }
  if (blocksize != file->blocksize) {
    void *newbuf = realloc(file->buf, blocksize);
    if (newbuf == NULL) {
      errno = ENOMEM;
      return -1;
    }
    file->buf = newbuf;
    file->blocksize = blocksize;
  }
  file->cuesize = cuesize;
  return 0;
}

/**
 * bob_file_parses parses the specified file.
 *
 * @param file BOB file.
 * @param len Pointer where the length of the parsed data is stored.
 *
 * @return On success, a pointer to the BOB data is returned, and the length of
 *  the data is stored in @c *len. The returned pointer may be a null pointer if
 *  @c *len is @c 0.
 *  On error, a null pointer is returned, <code>(size_t) -1</code> is stored in
 *  @c *len, and @c errno is set appropriately.
 */
static inline void* bob_file_parse(struct BOBFile *file, size_t *len) {
  void *data = NULL;
  uint64_t n = 0;
  *len = (size_t) -1;
  for (;;) {
    int rc = bob_file_iseof(file);
    if (rc < 0) {
      free(data);
      return NULL;
    }
    if (rc > 0) {
      /* Reset buffer for write mode */
      file->pos = file->blocksize;
      file->written = file->blocksize;
      *len = n;
      return data;
    }
    if (bob_file_read_varint(file, &n) != 0) {
      free(data);
      return NULL;
    }
    switch (n) {
    case BOB_BLOCKID_REWRITE:
      if (bob_file_read_varint(file, &n) != 0) {
        free(data);
        return NULL;
      }
      void *newdata = realloc(data, n);
      if ((newdata == NULL) && (n > 0)) {
        free(data);
        errno = ENOMEM;
        return NULL;
      }
      data = newdata;
      if (bob_file_read(file, data, n) != 0) {
        free(data);
        return NULL;
      }
      break;
    default:
      errno = EILSEQ;
      return NULL;
    }
  }
}

/**
 * bob_file_create creates a new BOB file.
 *
 * bob_file_create allocates a new BOBFile, opens the file, and prepares writing
 * the file header.
 *
 * @param cfg The BOB configuration.
 * @param path The file system path at which the file should be created.
 *
 * @return On success, a pointer to the new BOB file structure is returned.
 *  On error, a null pointer is returned, and @c errno is set appropriately.
 */
static inline struct BOBFile* bob_file_create(
  struct BOBConfig *cfg, const char *path
) {
  int lerrno;
  struct BOBFile *result = malloc(sizeof(*result));
  if (result == NULL) {
    lerrno = ENOMEM;
    goto nomem;
  }
  result->fd = bob_sys_open(path, O_RDWR|O_CREAT|O_EXCL);
  if (result->fd == -1) {
    lerrno = errno;
    goto nofd;
  }
  result->blocksize = bob_get_real_blocksize(result->fd,
    bob_config_get_blocksize(cfg));
  result->cuesize = bob_get_real_cuesize(result->blocksize,
    bob_config_get_cuesize(cfg));
  if (bob_file_initbuf(result) != 0) {
    lerrno = errno;
    goto badbuf;
  }
  if (bob_file_write_header(result) != 0) {
    lerrno = errno;
    goto badhdr;
  }
  return result;

badhdr:
badbuf:
  bob_sys_close(result->fd);
  bob_sys_unlink(path);
nofd:
  free(result);
nomem:
  errno = lerrno;
  return NULL;
}

/**
 * bob_file_open opens an existing BOB file.
 *
 * bob_file_open allocates a new BOBFile, opens the existing file, and prepares
 * reading it. A successful call to this function @e must be followed by a
 * call to #bob_file_parse, as otherwise, writing to the file will trash it.
 *
 * @param path Path to existing file.
 *
 * @return On success, a pointer to the new BOB file structure is returned.
 *  On error, a null pointer is retruned, and @c errno is set appropriately.
 *
 * @sa bob_file_parse
 */
static inline struct BOBFile* bob_file_open(const char *path) {
  int lerrno;
  struct BOBFile *result = malloc(sizeof(*result));
  if (result == NULL) {
    lerrno = ENOMEM;
    goto nomem;
  }
  /* We don't know the block size yet, so we start with the default size and
   * update later. */
  result->blocksize = DEFAULT_BLOCK_SIZE;
  result->buf = malloc(result->blocksize);
  if (result->buf == NULL) {
    lerrno = ENOMEM;
    goto nobufmem;
  }
  result->pos = 0;
  result->written = 0;
  result->fd = bob_sys_open(path, O_RDWR);
  if (result->fd == -1) {
    lerrno = errno;
    goto nofd;
  }
  /* Seek to start of data */
  if (bob_sys_lseek(result->fd, 0, SEEK_DATA) != 0) {
    lerrno = errno;
    goto nodata;
  }
  if (bob_file_read_header(result) != 0) {
    lerrno = errno;
    goto noheader;
  }
  return result;
noheader:
nodata:
  bob_sys_close(result->fd);
nofd:
  free(result->buf);
nobufmem:
  free(result);
nomem:
  errno = lerrno;
  return NULL;
}

/**
 * bob_file_close closes a BOB file, flushing data to disk.
 *
 * @param file BOB file.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_file_close(struct BOBFile *file) {
  if (file == NULL) {
    errno = EBADF;
    return -1;
  }
  int lerrno = 0;
  if (bob_file_write_commit(file) != 0) {
    lerrno = errno;
  }
  free(file->buf);
  if (bob_sys_fsync(file->fd) != 0) {
    lerrno = errno;
  }
  if (bob_sys_close(file->fd) != 0) {
    lerrno = errno;
  }
  if (lerrno == 0) {
    return 0;
  }
  errno = lerrno;
  return -1;
}

/**
 * bob_file_flush flushes all file data to disk.
 *
 * @param file BOB file.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_file_flush(struct BOBFile *file) {
  if (file == NULL) {
    errno = EBADF;
    return -1;
  }
  int lerrno = 0;
  if (bob_file_write_commit(file) != 0) {
    lerrno = errno;
  }
  if (bob_sys_fsync(file->fd) != 0) {
    lerrno = errno;
  }
  if (lerrno == 0) {
    return 0;
  }
  errno = lerrno;
  return -1;
}

/**
 * bob_file_cue_remaining returns the remaining space in the current
 * cue block.
 *
 * @param file BOB file.
 *
 * @return On success, the remaining cue space in bytes is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline off_t bob_file_cue_remaining(struct BOBFile *file) {
  off_t current = bob_sys_lseek(file->fd, 0, SEEK_CUR);
  if (current < 0) {
    return -1;
  }
  if (current % (off_t) file->cuesize == 0) {
    return 0;
  }
  return (off_t) file->cuesize - (current % (off_t) file->cuesize);
}

/**
 * bob_file_new_cue starts a new cue block.
 *
 * @param file BOB file.
 *
 * @return On success, the start offset of the new cue block is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline off_t bob_file_new_cue(struct BOBFile *file) {
  off_t current = bob_sys_lseek(file->fd, 0, SEEK_CUR);
  if (current < 0) {
    return -1;
  }
  if (current % (off_t) file->cuesize != 0) {
    current += (off_t) file->cuesize - current % (off_t) file->cuesize;
    if (bob_sys_lseek(file->fd, current, SEEK_SET) < 0) {
      return -1;
    }
  }
  file->pos = 0;
  file->written = 0;
  if (bob_file_write_header(file) != 0) {
    return -1;
  }
  return current;
}

/**
 * bob_file_zap cuts a hole into a BOB file.
 *
 * The hole ranges from the beginning of the file until and excluding the
 * specified start offset.
 *
 * @param file The BOB file.
 * @param start_off New start of data.
 *
 * @return On success, @c 0 is returned.
 *  On error, @c -1 is returned and @c errno is set appropriately.
 */
static inline int bob_file_zap(struct BOBFile *file, off_t start_off) {
  return bob_sys_fallocate(file->fd, FALLOC_FL_PUNCH_HOLE|FALLOC_FL_KEEP_SIZE,
    0, start_off);
}

#endif
