
from typing import Callable

from math import log, exp

def heatingCurve(tauStart: float, tStart: float, tauEnd: float, tEnd: float) -> Callable[[float], float]:
    inner = lambda x: (tStart - tEnd)/(log(tauStart) - log(tauEnd))*log(x*exp((tEnd*log(tauStart) - tStart*log(tauEnd))/(tStart - tEnd)))

    def f(x: float) -> float:
        if x < tauStart:
            return tStart

        if x > tauEnd:
            return tEnd

        return inner(x)

    return f

def temp(tau: float):
    sqrt2pi = 2.506628274631
    sigma = 0.39
    mi = -0.6

    f = lambda x:1.0/(x*sigma*sqrt2pi)*exp(-pow(log(x) - mi, 2)/(2*sigma*sigma))

    peakStart = 304.444052915472 # [s]
    peakEnd = 1100.0 # [s]

    tMax = 10000.0 # [deg C]
    tMin = -180.0 # [deg C]

    tNorm = 0
    if tau < peakStart:
        # rise
        tNorm = f(tau/646.0 + 0.0001)
    elif peakStart <= tau < peakEnd:
        # peak
        tNorm = 2.
    else:
        # fall
        tNorm = f((tau - (peakEnd - peakStart) + 25.081)/646.0 + 0.0001) + 0.041

    tNorm /= 2.

    return tMin + (tMax-tMin)*tNorm
