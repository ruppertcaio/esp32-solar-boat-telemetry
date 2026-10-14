#ifndef CRC8_H
#define CRC8_H

#include <cstddef>
#include <cstdint>

// --------------------------------------------------------------
// CRC-8 Dallas/Maxim (1-Wire)
// Polinômio x^8 + x^5 + x^4 + 1 (0x31), forma refletida 0x8C,
// valor inicial 0x00, sem XOR final.
// Função pura (sem includes do ESP-IDF) para permitir teste no host.
// Propriedade útil: o CRC calculado sobre (dados + CRC) resulta 0.
// --------------------------------------------------------------
inline uint8_t crc8Maxim(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x01) ? static_cast<uint8_t>((crc >> 1) ^ 0x8C)
                               : static_cast<uint8_t>(crc >> 1);
        }
    }
    return crc;
}

#endif
