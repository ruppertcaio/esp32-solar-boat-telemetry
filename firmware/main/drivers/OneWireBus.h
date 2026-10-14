#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

// --------------------------------------------------------------
// Barramento 1-Wire por bit-banging (GPIO open-drain).
// Cada time slot roda em seção crítica: uma interrupção no meio
// do slot alongaria os pulsos e corromperia o bit.
// --------------------------------------------------------------
class OneWireBus {
public:
    explicit OneWireBus(gpio_num_t pin);

    bool reset();          // true se algum dispositivo respondeu (presence)
    void writeBit(int bit);
    int readBit();
    void write(uint8_t byte);
    uint8_t read();

    void skip();           // SKIP ROM (0xCC): fala com o único dispositivo do barramento

private:
    gpio_num_t pin_;
    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

#endif
