from typing import Callable
from math import log, exp

def heatingCurve(tauStart: float, tStart: float, tauEnd: float, tEnd: float) -> Callable[[float], float]:
    """Returns a function that is a logarightm function passing through points `(tauStart, tStart)` and `(tauEnd, tEnd)`"""

    inner = lambda x: (
        (tStart - tEnd)/(log(tauStart) - log(tauEnd))
            * log(x*exp((tEnd*log(tauStart) - tStart*log(tauEnd))/(tStart - tEnd)))
    )

    def f(x: float) -> float:
        if x < tauStart:
            return tStart

        if x > tauEnd:
            return tEnd

        return inner(x)

    return f
