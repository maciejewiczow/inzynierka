#ifndef PRINT_UTILS_GUARD
#define PRINT_UTILS_GUARD

#include <Print.h>

namespace prnt {
Print& operator<<(Print& out, const __FlashStringHelper* thing);
Print& operator<<(Print& out, const String& thing);
Print& operator<<(Print& out, const char thing[]);
Print& operator<<(Print& out, char thing);
Print& operator<<(Print& out, unsigned char thing);
Print& operator<<(Print& out, int thing);
Print& operator<<(Print& out, unsigned int thing);
Print& operator<<(Print& out, long thing);
Print& operator<<(Print& out, unsigned long thing);
Print& operator<<(Print& out, double thing);
Print& operator<<(Print& out, const Printable& thing);
Print& operator<<(Print& out, Print& (*modifier)(Print&) );
Print& endl(Print& out);
}

#endif
