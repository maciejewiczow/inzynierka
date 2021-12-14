#ifndef INTEGRATION_POINTS_H
#define INTEGRATION_POINTS_H

class IntegrationPoints {
public:
    struct Point {
        float x, weight;
    };

private:
    const static inline Point pts[] = {
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


public:
    static inline const Point& get(size_t n, size_t i) {
        return pts[static_cast<size_t>(0.5*n*(n + 1) - 1) + i];
    }
};

#endif
