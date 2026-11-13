#!/bin/sh
# Run the unit tests on the PC (Linux / WSL / Git-Bash with gcc)
cd "$(dirname "$0")/../.." && E=CityDispatch_extra
gcc -std=c11 -Wall -Wextra -Werror -I RTG/Inc \
    $E/tests/host_main.c RTG/Src/tests.c RTG/Src/city_event.c \
    RTG/Src/event_db_core.c RTG/Src/tftp_proto.c -o $E/tests/host_tests && ./$E/tests/host_tests
gcc -std=c11 -Wall -Wextra -Werror -I RTG/Inc $E/tests/check_event_files.c RTG/Src/city_event.c \
    -o $E/tests/check_event_files && ./$E/tests/check_event_files $E/events/*.bin | tail -3
