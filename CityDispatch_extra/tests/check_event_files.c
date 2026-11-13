/* check_event_files.c - parse event files with the SAME C code the EVB uses.
   build: see run_host_tests.sh   usage: ./check_event_files events/event000.bin ...            */
#include <stdio.h>
#include "city_event.h"

int main(int argc, char **argv)
{
  int bad = 0;
  for (int i = 1; i < argc; i++)
  {
    unsigned char raw[200];
    FILE *f = fopen(argv[i], "rb");
    if (!f) { printf("%s: cannot open\n", argv[i]); bad++; continue; }
    size_t n = fread(raw, 1, sizeof raw, f);
    fclose(f);

    CityEvent_t ev;
    char addr[CITY_ADDR_LEN + 1];
    EvStatus_t st = CityEvent_Parse(raw, (uint32_t)n, &ev);
    if (st == EV_OK)
    {
      CityEvent_AddressStr(&ev, addr);
      printf("%-24s OK   %-11s %-8s %s\n", argv[i], CityEvent_DeptName(ev.department),
             CityEvent_SevName(ev.severity), addr);
    }
    else
    {
      printf("%-24s BAD  %s\n", argv[i], CityEvent_StatusStr(st));
      bad++;
    }
  }
  return bad;
}
