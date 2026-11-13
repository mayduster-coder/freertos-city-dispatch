//The file maintains counters: how many events were downloaded, how many were sent to each department, and how many were processed.
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "city_stats.h"

static AppStats_t st; // holds all counters

void Stats_Reset(void) // reset st
{
  taskENTER_CRITICAL();// block interrupts to set st
  memset(&st, 0, sizeof st);
  taskEXIT_CRITICAL();
}

void Stats_IncSent(uint8_t dept, uint8_t sev) // dispatcher calls this function to add dept and the severity
{
  if (dept >= DEPT_COUNT || sev >= SEV_COUNT) return;
  taskENTER_CRITICAL();
  st.sent[dept]++;
  st.sent_by_sev[sev]++;
  taskEXIT_CRITICAL();
}

void Stats_IncHandled(uint8_t dept) // add 1 to handle events
{
  if (dept >= DEPT_COUNT) return;
  taskENTER_CRITICAL();
  st.handled[dept]++;
  taskEXIT_CRITICAL();
}
// add 1 to other sections
void Stats_IncDownloaded(void) { taskENTER_CRITICAL(); st.downloaded++;  taskEXIT_CRITICAL(); }
void Stats_IncRejected(void)   { taskENTER_CRITICAL(); st.rejected++;    taskEXIT_CRITICAL(); }
void Stats_IncTftpError(void)  { taskENTER_CRITICAL(); st.tftp_errors++; taskEXIT_CRITICAL(); }
void Stats_IncDbFull(void)     { taskENTER_CRITICAL(); st.db_full++;     taskEXIT_CRITICAL(); }

void Stats_Get(AppStats_t *out)// calls all counters
{
  taskENTER_CRITICAL();
  *out = st;
  taskEXIT_CRITICAL();
}
