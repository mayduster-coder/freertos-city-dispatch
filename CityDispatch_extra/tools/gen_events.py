#!/usr/bin/env python3
"""
gen_events.py - create City Dispatch event files for the TFTP server folder.

Each file = exactly 100 bytes:
    [0]      department  0 Police, 1 Fire, 2 Ambulance, 3 Maintenance
    [1]      severity    0 Critical, 1 Warning, 2 Notice
    [2..81]  address     ASCII, '\\0' padded (80 bytes)
    [82..99] padding     0xFF x 18

Usage (Windows: py gen_events.py ...):
    python gen_events.py                      -> 40 files into ./events
    python gen_events.py -n 100 -o C:\\tftp   -> 100 files straight into the server folder
    python gen_events.py --bad                -> also 3 broken files to test rejection
    python gen_events.py --show C:\\tftp\\event000.bin   -> decode one file
"""
import argparse
import os
import random
import sys

DEPTS = ["Police", "Fire", "Ambulance", "Maintenance"]
SEVS = ["CRITICAL", "WARNING", "NOTICE"]
STREETS = ["Herzl", "Rothschild", "Dizengoff", "Ibn Gabirol", "Allenby", "King George",
           "Ben Yehuda", "Weizmann", "Arlozorov", "Jabotinsky", "Sokolov", "Bialik",
           "HaYarkon", "Namir", "Begin", "Kaplan", "Shenkar", "Golomb", "Eilat", "Yefet"]
CITIES = ["Tel Aviv", "Holon", "Ramat Gan", "Bat Yam", "Givatayim", "Jaffa"]
WHAT = {
    0: ["Burglary in progress", "Car accident", "Noise complaint", "Suspicious object"],
    1: ["Apartment fire", "Car on fire", "Gas leak", "Smoke from building"],
    2: ["Person collapsed", "Injured cyclist", "Breathing problems", "Fall from ladder"],
    3: ["Water pipe burst", "Traffic light down", "Pothole", "Street light out"],
}
# real life: most events are notices, few are critical
SEV_WEIGHTS = [2, 3, 5]


def make_event(dept: int, sev: int, address: str) -> bytes:
    a = address.encode("ascii", "replace")[:80]
    data = bytes([dept, sev]) + a.ljust(80, b"\0") + b"\xff" * 18
    assert len(data) == 100
    return data


def decode(data: bytes) -> str:
    if len(data) != 100:
        return f"BAD SIZE {len(data)}"
    d, s = data[0], data[1]
    addr = data[2:82].split(b"\0", 1)[0].decode("ascii", "replace")
    pad_ok = data[82:] == b"\xff" * 18
    dn = DEPTS[d] if d < 4 else f"?{d}"
    sn = SEVS[s] if s < 3 else f"?{s}"
    return f"{dn:<11} {sn:<8} pad={'ok' if pad_ok else 'BAD'}  \"{addr}\""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", type=int, default=40, help="number of events (default 40)")
    ap.add_argument("-o", default="events", help="output folder (default ./events)")
    ap.add_argument("--seed", type=int, default=2026, help="random seed (same seed = same files)")
    ap.add_argument("--fmt", default="event{:03d}.bin", help="file name format")
    ap.add_argument("--bad", action="store_true", help="add broken files to test rejection")
    ap.add_argument("--show", help="decode one event file and exit")
    args = ap.parse_args()

    if args.show:
        with open(args.show, "rb") as f:
            print(decode(f.read()))
        return 0

    rnd = random.Random(args.seed)
    os.makedirs(args.o, exist_ok=True)

    count = [[0] * 3 for _ in range(4)]
    for i in range(args.n):
        dept = rnd.randrange(4)
        sev = rnd.choices(range(3), weights=SEV_WEIGHTS)[0]
        addr = f"{rnd.choice(WHAT[dept])} - {rnd.choice(STREETS)} {rnd.randint(1, 120)}, {rnd.choice(CITIES)}"
        data = make_event(dept, sev, addr)
        with open(os.path.join(args.o, args.fmt.format(i)), "wb") as f:
            f.write(data)
        count[dept][sev] += 1
        print(f"{args.fmt.format(i)}  {decode(data)}")

    n = args.n
    if args.bad:
        bad = {
            "bad department (7)": bytes([7, 0]) + b"Bad dept".ljust(80, b"\0") + b"\xff" * 18,
            "bad padding": bytes([0, 0]) + b"Bad padding".ljust(80, b"\0") + b"\x00" * 18,
            "99 bytes": bytes([1, 1]) + b"Too short".ljust(80, b"\0") + b"\xff" * 17,
        }
        for why, data in bad.items():
            name = args.fmt.format(n)
            with open(os.path.join(args.o, name), "wb") as f:
                f.write(data)
            print(f"{name}  BROKEN on purpose: {why}")
            n += 1

    print(f"\n{n} files in {os.path.abspath(args.o)}")
    print(f"{'':<12}{'CRIT':>6}{'WARN':>6}{'NOTE':>6}")
    for d in range(4):
        print(f"{DEPTS[d]:<12}{count[d][0]:>6}{count[d][1]:>6}{count[d][2]:>6}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
