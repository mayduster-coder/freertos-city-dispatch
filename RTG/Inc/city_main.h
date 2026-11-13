/*
 * city_main.h  -  one entry point for freertos.c
 */
#ifndef CITY_MAIN_H
#define CITY_MAIN_H

/* call from StartDefaultTask, right AFTER MX_LWIP_Init() */
void App_Start(void);

#endif /* CITY_MAIN_H */
