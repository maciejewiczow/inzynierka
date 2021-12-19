#ifndef INTEGRATION_POINTS_H
#define INTEGRATION_POINTS_H

class IntegrationPoints {
public:
    struct Point {
        float xi, weight;
    };

private:
    const static inline Point pts[] PROGMEM = {
        // n == 1
        { -.577350, 1.       },
        {  .577350, 1.       },

        // n == 2
        { -.774597,  .555555 },
        {  .0     ,  .888888 },
        {  .774597,  .555555 },

        // n == 3
        { -.861136,  .347855 },
        { -.339981,  .652145 },
        {  .339981,  .652145 },
        {  .861136,  .347855 },

        // n == 4
        { -.906180,  .236927 },
        { -.538469,  .478629 },
        {  .0     ,  .568889 },
        {  .538469,  .478629 },
        {  .906180,  .236927 },
    };

    static inline Point current;

public:
    static inline const Point& get(size_t n, size_t i) {
        memcpy_P(&current, &pts[static_cast<size_t>(0.5*n*(n + 1) - 1) + i], sizeof(Point));
        return current;
    }
};

#endif
