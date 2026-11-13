/*
 * dispatcher.h  -  dispatcherTask: job-queue DB --(severity first)--> department
 */
#ifndef DISPATCHER_H
#define DISPATCHER_H

void Dispatcher_Start(void);
void Dispatcher_Wake(void);          /* "something changed" - call from tasks only */
void Dispatcher_SetVerbose(int on);

#endif /* DISPATCHER_H */
