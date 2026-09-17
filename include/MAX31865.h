#ifndef MAX31865_H
#define MAX31865_H

#include "esp_err.h"
#include <stdbool.h>

#define PT100_COUNT 4

typedef struct
{
    float temp_celsius[PT100_COUNT];
    bool is_faulty[PT100_COUNT];
} pt100_telemetry_t;

// SPI busz, CS lábak és a 4 db IC inicializálása
esp_err_t max31865_init_all(void);

// Adatok beolvasása a 4 csatornáról
esp_err_t max31865_read_all(pt100_telemetry_t *data);

#endif // MAX31865_H