#include <errno.h>
#include <stdlib.h>

#include "config.h"

struct BOBConfig* bob_config_default(void) {
  return calloc(1, sizeof(struct BOBConfig));
}

void bob_config_del(struct BOBConfig *cfg) {
  free(cfg);
}

int bob_config_set_blocksize(struct BOBConfig *cfg, unsigned long size) {
  if (cfg == NULL) {
    errno = EINVAL;
    return -1;
  }
  cfg->blocksize = size;
  return 0;
}

unsigned long bob_config_get_blocksize(struct BOBConfig *cfg) {
  if (cfg == NULL) {
    return 0;
  }
  return cfg->blocksize;
}

int bob_config_set_cuesize(struct BOBConfig *cfg, unsigned long size) {
  if (cfg == NULL) {
    errno = EINVAL;
    return -1;
  }
  cfg->cuesize = size;
  return 0;
}

unsigned long bob_config_get_cuesize(struct BOBConfig *cfg) {
  if (cfg == NULL) {
    return 0;
  }
  return cfg->cuesize;
}
