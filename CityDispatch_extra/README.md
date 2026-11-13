# City Dispatch - FreeRTOS SE Final Project

NUCLEO-F756ZG / F746ZG, FreeRTOS (native API only - `xTaskCreate`, `xQueue*`, mutex,
task notifications) + lwIP. CMSIS-RTOS appears only in code CubeMX generates.

```
 PC: Tftpd64 (C:\tftp\event000.bin ...)
        |  TFTP over UDP (RRQ / DATA / ACK)
        v
 downloaderTask ---- every 100 ms ('+'/'-' = +-10 ms) ----+
        | CityEvent_Parse (size, dept, severity, address, padding)
        v
 [ job-queue DB ]  32 slots in RAM, mutex
        | PopBest(): Critical > Warning > Notice, oldest first,
        |            only departments that have room
        v
 dispatcherTask ---------------------------------------------------------------+
   |                      |                       |                            |
 [inbox]               [inbox]                 [inbox]                      [inbox]
 policeTask            fireTask                ambulTask                    maintTask
 [message queue]       [message queue]         [message queue]              [message queue]
 police1 2 3           fire1 2 3               ambul1 2 3                   maint1 2 3
 (print + busy wait 300 ms, then block)

 consoleTask: s + - p b t v r h        heartbeatTask: LD1 blinks every 500 ms
```

## Requirements -> code

| # | Requirement | Where |
|---|-------------|-------|
| 1 | TFTP server holds the events | Tftpd64, `C:\tftp`, files from `tools/gen_events.py` |
| 2 | TFTP client on the EVB with lwIP | `tftp_proto.c` (packets), `tftp_client.c` (netconn UDP) |
| 3 | 100-byte event struct | `city_event.h` (`_Static_assert` size == 100) |
| 4 | job-queue DB with easy access | `event_db_core.c` (array + PopBest), `event_db.c` (mutex) |
| 5 | one event every 100 ms | `downloader.c` (period measured start-to-start) |
| 6 | route by department field | `dispatcher.c` -> `Department_Send()` |
| 7 | severity precedence | `EvDbCore_PopBest()` + short department queues |
| 8 | count per department, print on request | `city_stats.c`, key `s` |
| 9a | department task -> department message queue | `dept_task()` in `department.c` |
| 9b | thread pool of 3 per department | `worker_task()` x 3, `DEPT_POOL_SIZE` |
| 9c | print + busy wait 300 ms | `worker_task()`, `busy_wait_ms()` |
| 9d | sleep when idle | all tasks block on queues / notifications (`portMAX_DELAY`) |
| 10 | heartbeat LED | `heartbeat.c` (LD1, highest app priority) |
| 11 | test cases | `tests.c` (boot + key `t`), `tests/` (PC), key `b` |
| 12 | bonus `+` / `-` | `console.c` -> `Downloader_Faster/Slower()` |

## Why the DB is an array and the department queues are short

- **Requirement 7.** Severity order can only be applied while events still wait in one shared place.
  - If the department queues were long, every event would leave the DB immediately. Each queue is FIFO, so a Critical event would then wait behind Notices.
  - The queues are therefore 1 (inbox) + 3 (message queue). When a department is busy, its events stay in the DB, where `PopBest()` picks the most urgent one.
- **A busy department doesn't block the others.** `PopBest()` gets a mask of the departments that have room right now. So if Fire is full, the dispatcher can still send a Police event.
- **Why not a heap or linked lists.** 32 slots with a linear scan is a few hundred cycles. There's no `malloc`, and it's easy to search and count (`CountSeverity`, `CountDept`).

## Priorities (native FreeRTOS, 0 = idle)

| Prio | Task | Why |
|------|------|-----|
| 4 | heartbeat | if LD1 stops, something is stuck |
| 3 | dispatcher, 4 department tasks | short work, keep events flowing |
| 2 | downloader, console | network + keys |
| 1 | 12 workers | they **busy-wait**. If they were higher, they would starve everyone. |
| (lwIP tcpip thread is created by CubeMX above these) | | |

## Terminal keys (115200 8N1, ST-LINK COM port)

| Key | Action |
|-----|--------|
| `s` | statistics: sent / handled / waiting / busy per department, DB content, heap |
| `+` | download faster (period - 10 ms, min 10) |
| `-` | download slower (period + 10 ms, max 2000) |
| `p` | pause / resume the downloader |
| `b` | burst test: 9 Maintenance events N N W N C W C N W, expected handling C C W W W N N N N |
| `t` | run the 47 unit checks on the board |
| `v` | verbose on/off (download + dispatch lines; department lines always print) |
| `r` | reset statistics |
| `h` | help |

## Test cases

1. **Unit tests, 47 checks.** `tests.c` covers the event layout and offsets, parsing (good and bad size / department / severity / address / padding), DB severity order, FIFO inside a severity (also when slots are reused), department mask, DB full, id wrap-around, exact TFTP packet bytes, and period clamp.
   - On the board they run at boot and with the `t` key.
   - On a PC run `tests/run_host_tests.sh`.
2. **Event files cross-check.** `tests/check_event_files` parses the generated files with the same C parser the board uses. The `--bad` files must be rejected.
3. **Burst test on the board.** Press `b` and watch the order in the terminal.
4. **PC simulation with real FreeRTOS.** `tests/sim/run_sim.sh` runs the real DB, dispatcher, department, stats and log code on the FreeRTOS POSIX port. It has three phases:
   - **A:** 100 ms downloads. Every event sent equals every event handled.
   - **B:** the burst. Order is C C W W W N N N N.
   - **C:** overload at 10 ms. Critical waits about 0.5 s on average and Notice about 1.8 s.

   Example output: `tests/sim/sim_output_example.txt`.
5. **Rejection test on the board.** Generate with `--bad` and the terminal shows `REJECTED: bad department` and the other reasons.

## CubeMX checklist

See `Core_snippets/freertos_c_changes.txt` for the 2 lines in `freertos.c`.

- **ETH / LWIP.** Same as your working matrix project: RMII, LAN8742, LWIP enabled, DHCP off.
  - Use a static IP, for example `10.0.0.50 / 255.255.255.0`, with the gateway set to your router.
  - **LWIP_SO_RCVTIMEO = Enabled** (Key Options -> Show Advanced). Otherwise the code refuses to compile, on purpose.
- **FreeRTOS (CMSIS_V1).**
  - TOTAL_HEAP_SIZE >= 65536
  - defaultTask stack 512 words
  - CHECK_FOR_STACK_OVERFLOW = Option2
  - USE_MALLOC_FAILED_HOOK = Enabled
- **USART3.** 115200, and NVIC "USART3 global interrupt" enabled.
- **LD1 (PB0).** GPIO output (Nucleo default).
- **Tftpd64 on the PC.**
  - Base directory `C:\tftp` holding `event000.bin ...`.
  - Firewall rule allowing tftpd64.exe on Private networks.
