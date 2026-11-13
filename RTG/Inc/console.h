/*
 * console.h  -  keys from the debugger terminal (USART3, interrupt, 1 byte at a time)
 *
 *   s  statistics (requirement 8)     +  download faster (-10 ms)   -  slower (+10 ms)
 *   p  pause / resume download         b  burst test (severity order)
 *   t  self test                       v  verbose on/off             r  reset statistics
 *   h  help
 */
#ifndef CONSOLE_H
#define CONSOLE_H

void Console_Init(void);     /* queue - before the tasks start */
void Console_Start(void);

#endif /* CONSOLE_H */
