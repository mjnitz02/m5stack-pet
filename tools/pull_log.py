# /// script
# requires-python = ">=3.10"
# dependencies = ["pyserial"]
# ///
"""
Copy the on-device log off a stick running a debug build (`make flash-debug`).

Writes logs/pet-<date>-<time>.csv; the columns are described in
src/petlog.h. The stick has to be awake and on USB. `--clear` empties the
log on the stick instead.
"""

import argparse
import glob
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

ROOT = Path(__file__).resolve().parent.parent
TIMEOUT_S = 20


def open_port(name: str | None) -> serial.Serial:
    if name is None:
        found = sorted(glob.glob("/dev/cu.usbmodem*"))
        if not found:
            sys.exit("no /dev/cu.usbmodem* port: is the stick plugged in and awake?")
        name = found[0]
    port = serial.Serial()
    port.port = name
    port.baudrate = 115200
    port.timeout = 1
    # Leave the control lines alone so opening the port doesn't reset the stick.
    port.dtr = False
    port.rts = False
    port.open()
    return port


def ask(port: serial.Serial, command: bytes, reply: bytes) -> bytes:
    """Send `command` once a second until a line starting with `reply` comes back."""
    deadline = time.monotonic() + TIMEOUT_S
    while time.monotonic() < deadline:
        port.write(command)
        end = time.monotonic() + 1
        while time.monotonic() < end:
            line = port.readline()
            if line.startswith(reply):
                return line
    sys.exit("no answer from the stick: is it awake, and was it flashed with `make flash-debug`?")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", help="serial port (default: first /dev/cu.usbmodem*)")
    ap.add_argument("--clear", action="store_true", help="empty the log on the stick")
    args = ap.parse_args()

    with open_port(args.port) as port:
        if args.clear:
            ask(port, b"C", b"#LOG CLEARED")
            print("log cleared")
            return

        size = int(ask(port, b"L", b"#LOG BEGIN").split()[2])
        rows = []
        deadline = time.monotonic() + TIMEOUT_S + size / 2000
        while time.monotonic() < deadline:
            line = port.readline()
            if line.startswith(b"#LOG END"):
                break
            # The firmware's own serial chatter and repeat markers are not log rows.
            if line and not line.startswith((b"#LOG", b"bat=")):
                rows.append(line)
        else:
            sys.exit("the dump stopped before its end marker")

    data = b"".join(rows)
    if len(data) != size:
        print(f"warning: the stick said {size} bytes, got {len(data)}", file=sys.stderr)
    out = ROOT / "logs" / f"pet-{datetime.now():%Y%m%d-%H%M%S}.csv"
    out.parent.mkdir(exist_ok=True)
    out.write_bytes(data)
    print(f"{out.relative_to(ROOT)}: {len(rows) - 1} rows")


if __name__ == "__main__":
    main()
