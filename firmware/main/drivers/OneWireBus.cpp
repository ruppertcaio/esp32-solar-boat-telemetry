#include "OneWireBus.h"

#include "esp_rom_sys.h"

OneWireBus::OneWireBus(gpio_num_t pin) : pin_(pin) {
    gpio_reset_pin(pin_);   // garante função GPIO no IOMUX
    gpio_set_direction(pin_, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(pin_, GPIO_PULLUP_ONLY);
    gpio_set_level(pin_, 1);   // linha em repouso (liberada)
}

bool OneWireBus::reset() {
    // Linha presa em nível baixo (curto ou sem pull-up) não é presença
    if (gpio_get_level(pin_) == 0) return false;

    portENTER_CRITICAL(&mux_);
    gpio_set_level(pin_, 0);
    esp_rom_delay_us(480);
    gpio_set_level(pin_, 1);
    esp_rom_delay_us(70);

    bool presence = gpio_get_level(pin_) == 0;
    portEXIT_CRITICAL(&mux_);

    esp_rom_delay_us(410);   // fim do slot de presença (fora da seção crítica)

    return presence;
}

void OneWireBus::writeBit(int bit) {
    portENTER_CRITICAL(&mux_);
    gpio_set_level(pin_, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    gpio_set_level(pin_, 1);
    esp_rom_delay_us(bit ? 64 : 10);
    portEXIT_CRITICAL(&mux_);
}

int OneWireBus::readBit() {
    portENTER_CRITICAL(&mux_);
    gpio_set_level(pin_, 0);
    esp_rom_delay_us(6);
    gpio_set_level(pin_, 1);
    esp_rom_delay_us(8);   // amostra antes de 15 us do início do slot

    int bit = gpio_get_level(pin_);
    portEXIT_CRITICAL(&mux_);

    esp_rom_delay_us(55);   // recuperação (fora da seção crítica)
    return bit;
}

void OneWireBus::write(uint8_t byte) {
    for (int i=0; i<8; i++) {
        writeBit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t OneWireBus::read() {
    uint8_t v = 0;
    for (int i=0; i<8; i++) {
        v |= (readBit() << i);
    }
    return v;
}

void OneWireBus::skip() {
    write(0xCC);
}
