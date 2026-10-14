#ifndef HD44780_I2C_H
#define HD44780_I2C_H

#include <vector>
#include <cstdint>

#include "driver/i2c_master.h"
#include "esp_err.h"

// --------------------------------------------------------------
// LCD HD44780 (modo 4 bits) via expansor I2C PCF8574
// --------------------------------------------------------------
class Hd44780I2c
{
public:
    explicit Hd44780I2c(uint8_t addr);

    // Verifica o endereço, adiciona o LCD ao barramento e executa a inicialização
    esp_err_t begin(i2c_master_bus_handle_t bus, uint8_t cols, uint8_t rows);

    void clear();
    void home();
    void setCursor(uint8_t col, uint8_t row);

    // Escreve a linha inteira (formato printf), completando com espaços
    // até a largura do display; dispensa clear() e evita flicker.
    void printLine(uint8_t row, const char *fmt, ...);

private:
    i2c_master_dev_handle_t i2c_dev = nullptr;
    uint8_t i2c_addr;

    uint8_t _cols = 0;
    uint8_t _rows = 0;
    std::vector<uint8_t> rowOffsets;

    esp_err_t writeByte(uint8_t data);
    void sendNibble(uint8_t nibble, uint8_t mode);
    void sendByte(uint8_t value, uint8_t mode);
    void initLCD();
    void updateRowOffsets();
};

#endif
