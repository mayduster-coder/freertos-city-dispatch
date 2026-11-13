/*
 * log.h  -  printf-style logging to the debugger UART, safe from many tasks (mutex)
 */
#ifndef LOG_H
#define LOG_H

void App_Log_Init(void);
void App_Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* LOG_H */
