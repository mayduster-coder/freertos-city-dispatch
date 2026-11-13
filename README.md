# City Dispatch — Real-Time Event Dispatcher

A **FreeRTOS** application on an **STM32F756 (ARM Cortex-M7)** that downloads emergency events from a PC server over **TFTP/UDP (lwIP)** and dispatches them to multiple departments through a priority-aware, mutex-protected job queue. Written with the **native FreeRTOS API**.

---

## Overview

The system simulates a city emergency-dispatch center on embedded hardware:

- A **TFTP client** (over lwIP UDP) pulls a 100-byte event from a PC server every 100 ms.
- Incoming events are placed into a **priority job queue** that serves **Critical events first** (FIFO within each severity level).
- Events are routed to **4 departments**, each served by a pool of **3 worker tasks**.
- **Back-pressure** ensures that a busy department never blocks the others.
- An **interrupt-driven UART console** (ISR → queue) provides live statistics and download-rate control.

## Architecture

| Element | Role |
|---|---|
| **TFTP client** | RRQ / DATA / ACK over lwIP UDP, with retransmission and error handling |
| **Job queue** | Mutex-protected, priority-ordered (Critical first, FIFO within severity) |
| **Departments** | 4 departments × 3-task worker pool each |
| **UART console** | Interrupt-driven (ISR to queue) for live stats and control |
| **Health** | Heartbeat LED, stack-overflow & out-of-memory hooks |

Built as a **20-task** FreeRTOS application using the **native API** (tasks, queues, mutexes, task notifications) — minimal CMSIS-RTOS.

## Highlights

- **TFTP client from scratch** over lwIP UDP (RRQ / DATA / ACK, retransmission, error handling)
- **Priority scheduling with back-pressure** — avoids head-of-line blocking between departments
- **Interrupt-to-task design** for the UART console (short ISR, work deferred to a task)
- **Robustness**: stack-overflow and malloc-failed hooks, heartbeat LED
- **Tested**: 47 unit tests that run both on the board and on a PC, plus a FreeRTOS simulation on Linux

## Key Concepts Demonstrated

- FreeRTOS native API: tasks, priorities, queues, mutexes, task notifications, hooks
- Worker-pool concurrency pattern with back-pressure
- Embedded networking: lwIP, UDP, TFTP protocol
- Deferred interrupt processing (ISR → queue → task)
- Host-based unit testing for embedded code

## Running the TFTP Server

The event source is a TFTP server on the PC (e.g. **Tftpd64** on Windows, base folder `C:\tftp`). The EVB connects over Ethernet and requests events via TFTP.

## Toolchain

- **STM32CubeIDE** / CubeMX (HAL, FreeRTOS, lwIP)
- **GCC** for ARM (arm-none-eabi)
- Language: **C**

## Author

**May Duster** — Embedded Software Engineer
[LinkedIn](https://www.linkedin.com/in/may-duster-201914241) · [GitHub](https://github.com/mayduster-coder)
