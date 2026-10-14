#ifndef DS18B20_H
#define DS18B20_H

#include "OneWireBus.h"

#include "esp_err.h"

// --------------------------------------------------------------
// Sensor de temperatura DS18B20 (único no barramento, SKIP ROM)
// --------------------------------------------------------------
class Ds18b20 {
public:
    static constexpr float kDisconnectedC = -127.0f;   // valor de "sem leitura"

    explicit Ds18b20(OneWireBus *bus);

    esp_err_t begin();        // ESP_OK se houver pulso de presença
    void startConversion();   // CONVERT T (0x44); conversão leva até 750 ms
    float readTempC();        // kDisconnectedC sem presença ou com CRC inválido

private:
    OneWireBus *bus_;
};

#endif
