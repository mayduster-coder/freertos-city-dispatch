/*
 * tests.h  -  requirement 11: test cases
 *
 * Tests_RunCore() checks the pure modules (event parsing, job DB order,
 * TFTP packets, period clamp). The SAME code runs:
 *   - on the PC:  tests/run_host_tests.sh
 *   - on the EVB: press 't' in the terminal
 */
#ifndef TESTS_H
#define TESTS_H

#include <stdint.h>

typedef void (*TestLogFn)(const char *fmt, ...);

/* returns the number of FAILED checks (0 = all good) */
uint32_t Tests_RunCore(TestLogFn log);

#endif /* TESTS_H */
