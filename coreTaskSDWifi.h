#ifndef CORETASKSDWIFI_H
#define CORETASKSDWIFI_H

typedef struct {
  String buffer;
  int bufferCount;
} coreTaskSDWifiHandleParams_t;

extern TaskHandle_t coreTaskSDWifiHandle;
void coreTaskSDWifiFunction(void *parameter);

#endif