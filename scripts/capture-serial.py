#!/usr/bin/env python3
"""Capture one complete UART benchmark after a physical reset (POSIX hosts)."""
import argparse
import os
from pathlib import Path
import select
import termios
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('port', help='XDS110 application UART, e.g. /dev/ttyACM0')
parser.add_argument('output', type=Path)
parser.add_argument('--timeout', type=float, default=60)
args = parser.parse_args()
fd = os.open(args.port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
old = termios.tcgetattr(fd)
try:
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIFLUSH)
    print('UART ready at 115200 8N1. Press S1 RESET on the board.', flush=True)
    deadline = time.monotonic() + args.timeout
    capture = bytearray()
    while time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], min(1, max(0, deadline - time.monotonic())))
        if ready:
            capture.extend(os.read(fd, 4096))
            if b'PASS\r\n' in capture or b'FAIL:' in capture:
                break
    args.output.write_bytes(capture)
    if b'# platform=LAUNCHXL-F28P55X' not in capture or b'PASS\r\n' not in capture:
        raise SystemExit('No complete passing run; partial capture saved for diagnosis')
    print('Saved ' + str(args.output))
finally:
    termios.tcsetattr(fd, termios.TCSANOW, old)
    os.close(fd)
