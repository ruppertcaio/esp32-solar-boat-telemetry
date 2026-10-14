#ifndef SD_CARD_H
#define SD_CARD_H

#include "driver/gpio.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"

// --------------------------------------------------------------
// Cartão SD em modo SPI (SPI2_HOST) com sistema de arquivos FAT
// registrado no VFS. Depois de mount(), os arquivos são acessados
// com a stdio comum (fopen/fputs) em "/sdcard/...".
// O barramento SPI é exclusivo do cartão: unmount() também o libera.
// --------------------------------------------------------------
class SdCard {
public:
    static constexpr const char *kMountPoint = "/sdcard";

    SdCard(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sck, gpio_num_t cs);

    esp_err_t mount();     // Inicia o SPI (se preciso) e monta o FAT
    esp_err_t unmount();   // Desmonta o FAT e libera o barramento SPI
    bool isMounted() const { return card_ != nullptr; }

private:
    gpio_num_t mosi_;
    gpio_num_t miso_;
    gpio_num_t sck_;
    gpio_num_t cs_;

    sdmmc_card_t *card_ = nullptr;   // != nullptr enquanto montado
    bool spiInitialized_ = false;
};

#endif
