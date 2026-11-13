/*
 * downloader.h  -  downloaderTask: TFTP -> validate -> job-queue DB, one event every period
 */
#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdint.h>
#include "config.h"

void     Downloader_Start(void);
uint32_t Downloader_GetPeriod(void);
uint32_t Downloader_Faster(void);      /* '+' : period - 10 ms, returns the new period */
uint32_t Downloader_Slower(void);      /* '-' : period + 10 ms */
int      Downloader_TogglePause(void); /* returns 1 if now paused */
int      Downloader_IsPaused(void);
void     Downloader_SetVerbose(int on);
int      Downloader_GetVerbose(void);

/* pure helper (unit-tested): clamp a new period into [MIN, MAX] */
static inline uint32_t Downloader_ClampPeriod(int32_t ms)
{
  if (ms < (int32_t)DOWNLOAD_PERIOD_MIN_MS) return DOWNLOAD_PERIOD_MIN_MS;
  if (ms > (int32_t)DOWNLOAD_PERIOD_MAX_MS) return DOWNLOAD_PERIOD_MAX_MS;
  return (uint32_t)ms;
}

#endif /* DOWNLOADER_H */
