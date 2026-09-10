#!/usr/bin/env python3
"""Validate one complete benchmark capture and summarize actual observations."""
import argparse
import csv
import io
import json
import math
from pathlib import Path
import statistics

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture', type=Path)
args = parser.parse_args()
lines = args.capture.read_text().splitlines()
metadata = {}
data = []
passed = False
for line in lines:
    if line.startswith('# '):
        key, separator, value = line[2:].partition('=')
        if separator:
            if key in metadata: raise SystemExit('Duplicate metadata: ' + key)
            metadata[key] = value
    elif line == 'PASS':
        if passed: raise SystemExit('Multiple runs in capture')
        passed = True
    elif line.strip():
        if passed or line.startswith('FAIL'): raise SystemExit('Incomplete or failed capture')
        data.append(line)
if not passed: raise SystemExit('Missing terminal PASS')
required = ['platform', 'revision', 'compiler', 'flags', 'timer_hz', 'repetitions',
            'n', 'nnz_a', 'nnz_l_strict', 'nnz_u', 'char_bit', 'real_c_bytes', 'index_c_bytes',
            'factor_c_bytes', 'symbolic_c_bytes', 'analysis_work_c_bytes', 'numeric_work_c_bytes',
            'benchmark_context_c_bytes', 'timer_overhead_ticks_min', 'warmup', 'overhead_subtracted']
if any(key not in metadata for key in required): raise SystemExit('Missing required metadata')
rows = list(csv.DictReader(io.StringIO('\n'.join(data))))
if len(rows) != int(metadata['repetitions']) or len(rows) != 64:
    raise SystemExit('Capture must contain all 64 measured solves')
if [row.get('sample') for row in rows] != [str(i) for i in range(len(rows))]:
    raise SystemExit('Missing or duplicated sample')
hz = int(metadata['timer_hz'])
if hz <= 0: raise SystemExit('Invalid timer frequency')
summary = {'metadata': metadata, 'observed_microseconds': {}}
for field in ['factor_ticks', 'solve_ticks']:
    ticks = [int(row[field]) for row in rows]
    if any(t <= 0 or t > 0xffffffff for t in ticks): raise SystemExit('Invalid timer delta')
    times = [t * 1e6 / hz for t in ticks]
    summary['observed_microseconds'][field.removesuffix('_ticks')] = {
        'min': min(times), 'median': statistics.median(times), 'max': max(times)}
for field in ['backward_error_e12', 'solution_error_e12']:
    values = [int(row[field]) * 1e-12 for row in rows]
    if any(not math.isfinite(v) or v < 0 or v > 64 * 2**-23 + 1e-12 for v in values):
        raise SystemExit('Numerical acceptance failed')
    summary['max_' + field.removesuffix('_e12')] = max(values)
char_bit = int(metadata['char_bit'])
if char_bit not in (8, 16): raise SystemExit('Unexpected C byte width: inspect capture')
summary['memory_octets'] = {key.removesuffix('_c_bytes'): int(value) * char_bit / 8
                            for key, value in metadata.items() if key.endswith('_c_bytes')}
summary['notes'] = ['Maximum is an observation, not a WCET bound.',
                    'Allocated stack is not measured stack usage.',
                    'Host timing is not MCU performance evidence.']
print(json.dumps(summary, indent=2))
