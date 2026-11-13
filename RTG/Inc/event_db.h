/*
 * event_db.h  -  thread-safe job-queue DB (event_db_core + a FreeRTOS mutex)
 *
 * Writers: downloaderTask (and the console burst test)
 * Reader : dispatcherTask
 */
#ifndef EVENT_DB_H
#define EVENT_DB_H

#include <stdint.h>
#include "city_event.h"

void     EventDb_Init(void);                                         /* before the tasks start */
int      EventDb_Push(const DispatchJob_t *job);                     /* 1 ok, 0 full */
uint32_t EventDb_PushBatch(const DispatchJob_t *jobs, uint32_t n);   /* all under one lock */
int      EventDb_PopBest(uint8_t dept_mask, DispatchJob_t *out);     /* 1 got one */
uint32_t EventDb_Count(void);
uint32_t EventDb_CountSeverity(uint8_t sev);
uint32_t EventDb_Capacity(void);

#endif /* EVENT_DB_H */
