#include "MAX31865.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 1. A 4 db eszköz példányosítása a hozzájuk tartozó CS lábakkal
static max31865_dev_t sensors[PT100_COUNT] = {
    {.cs_pin = GPIO_NUM_7},
    {.cs_pin = GPIO_NUM_8},
    {.cs_pin = GPIO_NUM_9},
    {.cs_pin = GPIO_NUM_10}};

void app_main(void)
{
    // SPI busz és a 4 példány inicializálása
    max31865_init_all(sensors, PT100_COUNT);

    while (1)
    {
        // Ciklusban végigmegyünk a 4 különálló példányon
        for (int i = 0; i < PT100_COUNT; i++)
        {
            max31865_read_sensor(&sensors[i]);

            if (sensors[i].is_faulty)
            {
                ESP_LOGE("MAIN", "Szenzor [%d] (GPIO %d) HIBÁS!", i, sensors[i].cs_pin);
            }
            else
            {
                ESP_LOGI("MAIN", "Szenzor [%d] Temp: %.2f °C", i, sensors[i].temperature);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}