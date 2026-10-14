#ifndef AD1115_H
#define AD1115_H

#include <cstdint>

#include "driver/i2c_master.h"
#include "esp_err.h"

class ADS1115 {
public:
    enum Channel { CH0 = 0, CH1, CH2, CH3 };

    explicit ADS1115(uint8_t addr);

    // Adiciona o ADS1115 ao barramento e verifica se responde
    esp_err_t begin(i2c_master_bus_handle_t bus);

    // Tensão no canal (single-ended, ±4.096 V) em volts.
    // Retorna NAN em falha de I2C; quem chama decide o que fazer
    // (os sensores mantêm o último valor filtrado).
    float readChannel(Channel ch);

private:
    i2c_master_dev_handle_t i2c_dev = nullptr;
    uint8_t i2c_addr;

    esp_err_t writeRegister(uint8_t reg, uint16_t value);
    esp_err_t readRegister(uint8_t reg, uint16_t &value);
};

#endif
