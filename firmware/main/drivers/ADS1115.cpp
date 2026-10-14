#include "ADS1115.h"

#include <cmath>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ADS1115";

// =====================================================
// Registradores e campos do CONFIG (datasheet ADS1115)
// =====================================================
constexpr uint8_t  kRegConversion  = 0x00;
constexpr uint8_t  kRegConfig      = 0x01;

constexpr uint16_t kCfgOsStart     = 0x8000;  // OS: inicia conversão single-shot
constexpr uint16_t kCfgMuxAin0Gnd  = 0x4000;  // MUX: AIN0 vs GND (AINx = 0x4000 + x << 12)
constexpr uint16_t kCfgPga4v096    = 0x0200;  // PGA: ±4.096 V
constexpr uint16_t kCfgModeSingle  = 0x0100;  // MODE: single-shot
constexpr uint16_t kCfgDr128Sps    = 0x0080;  // DR: 128 SPS (~7.8 ms por conversão)
constexpr uint16_t kCfgCompDisable = 0x0003;  // comparador desabilitado

constexpr float    kLsbVolts       = 0.000125f;  // 4.096 V / 32768 = 125 µV
constexpr uint32_t kI2cSpeedHz     = 100000;     // 100 kHz
constexpr int      kI2cTimeoutMs   = 50;
constexpr uint32_t kConversionMs   = 10;         // tempo mínimo para conversão

ADS1115::ADS1115(uint8_t addr)
    : i2c_addr(addr)
{
}

esp_err_t ADS1115::begin(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = i2c_addr;
    dev_cfg.scl_speed_hz = kI2cSpeedHz;

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar ADS1115 ao barramento: %s", esp_err_to_name(ret));
        return ret;
    }

    uint16_t dummy;
    ret = readRegister(kRegConversion, dummy);

    if (ret == ESP_OK)
        ESP_LOGI(TAG, "ADS1115 detectado no endereço 0x%02X", i2c_addr);
    else
        ESP_LOGE(TAG, "Falha ao detectar ADS1115 no endereço 0x%02X", i2c_addr);

    return ret;
}

float ADS1115::readChannel(Channel ch)
{
    if (i2c_dev == nullptr)
        return NAN;

    uint16_t cfg =
        kCfgOsStart |
        static_cast<uint16_t>(kCfgMuxAin0Gnd + (ch << 12)) |
        kCfgPga4v096 |
        kCfgModeSingle |
        kCfgDr128Sps |
        kCfgCompDisable;

    esp_err_t ret = writeRegister(kRegConfig, cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Falha ao configurar canal %d: %s", (int)ch, esp_err_to_name(ret));
        return NAN;
    }

    // +1 tick: com CONFIG_FREERTOS_HZ=100, pdMS_TO_TICKS(10) = 1 tick, que
    // pode durar quase 0 ms; o tick extra garante a espera mínima.
    vTaskDelay(pdMS_TO_TICKS(kConversionMs) + 1);

    uint16_t raw = 0;
    if (readRegister(kRegConversion, raw) != ESP_OK)
        return NAN;

    return static_cast<int16_t>(raw) * kLsbVolts;
}

esp_err_t ADS1115::writeRegister(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {
        reg,
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value & 0xFF)
    };

    return i2c_master_transmit(i2c_dev, buf, sizeof(buf), kI2cTimeoutMs);
}

esp_err_t ADS1115::readRegister(uint8_t reg, uint16_t &value)
{
    uint8_t data[2];
    esp_err_t ret = i2c_master_transmit_receive(i2c_dev, &reg, 1, data, sizeof(data), kI2cTimeoutMs);

    if (ret == ESP_OK)
        value = (data[0] << 8) | data[1];

    return ret;
}
