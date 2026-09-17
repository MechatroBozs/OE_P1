#include "MAX31865.h"
#include "esp_log.h"

static const char *TAG = "MAX31865";

// Egyetlen konkrét eszköz kiolvasása
esp_err_t max31865_read_sensor(max31865_dev_t *dev)
{
    if (!dev || !dev->spi_dev)
        return ESP_ERR_INVALID_ARG;

    uint8_t tx_buf[3] = {0x01, 0x00, 0x00}; // RTD MSB regiszter olvasás
    uint8_t rx_buf[3] = {0};

    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_polling_transmit(dev->spi_dev, &t);
    if (ret != ESP_OK)
    {
        dev->is_faulty = true;
        return ret;
    }

    uint16_t raw_rtd = ((uint16_t)rx_buf[1] << 8) | rx_buf[2];

    if (raw_rtd & 0x01)
    { // Hiba bit ellenőrzés
        dev->is_faulty = true;
        dev->temperature = 0.0f;
    }
    else
    {
        dev->is_faulty = false;
        raw_rtd >>= 1;
        float r_rtd = ((float)raw_rtd * 430.0f) / 32768.0f;
        dev->temperature = (r_rtd - 100.0f) / 0.3851f;
    }

    return ESP_OK;
}

// Az összes megadott példány felvétele az SPI buszra
esp_err_t max31865_init_all(max31865_dev_t *devs, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = 1 * 1000 * 1000,
            .mode = 1,
            .spics_io_num = devs[i].cs_pin, // A saját CS lábát kapja meg
            .queue_size = 1,
        };

        esp_err_t ret = spi_bus_add_device(SPI2_HOST, &devcfg, &devs[i].spi_dev);
        if (ret != ESP_OK)
            return ret;
    }
    return ESP_OK;
}