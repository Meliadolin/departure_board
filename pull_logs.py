#!/usr/bin/env python3
"""
pull_logs.py — Extract battery & boot logs from the departure board's
LittleFS partition via esptool.

Usage:
  python pull_logs.py <COM_PORT>

Requires esptool (pip install esptool). Tested with esptool>=4.9.

Partition layout:
  spiffs   data   spiffs   0x1F0000   0x200000   (2 MB for logging)

The script reads the first 256 KB of the partition — enough for ~7500
log entries (~5 days at 1 entry/min). If you have more data, increase
PARTITION_SIZE below.
"""

import re
import sys
import os
import subprocess
import tempfile

PARTITION_OFFSET = 0x1F0000
PARTITION_SIZE = 0x40000  # 256 KB (increase to 0x200000 for full 2 MB)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <COM_PORT>")
        print(f"Example: {sys.argv[0]} COM4   (Windows)")
        print(f"       {sys.argv[0]} /dev/ttyUSB0   (Linux)")
        print(f"       {sys.argv[0]} /dev/cu.usbserial-*   (macOS)")
        sys.exit(1)

    port = sys.argv[1]

    # 1. Dump LittleFS partition via esptool
    dump_file = tempfile.mktemp(suffix="_littlefs.bin")
    try:
        print(f"Reading {PARTITION_SIZE // 1024} KB from {port} at offset 0x{PARTITION_OFFSET:X} ...")
        sys.stdout.flush()
        subprocess.run(
            [
                "python", "-m", "esptool",
                "--port", port,
                "read-flash",
                hex(PARTITION_OFFSET),
                hex(PARTITION_SIZE),
                dump_file,
            ],
            check=True,
        )

        # 2. Parse binary dump for CSV lines
        with open(dump_file, "rb") as f:
            data = f.read()

        csv_lines = set()
        # Extract printable strings and find timestamped CSV rows
        for chunk in re.findall(rb"[\x20-\x7E,\-:\n\r]{12,}", data):
            text = chunk.decode("ascii", errors="replace")
            for line in text.split("\n"):
                line = line.strip()
                # Match: YYYY-MM-DD HH:MM:SS,<fields...>
                if re.match(r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},", line):
                    csv_lines.add(line)

        if not csv_lines:
            print("No log entries found. The board may not have started logging yet.")
            sys.exit(0)

        # 3. Sort and separate by file
        battery = []
        boot = []
        for line in sorted(csv_lines):
            cols = line.split(",")
            if len(cols) == 4:
                battery.append(line)
            elif len(cols) == 2:
                boot.append(line)

        # 4. Write output files
        if battery:
            with open("battery_log.csv", "w", newline="") as f:
                f.write("timestamp,voltage_v,wifi_rssi,boot_count\n")
                for line in battery:
                    f.write(line + "\n")
            print(f"Wrote {len(battery)} entries to battery_log.csv")

        if boot:
            with open("boot_log.csv", "w", newline="") as f:
                f.write("timestamp,boot_count\n")
                for line in boot:
                    f.write(line + "\n")
            print(f"Wrote {len(boot)} entries to boot_log.csv")

    except FileNotFoundError:
        print("esptool not found. Install it: pip install esptool")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"esptool failed: {e}")
        sys.exit(1)
    finally:
        if os.path.exists(dump_file):
            os.remove(dump_file)


if __name__ == "__main__":
    main()
