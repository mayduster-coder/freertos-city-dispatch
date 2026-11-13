// event data base
#include <string.h>
#include "event_db_core.h"

void EvDbCore_Init(EvDbCore_t *db) // set db struct
{
  memset(db, 0, sizeof *db);
}

int EvDbCore_Push(EvDbCore_t *db, const DispatchJob_t *job)// push event to data base
{
  if (db->count >= EVENT_DB_CAPACITY) return 0;

  for (uint32_t i = 0; i < EVENT_DB_CAPACITY; i++)
  {
    if (!db->slot[i].used)// if there is a spot in this event
    {
      db->slot[i].job  = *job;
      db->slot[i].used = 1;
      db->count++;
      return 1;
    }
  }
  return 0;   //not reached
}

//in case of a is "more urgent" than b?
static int better(const DispatchJob_t *a, const DispatchJob_t *b)
{
  if (a->ev.severity != b->ev.severity)
    return a->ev.severity < b->ev.severity;// Critical(0) < Warning(1) < Notice(2)
  return (int32_t)(a->id - b->id) < 0;//older first, safe when id wraps
}

int EvDbCore_PopBest(EvDbCore_t *db, uint8_t dept_mask, DispatchJob_t *out) //finds most urgent task
{
  int best = -1; // holds current number

  for (uint32_t i = 0; i < EVENT_DB_CAPACITY; i++) // going through 32 places
  {
    if (!db->slot[i].used) continue;
    if ((dept_mask & (1U << db->slot[i].job.ev.department)) == 0U) continue;   // dept busy?
    if (best < 0 || better(&db->slot[i].job, &db->slot[best].job))
      best = (int)i;
  }
  if (best < 0) return 0;

  *out = db->slot[best].job;// get urgent task
  db->slot[best].used = 0;
  db->count--;
  return 1;
}

uint32_t EvDbCore_Count(const EvDbCore_t *db)// how many events we have
{
  return db->count;
}

uint32_t EvDbCore_CountSeverity(const EvDbCore_t *db, uint8_t sev)// how many events we have from certain severity
{
  uint32_t n = 0;
  for (uint32_t i = 0; i < EVENT_DB_CAPACITY; i++)
    if (db->slot[i].used && db->slot[i].job.ev.severity == sev) n++;
  return n;
}

uint32_t EvDbCore_CountDept(const EvDbCore_t *db, uint8_t dept)
{
  uint32_t n = 0;
  for (uint32_t i = 0; i < EVENT_DB_CAPACITY; i++)
    if (db->slot[i].used && db->slot[i].job.ev.department == dept) n++;// how many events we have from certain dept
  return n;
}
