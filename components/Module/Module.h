#include "nvs_flash.h"

#define N_PERIPHERAL_TYPES 4

extern nvs_handle_t https_nvs_handle;

const char *peripheral_type[] = {
  "hygrometer",
  "thermometer",
  "valve",
  "other",
};

struct peripheral {
  int id;
  const char *p_type;
};

void HandleModuleInit();