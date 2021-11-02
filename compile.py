import sys
import warnings
import subprocess
from util import getAndUpdateArduinoPort

arduinoPresent, portInfo = getAndUpdateArduinoPort('./.vscode/arduino.json')

if not arduinoPresent:
    warnings.warn("No arduino board detected! Project will be only compiled, without upload")

args = [
    "arduino-cli", "compile",
    "--libraries", "D:\\Projekty\\arduinko\\libraries",
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
