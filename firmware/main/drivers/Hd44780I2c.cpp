#include "Hd44780I2c.h"

#include <cstdarg>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

// =====================================================
// Bits do PCF8574 -> pinos do LCD
// =====================================================
constexpr uint8_t  kBacklight   = 0x08;
constexpr uint8_t  kEnable      = 0x04;
constexpr uint8_t  kRsCmd       = 0x00;
constexpr uint8_t  kRsData      = 0x01;

constexpr uint32_t kI2cSpeedHz  = 100000;  // 100 kHz
constexpr int      kI2cTimeoutMs = 50;

Hd44780I2c::Hd44780I2c(uint8_t addr)
    : i2c_addr(addr)
{
}

esp_err_t Hd44780I2c::begin(i2c_master_bus_handle_t bus, uint8_t cols, uint8_t rows)
{
    _cols = cols;
    _rows = rows;
    updateRowOffsets();

    // Só endereço (sem dados): não interfere na sequência de inicialização
    esp_err_t ret = i2c_master_probe(bus, i2c_addr, kI2cTimeoutMs);
    if (ret != ESP_OK)
        return ret;

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = i2c_addr;
    dev_cfg.scl_speed_hz = kI2cSpeedHz;

    ret = i2c_master_bus_add_device(bus, &dev_cfg, &i2c_dev);
    if (ret != ESP_OK)
        return ret;

    initLCD();
    return ESP_OK;
}

esp_err_t Hd44780I2c::writeByte(uint8_t data)
{
    // LCD ausente: ignora em silêncio (evita log a cada caractere)
    if (i2c_dev == nullptr)
        return ESP_ERR_INVALID_STATE;

    return i2c_master_transmit(i2c_dev, &data, 1, kI2cTimeoutMs);
}

void Hd44780I2c::sendNibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | kBacklight | mode;

    writeByte(data | kEnable);
    esp_rom_delay_us(1);

    writeByte(data & ~kEnable);
    esp_rom_delay_us(50);
}

void Hd44780I2c::sendByte(uint8_t value, uint8_t mode)
{
    sendNibble(value & 0xF0, mode);
    sendNibble((value << 4) & 0xF0, mode);
}

void Hd44780I2c::initLCD()
{
    vTaskDelay(pdMS_TO_TICKS(50));

    sendNibble(0x30, kRsCmd);
    esp_rom_delay_us(5000);   // > 4.1 ms (vTaskDelay de 5 ms vira 0 tick com HZ=100)

    sendNibble(0x30, kRsCmd);
    esp_rom_delay_us(150);

    sendNibble(0x30, kRsCmd);
    sendNibble(0x20, kRsCmd); // 4 bits

    sendByte(0x28, kRsCmd);
    sendByte(0x0C, kRsCmd);
    sendByte(0x06, kRsCmd);
    clear();
}

void Hd44780I2c::clear()
{
    sendByte(0x01, kRsCmd);
    esp_rom_delay_us(2000);   // > 1.52 ms (vTaskDelay de 2 ms vira 0 tick com HZ=100)
}

void Hd44780I2c::home()
{
    sendByte(0x02, kRsCmd);
    esp_rom_delay_us(2000);
}

void Hd44780I2c::setCursor(uint8_t col, uint8_t row)
{
    if (row >= _rows)
        row = _rows - 1;

    sendByte(0x80 | (rowOffsets[row] + col), kRsCmd);
}

void Hd44780I2c::printLine(uint8_t row, const char *fmt, ...)
{
    char buffer[64];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) len = 0;

    setCursor(0, row);
    for (int col = 0; col < _cols; col++)
    {
        char c = (col < len && buffer[col] != '\0') ? buffer[col] : ' ';
        sendByte(c, kRsData);
    }
}

void Hd44780I2c::updateRowOffsets()
{
    rowOffsets.clear();

    if (_rows == 1) rowOffsets = {0x00};
    else if (_rows == 2) rowOffsets = {0x00, 0x40};
    else if (_rows == 4)
    {
        if (_cols == 16)
            rowOffsets = {0x00, 0x40, 0x10, 0x50};
        else
            rowOffsets = {0x00, 0x40, 0x14, 0x54};
    }
    else rowOffsets = {0x00, 0x40};
}
