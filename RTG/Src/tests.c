//tests.c  - Tests to prove that the code is correct

#include <stddef.h>
#include <string.h>
#include "config.h"
#include "city_event.h"
#include "event_db_core.h"
#include "tftp_proto.h"
#include "downloader.h"
#include "tests.h"

static TestLogFn tlog;
static uint32_t  n_pass, n_fail;

#define CHECK(cond, name)                                         \
  do {                                                            \
    if (cond) { n_pass++; }                                       \
    else      { n_fail++; tlog("  FAIL: %s (line %d)\r\n", name, __LINE__); } \
  } while (0)

static DispatchJob_t mk_job(uint32_t id, uint8_t dept, uint8_t sev)// build events with id
{
  DispatchJob_t j;
  memset(&j, 0, sizeof j);
  j.id = id;
  j.file_index = 0xFFFFU;
  CityEvent_Make(&j.ev, dept, sev, "Test street 1");
  return j;
}

/* ---------------- event format ---------------- */
static void test_event_layout(void)// memory struct
{
  CHECK(sizeof(CityEvent_t) == 100U,                     "event is 100 bytes");
  CHECK(offsetof(CityEvent_t, department) == 0U,         "department at offset 0");
  CHECK(offsetof(CityEvent_t, severity)   == 1U,         "severity at offset 1");
  CHECK(offsetof(CityEvent_t, address)    == 2U,         "address at offset 2");
  CHECK(offsetof(CityEvent_t, padding)    == 82U,        "padding at offset 82");
}

static void test_event_parse(void)//test event
{
  CityEvent_t ev, out;
  uint8_t raw[101];

  CityEvent_Make(&ev, DEPT_FIRE, SEV_WARNING, "Herzl 10, Tel Aviv");
  memcpy(raw, &ev, 100);
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_OK,                 "valid event parses");
  CHECK(out.department == DEPT_FIRE && out.severity == SEV_WARNING, "fields copied");
  CHECK(strcmp(out.address, "Herzl 10, Tel Aviv") == 0,           "address copied");

  CHECK(CityEvent_Parse(raw, 99,  &out) == EV_BAD_SIZE,           "99 bytes rejected");
  CHECK(CityEvent_Parse(raw, 101, &out) == EV_BAD_SIZE,           "101 bytes rejected");
  CHECK(CityEvent_Parse(NULL, 100, &out) == EV_BAD_SIZE,          "NULL rejected");

  raw[0] = DEPT_COUNT;
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_BAD_DEPT,           "department 4 rejected");
  raw[0] = DEPT_POLICE; raw[1] = SEV_COUNT;
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_BAD_SEVERITY,       "severity 3 rejected");
  raw[1] = SEV_NOTICE; raw[2] = '\0';
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_BAD_ADDRESS,        "empty address rejected");
  raw[2] = 'H';
#if EVENT_STRICT_PADDING
  raw[99] = 0x00;
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_BAD_PADDING,        "padding != 0xFF rejected");
  raw[99] = 0xFF;
#endif
  CHECK(CityEvent_Parse(raw, 100, &out) == EV_OK,                 "fixed event parses again");
}

static void test_event_address_80(void) //test address
{
  CityEvent_t ev;
  char s[CITY_ADDR_LEN + 1U];
  char longaddr[CITY_ADDR_LEN + 10U];
  memset(longaddr, 'A', sizeof longaddr - 1U);
  longaddr[sizeof longaddr - 1U] = '\0';

  CityEvent_Make(&ev, DEPT_POLICE, SEV_CRITICAL, longaddr);     /* 89 chars -> cut to 80 */
  CHECK(ev.padding[0] == 0xFF,                    "long address does not touch padding");
  CityEvent_AddressStr(&ev, s);
  CHECK(strlen(s) == CITY_ADDR_LEN,               "80-char address without '\\0' is safe");
}

/* ---------------- job DB ---------------- */
static void test_db_severity_order(void)
{
  static EvDbCore_t db;
  DispatchJob_t j, o;
  EvDbCore_Init(&db);

  j = mk_job(1, DEPT_POLICE, SEV_NOTICE);   EvDbCore_Push(&db, &j);
  j = mk_job(2, DEPT_POLICE, SEV_WARNING);  EvDbCore_Push(&db, &j);
  j = mk_job(3, DEPT_POLICE, SEV_CRITICAL); EvDbCore_Push(&db, &j);
  CHECK(EvDbCore_Count(&db) == 3U, "3 events stored");
  CHECK(EvDbCore_CountSeverity(&db, SEV_CRITICAL) == 1U, "count by severity");

  CHECK(EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o) && o.id == 3U, "Critical first");
  CHECK(EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o) && o.id == 2U, "then Warning");
  CHECK(EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o) && o.id == 1U, "then Notice");
  CHECK(!EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o),              "empty DB returns 0");
}

static void test_db_fifo_same_severity(void)
{
  static EvDbCore_t db;
  DispatchJob_t j, o;
  EvDbCore_Init(&db);

  for (uint32_t i = 10; i < 15; i++) { j = mk_job(i, DEPT_FIRE, SEV_WARNING); EvDbCore_Push(&db, &j); }
  /* free a slot in the middle and refill it with a NEWER event -> order must stay by id */
  CHECK(EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o) && o.id == 10U, "oldest Warning first");
  j = mk_job(15, DEPT_FIRE, SEV_WARNING); EvDbCore_Push(&db, &j);   /* goes into slot 0 */

  uint32_t expect = 11, ok = 1;
  while (EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o)) { if (o.id != expect++) ok = 0; }
  CHECK(ok && expect == 16U, "FIFO inside a severity even when slots are reused");
}

static void test_db_dept_mask(void)
{
  static EvDbCore_t db;
  DispatchJob_t j, o;
  EvDbCore_Init(&db);

  j = mk_job(1, DEPT_FIRE,   SEV_CRITICAL); EvDbCore_Push(&db, &j);
  j = mk_job(2, DEPT_POLICE, SEV_NOTICE);   EvDbCore_Push(&db, &j);

  /* Fire is busy -> Police must still get its event */
  CHECK(EvDbCore_PopBest(&db, (uint8_t)(1U << DEPT_POLICE), &o) && o.id == 2U,
        "busy department does not block the others");
  CHECK(!EvDbCore_PopBest(&db, (uint8_t)(1U << DEPT_POLICE), &o), "nothing more for Police");
  CHECK(EvDbCore_PopBest(&db, (uint8_t)(1U << DEPT_FIRE), &o) && o.id == 1U,
        "Fire gets its event when free");
  CHECK(!EvDbCore_PopBest(&db, 0, &o), "mask 0 returns nothing");
}

static void test_db_full(void)
{
  static EvDbCore_t db;
  DispatchJob_t j;
  EvDbCore_Init(&db);

  uint32_t ok = 0;
  for (uint32_t i = 0; i < EVENT_DB_CAPACITY; i++) { j = mk_job(i, DEPT_AMBULANCE, SEV_NOTICE); ok += (uint32_t)EvDbCore_Push(&db, &j); }
  CHECK(ok == EVENT_DB_CAPACITY,                "DB takes CAPACITY events");
  CHECK(!EvDbCore_Push(&db, &j),                "push into a full DB fails");
  CHECK(EvDbCore_Count(&db) == EVENT_DB_CAPACITY, "count stays at CAPACITY");
  CHECK(EvDbCore_CountDept(&db, DEPT_AMBULANCE) == EVENT_DB_CAPACITY, "count by department");
}

static void test_db_id_wrap(void)
{
  static EvDbCore_t db;
  DispatchJob_t j, o;
  EvDbCore_Init(&db);

  j = mk_job(0x00000001U, DEPT_POLICE, SEV_CRITICAL); EvDbCore_Push(&db, &j);  /* newer (after wrap) */
  j = mk_job(0xFFFFFFFEU, DEPT_POLICE, SEV_CRITICAL); EvDbCore_Push(&db, &j);  /* older */
  CHECK(EvDbCore_PopBest(&db, DEPT_MASK_ALL, &o) && o.id == 0xFFFFFFFEU, "id wrap keeps FIFO");
}

/* ---------------- TFTP packets ---------------- */
static void test_tftp_proto(void)
{
  uint8_t b[64];
  static const uint8_t rrq_ok[] = { 0,1, 'e','.','b','i','n',0, 'o','c','t','e','t',0 };
  uint16_t n = TftpProto_BuildRrq(b, sizeof b, "e.bin");
  CHECK(n == sizeof rrq_ok && memcmp(b, rrq_ok, n) == 0, "RRQ bytes exact");
  CHECK(TftpProto_BuildRrq(b, 10, "event000.bin") == 0,  "RRQ too long for buffer -> 0");
  CHECK(TftpProto_BuildRrq(b, sizeof b, "") == 0,        "empty file name -> 0");

  n = TftpProto_BuildAck(b, sizeof b, 0x1234);
  CHECK(n == 4U && b[0] == 0 && b[1] == 4 && b[2] == 0x12 && b[3] == 0x34, "ACK bytes exact");

  TftpPkt_t p;
  static uint8_t data[4 + 100] = { 0,3, 0,1 };
  CHECK(TftpProto_Parse(data, sizeof data, &p) && p.op == TFTP_OP_DATA && p.block == 1U
        && p.data_len == 100U && p.data == &data[4], "parse DATA");

  static const uint8_t err[] = { 0,5, 0,1, 'F','i','l','e',0 };
  CHECK(TftpProto_Parse(err, sizeof err, &p) && p.op == TFTP_OP_ERROR
        && p.block == TFTP_ERRCODE_NOT_FOUND, "parse ERROR file not found");

  static const uint8_t bad[] = { 0,9, 0,0 };
  CHECK(!TftpProto_Parse(bad, sizeof bad, &p), "unknown opcode rejected");
  CHECK(!TftpProto_Parse(data, 3, &p),         "3-byte packet rejected");
}

/* ---------------- bonus: '+' / '-' ---------------- */
static void test_period_clamp(void)
{
  CHECK(Downloader_ClampPeriod(100) == 100U,                    "100 ms stays");
  CHECK(Downloader_ClampPeriod(0)   == DOWNLOAD_PERIOD_MIN_MS,  "never below MIN");
  CHECK(Downloader_ClampPeriod(-50) == DOWNLOAD_PERIOD_MIN_MS,  "negative -> MIN");
  CHECK(Downloader_ClampPeriod(100000) == DOWNLOAD_PERIOD_MAX_MS, "never above MAX");
}

uint32_t Tests_RunCore(TestLogFn log)
{
  tlog = log;
  n_pass = n_fail = 0;

  tlog("==== self test ====\r\n");
  test_event_layout();
  test_event_parse();
  test_event_address_80();
  test_db_severity_order();
  test_db_fifo_same_severity();
  test_db_dept_mask();
  test_db_full();
  test_db_id_wrap();
  test_tftp_proto();
  test_period_clamp();
  tlog("==== %lu passed, %lu failed ====\r\n", (unsigned long)n_pass, (unsigned long)n_fail);
  return n_fail;
}
