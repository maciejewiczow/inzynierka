#ifndef UTIL_HEADER_GUARD
#define UTIL_HEADER_GUARD

#define nameof(x) #x

#define DBG_HEADER() __func__ << ": "

#if DEBUG_PRINTS
#   define DBG_Serial(x) Serial << x
#else
#   define DBG_Serial(x)
#endif

#endif
