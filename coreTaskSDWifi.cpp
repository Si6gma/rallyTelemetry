#include "coreTaskSDWifi.h"
#include "storage.h"

TaskHandle_t coreTaskSDWifiHandle;

void coreTaskSDWifiFunction(void *voidParams) {
  coreTaskSDWifiHandleParams_t *params = (coreTaskSDWifiHandleParams_t *)voidParams;

  if (!SDFileExists("live.csv")) {
    SDFileWriteln("live.csv", logHeaderData());
  }

  while (1) {
    params->buffer
  }
}
