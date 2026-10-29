"""Cross-language CLI rejection and output parity regression.
Run after `make all` from the repository root.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMMANDS = [[sys.argv[1] if len(sys.argv) > 1 else str(ROOT / 'build/dacb')],
            [sys.executable, str(ROOT / 'python_twin/dacb_twin.py')]]
INVALID = [
    ['exhaustive', '3', '2'], ['exhaustive', '1', '2'],
    ['exhaustive', '2', '17'], ['exhaustive', '-2', '2'],
    ['run', '-1'], ['run', '-18446744073709551615'],
    ['run', '18446744073709551616'], ['run', 'nope'],
    ['run', ''], ['run', '+1'], ['run', ' 1'], ['run', '1_0'],
]

def invoke(command, args):
    return subprocess.run(command + args, capture_output=True, timeout=3, cwd=ROOT)

for args in INVALID:
    for command in COMMANDS:
        p = invoke(command, args)
        assert p.returncode == 2 and not p.stdout, (command, args, p.returncode, p.stdout, p.stderr)
for args in [['run', '0'], ['run', '1'], ['run', '5000'], ['exhaustive', '2', '3']]:
    c, py = [invoke(command, args) for command in COMMANDS]
    assert c.returncode == py.returncode == 0, (args, c.stderr, py.stderr)
    assert c.stdout == py.stdout, (args, c.stdout, py.stdout)
print('CLI_CONTRACT_PASS: 24 invalid invocations rejected; 4 valid output pairs identical')
