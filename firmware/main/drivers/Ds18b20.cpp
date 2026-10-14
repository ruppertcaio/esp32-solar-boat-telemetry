#include "Ds18b20.h"

#include "Crc8.h"

constexpr uint8_t kCmdConvertT        = 0x44;
constexpr uint8_t kCmdReadScratchpad  = 0xBE;
constexpr int     kScratchpadSize     = 9;    // 8 bytes de dados + CRC

Ds18b20::Ds18b20(OneWireBus *bus)
    : bus_(bus) {}

esp_err_t Ds18b20::begin() {
    return bus_->reset() ? ESP_OK : ESP_ERR_NOT_FOUND;
}

void Ds18b20::startConversion() {
    bus_->reset();
    bus_->skip();
    bus_->write(kCmdConvertT);
}

float Ds18b20::readTempC() {
    if (!bus_->reset())
        return kDisconnectedC;

    bus_->skip();
    bus_->write(kCmdReadScratchpad);

    uint8_t data[kScratchpadSize];
    for (int i = 0; i < kScratchpadSize; i++) data[i] = bus_->read();

    // Byte 8 é o CRC dos 8 primeiros: descarta leitura corrompida por ruído
    if (crc8Maxim(data, kScratchpadSize - 1) != data[kScratchpadSize - 1])
        return kDisconnectedC;

    // Registrador de configuração (byte 4) tem bits fixos 0xx11111:
    // rejeita scratchpad todo 0x00 ou 0xFF, que passaria no CRC
    if ((data[4] & 0x9F) != 0x1F) return kDisconnectedC;

    int16_t raw = (data[1] << 8) | data[0];
    return raw / 16.0f;   // resolução de 12 bits: 0,0625 °C/LSB
}
