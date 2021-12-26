import sys
import warnings
import subprocess
from util import getAndUpdateArduinoPort

"""
    This script compiles the project and uploads it to the currently connected Arduino board.
    Requires [arduino-cli](https://github.com/arduino/arduino-cli) to be installed and added to the PATH!
"""

arduinoPresent, portInfo = getAndUpdateArduinoPort('./.vscode/arduino.json')

if not arduinoPresent:
    warnings.warn("No arduino board detected! Project will be only compiled, without upload")

args = [
    "arduino-cli", "compile",
    "--libraries", "./lib",
    "-b", portInfo.FQBN,
]

if arduinoPresent:
    args.extend([
        "-p", portInfo.port,
        "-u",
    ])

args.append(sys.argv[1])

proc = subprocess.run(
    args,
    stdout=sys.stdout,
    stderr=sys.stderr,
)

exit(proc.returncode)
