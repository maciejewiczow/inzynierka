import sys
import warnings
import subprocess
from util import getAndUpdateArduinoPort

portInfo = getAndUpdateArduinoPort('./.vscode/arduino.json')

if not portInfo:
    warnings.warn("No arduino board detected! Make sure the arduino is connected")
    exit(1)

proc = subprocess.run(
    [
        "arduino-cli",
        "compile",
        "-b",
        portInfo.FQBN,
        "-u",
        "-p",
        portInfo.port,
        "--libraries",
        "D:\\Projekty\\arduinko\\libraries",
        sys.argv[1]
    ],
    stdout=sys.stdout,
    stderr=sys.stderr,
)

exit(proc.returncode)
