
/*
 * config.h  -  City Dispatch: every setting in one place
 *
 * Board: NUCLEO-F756ZG / F746ZG, FreeRTOS (native API) + lwIP
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ========================= Network / TFTP ========================= */
#define APP_SERVER_IP              "192.168.1.10"   /* PC Ethernet port (direct cable to the board) */
#define EVENT_FILE_FMT             "event%03u.bin"  /* event000.bin, event001.bin ... */
#define EVENT_FILE_FIRST           0U
#define EVENT_WRAP_AROUND          1    /* 1 = after the last file start again from the first */

#define TFTP_TIMEOUT_MS            500U /* wait for one DATA packet */
#define TFTP_MAX_RETRIES           3U   /* resend RRQ / last ACK */
#define TFTP_VERBOSE               0    /* 1 = print every TFTP step */

/* ========================= Download rate ========================= */
#define DOWNLOAD_PERIOD_DEFAULT_MS 100U /* requirement 5: one event every 100 ms */
#define DOWNLOAD_PERIOD_STEP_MS    10U  /* bonus: '+' / '-' change it by 10 ms */
#define DOWNLOAD_PERIOD_MIN_MS     10U
#define DOWNLOAD_PERIOD_MAX_MS     2000U
#define DOWNLOAD_EMPTY_RETRY_MS    1000U /* no files on the server -> try again later */

/* ========================= Job queue DB ========================= */
#define EVENT_DB_CAPACITY          32U  /* events waiting to be dispatched */
#define EVENT_STRICT_PADDING       1    /* 1 = reject events whose padding is not all 0xFF */

/* ========================= Departments ========================= */
#define DEPT_INBOX_LEN             1U   /* dispatcher -> department task */
#define DEPT_WORK_QUEUE_LEN        3U   /* department task -> thread pool (message queue) */
#define DEPT_POOL_SIZE             3U   /* workers per department (requirement 9b) */
#define DEPT_WORK_TIME_MS          300U /* busy wait per event (requirement 9c) */

/* ========================= Tasks: priorities (native FreeRTOS) =========================
 * Workers BUSY-WAIT, so they must be the lowest - otherwise they starve everybody.
 * lwIP's tcpip thread (created by CubeMX) is above all of these.               */
#define PRIO_HEARTBEAT             (tskIDLE_PRIORITY + 4U)
#define PRIO_DISPATCHER            (tskIDLE_PRIORITY + 3U)
#define PRIO_DEPT_MANAGER          (tskIDLE_PRIORITY + 3U)
#define PRIO_DOWNLOADER            (tskIDLE_PRIORITY + 2U)
#define PRIO_CONSOLE               (tskIDLE_PRIORITY + 2U)
#define PRIO_WORKER                (tskIDLE_PRIORITY + 1U)

/* stack sizes in WORDS (x4 = bytes) */
#define STACK_HEARTBEAT            128U
#define STACK_DISPATCHER           384U
#define STACK_DEPT_MANAGER         256U
#define STACK_DOWNLOADER           512U
#define STACK_CONSOLE              512U
#define STACK_WORKER               384U

#define DISPATCH_IDLE_POLL_MS      100U /* dispatcher also re-checks by itself this often */

/* ========================= Board ========================= */
#define HEARTBEAT_PORT             GPIOB        /* LD1 green */
#define HEARTBEAT_PIN              GPIO_PIN_0
#define HEARTBEAT_PERIOD_MS        500U

#define LOG_UART                   huart3       /* ST-LINK virtual COM port */
#define LOG_LINE_MAX               200U

/* 1 = console.c enables the USART3 interrupt and defines USART3_IRQHandler itself.
 * Set to 0 ONLY if you enable "USART3 global interrupt" in CubeMX
 * (otherwise you get "multiple definition of USART3_IRQHandler"). */
#define CONSOLE_OWN_UART_IRQ       1
#define CONSOLE_UART_IRQ_PRIO      6            /* >= 5: allowed to call FreeRTOS FromISR */

#endif /* CONFIG_H */
