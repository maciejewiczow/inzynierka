#ifndef CONFIG_HEADER_GUARD
#define CONFIG_HEADER_GUARD

struct Config {
    unsigned integrationScheme = 1;
    float alphaAir = 300;   // [W/(m^2*K)]
    float C = 700.0;        // [J/(kg*K)]
    float Ro = 7800.0;      // [kg/m^3]
    float K = 25.0;         // [W/m*K]
};

#endif
