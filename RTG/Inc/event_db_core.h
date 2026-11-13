/*
 * event_db_core.h  -  the job-queue "Data Base" logic (pure C, unit-tested on the PC)
 *
 * Why this structure (requirement 4 + 7):
 *   - A fixed array of slots in RAM: no malloc, bounded size, every event
 *     can be read / counted / searched directly ("easy access").
 *   - PopBest() returns the most urgent event: lowest severity value first,
 *     and inside the same severity the oldest one (smallest id) -> FIFO.
 *   - PopBest() takes a mask of departments that have room right now, so a
 *     busy department never blocks events for the other departments.
 *   With 32 slots a linear scan is a few hundred cycles - simpler and safer
 *   than a heap or linked lists.
 *
 * NOT thread safe by itself - event_db.c wraps it with a mutex.
 */
#ifndef EVENT_DB_CORE_H
#define EVENT_DB_CORE_H

#include <stdint.h>
#include "config.h"
#include "city_event.h"

#define DEPT_MASK_ALL   ((uint8_t)((1U << DEPT_COUNT) - 1U))

typedef struct
{
  DispatchJob_t job;
  uint8_t       used;
} EvDbSlot_t;

typedef struct
{
  EvDbSlot_t slot[EVENT_DB_CAPACITY];
  uint32_t   count;
} EvDbCore_t;

void     EvDbCore_Init(EvDbCore_t *db);
int      EvDbCore_Push(EvDbCore_t *db, const DispatchJob_t *job);           /* 1 ok, 0 full */
int      EvDbCore_PopBest(EvDbCore_t *db, uint8_t dept_mask, DispatchJob_t *out); /* 1 got one */
uint32_t EvDbCore_Count(const EvDbCore_t *db);
uint32_t EvDbCore_CountSeverity(const EvDbCore_t *db, uint8_t sev);
uint32_t EvDbCore_CountDept(const EvDbCore_t *db, uint8_t dept);

#endif /* EVENT_DB_CORE_H */
