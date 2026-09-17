#include "MAX31865.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "MAX31865";

// ESP32-C3 SPI lábkiosztás
#define PIN_NUM_MISO GPIO_NUM_5
#define PIN_NUM_MOSI GPIO_NUM_6
#define PIN_NUM_CLK GPIO_NUM_4

// A 4 db MAX31865 modul egyedi Chip Select (CS) lábai
static const gpio_num_t s_cs_pins[PT100_COUNT] = {
    GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10};

static spi_device_handle_t s_spi_handles[PT100_COUNT];

// MAX31865 Regiszterek
#define MAX31865_CONFIG_REG 0x00
#define MAX31865_RTD_MSB_REG 0x01
#define MAX31865_CONFIG_WRITE 0x80

#define RREF 430.0f
#define RNOMINAL 100.0f

// Segédfüggvény: Regiszter írása SPI-n
static esp_err_t max31865_write_reg(spi_device_handle_t spi, uint8_t reg, uint8_t val)
{
    uint8_t tx_data[2] = {reg | MAX31865_CONFIG_WRITE, val};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    return spi_device_polling_transmit(spi, &t);
}

// Segédfüggvény: Regiszterek olvasása SPI-n
static esp_err_t max31865_read_regs(spi_device_handle_t spi, uint8_t reg, uint8_t *dest, size_t len)
{
    uint8_t tx_buf[3] = {reg & 0x7F, 0x00, 0x00};
    uint8_t rx_buf[3] = {0};

    spi_transaction_t t = {
        .length = 8 * (len + 1),
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    if (ret == ESP_OK)
    {
        for (size_t i = 0; i < len; i++)
        {
            dest[i] = rx_buf[i + 1];
        }
    }
    return ret;
}

// Nyers RTD értékből °C számítás
static float raw_to_celsius(uint16_t raw_rtd)
{
    float r_rtd = ((float)raw_rtd * RREF) / 32768.0f;
    return (r_rtd - RNOMINAL) / (RNOMINAL * 0.003851f);
}

esp_err_t max31865_init_all(void)
{
    // 1. SPI Busz konfigurációja
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI busz inicializalasi hiba!");
        return ret;
    }

    // 2. A 4 db IC csatlakoztatása az SPI buszhoz
    for (int i = 0; i < PT100_COUNT; i++)
    {
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
            .mode = 1,                         // SPI Mode 1
            .spics_io_num = s_cs_pins[i],
            .queue_size = 1,
        };

        ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handles[i]);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Hiba az SPI eszkoz [%d] hozzaadasanal!", i);
            return ret;
        }

        // 3. MAX31865 konfigurálása: VBIAS BE, Auto konverzió, 3-wire/4-wire, 50Hz szűrő (0xC2)
        ret = max31865_write_reg(s_spi_handles[i], MAX31865_CONFIG_REG, 0xC2);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "MAX31865 [%d] konfigurasios hiba!", i);
            return ret;
        }
    }

    ESP_LOGI(TAG, "Mind a 4 x MAX31865 modul sikeresen inicializalva.");
    return ESP_OK;
}

esp_err_t max31865_read_all(pt100_telemetry_t *data)
{
    if (!data)
        return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < PT100_COUNT; i++)
    {
        uint8_t buffer[2] = {0};

        esp_err_t ret = max31865_read_regs(s_spi_handles[i], MAX31865_RTD_MSB_REG, buffer, 2);
        if (ret != ESP_OK)
        {
            data->is_faulty[i] = true;
            continue;
        }

        uint16_t raw_rtd = ((uint16_t)buffer[0] << 8) | buffer[1];

        // Hiba-bit ellenőrzése (LSB legalsó bitje)
        if (raw_rtd & 0x01)
        {
            data->is_faulty[i] = true;
            data->temp_celsius[i] = 0.0f;
        }
        else
        {
            data->is_faulty[i] = false;
            raw_rtd >>= 1; // Hiba-bit eltolása, a maradt 15 bit az RTD adatszó
            data->temp_celsius[i] = raw_to_celsius(raw_rtd);
        }
    }

    return ESP_OK;
}