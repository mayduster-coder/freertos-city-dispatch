/* host_main.c - runs the same unit tests as the 't' key, on the PC */
#include <stdio.h>
#include <stdarg.h>
#include "tests.h"

static void host_log(const char *fmt, ...)
{
  va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

int main(void)
{
  return Tests_RunCore(host_log) == 0 ? 0 : 1;
}
