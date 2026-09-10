#!/usr/bin/env python3
"""Prove sparse tests detect representative defects in isolated temporary trees."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
mutations = {
    'indexing': ('work[brsp_lu_map(s->inverse_row_order, s->pattern.row_indices[p])] = values[p];', 'work[0] = values[p];'),
    'ordered-output': ('solution[j] = value;', 'solution[j] = solution[i];'),
    'row-permutation': ('return map == NULL ? index : map[index];', 'return index;'),
    'workspace-sizing': ('req.numeric_work_reals = pattern->column_count;',
                         'req.numeric_work_reals = pattern->column_count ? pattern->column_count - 1U : 0U;'),
    'substitution': ('solution[original_row] /= numeric->factors[s->diagonal_offsets[j]];',
                     'solution[original_row] *= numeric->factors[s->diagonal_offsets[j]];'),
}

def run(args, cwd):
    return subprocess.run(args, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

with tempfile.TemporaryDirectory(prefix='brsp-mutations-') as directory:
    tree = Path(directory) / 'source'
    shutil.copytree(root, tree, ignore=shutil.ignore_patterns('.git', 'build', 'out', '__pycache__'))
    build = Path(directory) / 'build'
    result = run(['cmake', '-S', str(tree), '-B', str(build), '-G', 'Ninja',
                  '-DBRSP_BUILD_EXAMPLES=OFF', '-DBRSP_BUILD_BENCHMARKS=OFF',
                  '-DCMAKE_C_COMPILER=' + os.environ.get('CC', 'cc')], tree)
    if result.returncode: raise SystemExit(result.stdout)
    source = tree / 'src/brsp_sparse_lu.c'
    original = source.read_text()
    for name, replacement in [('baseline', None), *mutations.items()]:
        if replacement:
            old, new = replacement
            if original.count(old) != 1: raise SystemExit('Mutation anchor is not unique: ' + name)
            source.write_text(original.replace(old, new))
        else:
            source.write_text(original)
        result = run(['cmake', '--build', str(build), '--target', 'brsp_sparse_tests'], tree)
        if result.returncode: raise SystemExit('Build failed (not a detected mutation):\n' + result.stdout)
        result = run([str(build / 'tests/brsp_sparse_tests')], tree)
        if (result.returncode == 0) != (name == 'baseline'):
            raise SystemExit('Unexpected test outcome for ' + name + ':\n' + result.stdout)
        print(name + ': ' + ('passed' if name == 'baseline' else 'detected'))
