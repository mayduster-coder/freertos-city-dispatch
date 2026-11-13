/*
 * department.h  -  requirement 9: 4 departments, each = 1 department task + 3 worker tasks
 *
 *   dispatcher --[inbox]--> deptTask --[message queue]--> worker 1 / 2 / 3
 */
#ifndef DEPARTMENT_H
#define DEPARTMENT_H

#include <stdint.h>
#include "city_event.h"

void     Department_InitAll(void);             /* queues - before the tasks start */
void     Department_StartAll(void);            /* 4 x (1 + 3) tasks */
int      Department_HasRoom(uint8_t dept);
int      Department_Send(uint8_t dept, const DispatchJob_t *job);   /* no wait, 1 = ok */
uint32_t Department_Waiting(uint8_t dept);     /* in inbox + message queue */
uint32_t Department_Busy(uint8_t dept);        /* workers handling an event now */

#endif /* DEPARTMENT_H */
