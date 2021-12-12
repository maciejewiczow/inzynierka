#include "print_util.h"

namespace prnt {

Print& operator<<(Print& out, const __FlashStringHelper* thing) {
    out.print(thing);
    return out;
}

Print& operator<<(Print& out, const String& thing) {
    out.print(thing);
    return out;
}

Print& operator<<(Print& out, const char thing[]) {
    out.print(thing);
    return out;
}

Print& operator<<(Print& out, char thing) {
    out.print(thing);
    return out;
}

Print& operator<<(Print& out, unsigned char thing) {
    out.print(thing, DEC);
    return out;
}

Print& operator<<(Print& out, int thing) {
    out.print(thing, DEC);
    return out;
}

Print& operator<<(Print& out, unsigned int thing) {
    out.print(thing, DEC);
    return out;
}

Print& operator<<(Print& out, long thing) {
    out.print(thing, DEC);
    return out;
}

Print& operator<<(Print& out, unsigned long thing) {
    out.print(thing, DEC);
    return out;
}

Print& operator<<(Print& out, double thing) {
    out.print(thing, 2);
    return out;
}

Print& operator<<(Print& out, const Printable& thing) {
    out.print(thing);
    return out;
}

Print& operator<<(Print& out, Print& (*modifier)(Print&)) {
    modifier(out);
    return out;
}

Print& endl(Print& out) {
    out.println();
    return out;
}

}
