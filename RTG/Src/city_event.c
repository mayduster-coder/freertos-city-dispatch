// city_event.c- check events

#include <string.h>
#include "config.h"
#include "city_event.h"

EvStatus_t CityEvent_Parse(const uint8_t *raw, uint32_t len, CityEvent_t *out) // Check and copy function
{
  if (raw == NULL || out == NULL || len != CITY_EVENT_SIZE) return EV_BAD_SIZE;

  const CityEvent_t *in = (const CityEvent_t *)raw;   //all fields are bytes - no alignment issue

  if (in->department >= DEPT_COUNT) return EV_BAD_DEPT; // return if bad data
  if (in->severity   >= SEV_COUNT)  return EV_BAD_SEVERITY;
  if (in->address[0] == '\0')       return EV_BAD_ADDRESS;

#if EVENT_STRICT_PADDING
  for (uint32_t i = 0; i < CITY_PAD_LEN; i++)
    if (in->padding[i] != CITY_PAD_BYTE) return EV_BAD_PADDING; // padding check for 0xFF
#endif

  memcpy(out, raw, CITY_EVENT_SIZE);// copy data to out
  return EV_OK;
}

void CityEvent_Make(CityEvent_t *ev, uint8_t dept, uint8_t sev, const char *address) // function to build event
{
  memset(ev, 0, sizeof *ev); // set bits to 0
  ev->department = dept;
  ev->severity   = sev;
  if (address != NULL)
    strncpy(ev->address, address, CITY_ADDR_LEN);
  memset(ev->padding, CITY_PAD_BYTE, CITY_PAD_LEN);
  // finish building
}

void CityEvent_AddressStr(const CityEvent_t *ev, char out[CITY_ADDR_LEN + 1U]) // safe address to print
{
  memcpy(out, ev->address, CITY_ADDR_LEN); // copy address
  out[CITY_ADDR_LEN] = '\0';
}

const char *CityEvent_DeptName(uint8_t dept) // create dept name
{
  static const char *const names[DEPT_COUNT] = { "Police", "Fire", "Ambulance", "Maintenance" };
  return (dept < DEPT_COUNT) ? names[dept] : "?";
}

const char *CityEvent_SevName(uint8_t sev) // create priority name
{
  static const char *const names[SEV_COUNT] = { "CRITICAL", "WARNING", "NOTICE" };
  return (sev < SEV_COUNT) ? names[sev] : "?";
}

const char *CityEvent_StatusStr(EvStatus_t st)// translate error code to message
{
  switch (st)
  {
    case EV_OK:           return "ok";
    case EV_BAD_SIZE:     return "size != 100";
    case EV_BAD_DEPT:     return "bad department";
    case EV_BAD_SEVERITY: return "bad severity";
    case EV_BAD_ADDRESS:  return "empty address";
    case EV_BAD_PADDING:  return "padding not 0xFF";
    default:              return "?";
  }
}
