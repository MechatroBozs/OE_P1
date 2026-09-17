#ifndef MAX31865_H
#define MAX31865_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define PT100_COUNT 4

typedef struct
{
    gpio_num_t cs_pin;           // Melyik CS lábra van kötve
    spi_device_handle_t spi_dev; // Az ESP-IDF SPI eszköze
    float temperature;           // Utolsó mért hőmérséklet (°C)
    bool is_faulty;              // Hibaállapot (pl. szakadás)
} max31865_dev_t;

// Inicializálás és olvasás a példányosított tömbbel
esp_err_t max31865_init_all(max31865_dev_t *devs, size_t count);
esp_err_t max31865_read_sensor(max31865_dev_t *dev);

#endif