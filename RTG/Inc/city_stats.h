/*
 * city_stats.h  -  counters (requirement 8: events sent per department)
 */
#ifndef CITY_STATS_H
#define CITY_STATS_H

#include <stdint.h>
#include "city_event.h"

typedef struct
{
  uint32_t sent[DEPT_COUNT];         /* dispatcher -> department */
  uint32_t handled[DEPT_COUNT];      /* finished by a worker */
  uint32_t sent_by_sev[SEV_COUNT];
  uint32_t downloaded;               /* valid events from TFTP */
  uint32_t rejected;                 /* file arrived but the event was bad */
  uint32_t tftp_errors;              /* timeouts etc. */
  uint32_t db_full;                  /* times the downloader had to wait */
} AppStats_t;

void Stats_Reset(void);
void Stats_IncSent(uint8_t dept, uint8_t sev);
void Stats_IncHandled(uint8_t dept);
void Stats_IncDownloaded(void);
void Stats_IncRejected(void);
void Stats_IncTftpError(void);
void Stats_IncDbFull(void);
void Stats_Get(AppStats_t *out);     /* consistent copy */

#endif /* CITY_STATS_H */
