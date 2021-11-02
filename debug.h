#ifndef UTIL_HEADER_GUARD
#define UTIL_HEADER_GUARD

// #define DEBUG_PRINTS

#define DBG_HEADER() __func__ << ": "

#if defined(DEBUG_PRINTS)
#   define DBG_Serial(x) Serial << x
#else
#   define DBG_Serial(x)
#endif

#endif
