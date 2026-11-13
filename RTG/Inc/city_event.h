/*
 * city_event.h  -  the 100-byte event file and the job that travels inside the EVB
 *
 *   offset  size  field
 *   0       1     department   0 Police, 1 Fire, 2 Ambulance, 3 Maintenance
 *   1       1     severity     0 Critical, 1 Warning, 2 Notice   (smaller = more urgent)
 *   2       80    address      ASCII, '\0' padded
 *   82      18    padding      all 0xFF
 *
 * Pure C - no FreeRTOS / HAL, so it is unit-tested on the PC too.
 */
#ifndef CITY_EVENT_H
#define CITY_EVENT_H

#include <stdint.h>

#define CITY_EVENT_SIZE   100U
#define CITY_ADDR_LEN     80U
#define CITY_PAD_LEN      18U
#define CITY_PAD_BYTE     0xFFU

typedef enum
{
  DEPT_POLICE = 0,
  DEPT_FIRE,
  DEPT_AMBULANCE,
  DEPT_MAINTENANCE,
  DEPT_COUNT
} Department_t;

typedef enum
{
  SEV_CRITICAL = 0,      /* highest precedence */
  SEV_WARNING,
  SEV_NOTICE,
  SEV_COUNT
} Severity_t;

typedef struct __attribute__((packed))
{
  uint8_t department;
  uint8_t severity;
  char    address[CITY_ADDR_LEN];
  uint8_t padding[CITY_PAD_LEN];
} CityEvent_t;

_Static_assert(sizeof(CityEvent_t) == CITY_EVENT_SIZE, "event must be exactly 100 bytes");

/* what is stored in the job-queue DB and sent to the departments */
typedef struct
{
  uint32_t    id;            /* running number, also keeps FIFO order inside a severity */
  uint32_t    rx_ms;         /* when it was downloaded (uptime ms) */
  uint16_t    file_index;    /* eventNNN.bin it came from (0xFFFF = test) */
  CityEvent_t ev;
} DispatchJob_t;

typedef enum
{
  EV_OK = 0,
  EV_BAD_SIZE,
  EV_BAD_DEPT,
  EV_BAD_SEVERITY,
  EV_BAD_ADDRESS,
  EV_BAD_PADDING
} EvStatus_t;

/* check raw bytes from TFTP and copy them into *out */
EvStatus_t  CityEvent_Parse(const uint8_t *raw, uint32_t len, CityEvent_t *out);

/* build an event (used by the tests and the burst test) */
void        CityEvent_Make(CityEvent_t *ev, uint8_t dept, uint8_t sev, const char *address);

/* address as a safe C string (the 80 bytes may have no '\0') */
void        CityEvent_AddressStr(const CityEvent_t *ev, char out[CITY_ADDR_LEN + 1U]);

const char *CityEvent_DeptName(uint8_t dept);
const char *CityEvent_SevName(uint8_t sev);
const char *CityEvent_StatusStr(EvStatus_t st);

#endif /* CITY_EVENT_H */
