#include "SdCard.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

static const char *TAG = "SdCard";

// =====================================================
// Configuração validada em bancada (teste isolado do SD)
// =====================================================
constexpr spi_host_device_t kSpiHost   = SPI2_HOST;
constexpr int    kMaxOpenFiles         = 5;          // arquivos abertos simultâneos no VFS
constexpr size_t kAllocUnitBytes       = 16 * 1024;  // 16 KiB (só usado ao formatar)

SdCard::SdCard(gpio_num_t mosi, gpio_num_t miso, gpio_num_t sck, gpio_num_t cs)
    : mosi_(mosi), miso_(miso), sck_(sck), cs_(cs)
{
}

// =====================================================
// mount()
// 1. Inicializa o barramento SPI (pinos físicos)
// 2. Descreve o cartão nesse barramento (CS) e o host SD sobre SPI
// 3. Monta o FAT em kMountPoint
// =====================================================
esp_err_t SdCard::mount()
{
    if (card_ != nullptr) return ESP_OK;

    if (!spiInitialized_) {
        spi_bus_config_t busConfig = {};
        busConfig.mosi_io_num = mosi_;
        busConfig.miso_io_num = miso_;
        busConfig.sclk_io_num = sck_;
        busConfig.quadwp_io_num = -1;
        busConfig.quadhd_io_num = -1;

        esp_err_t ret = spi_bus_initialize(kSpiHost, &busConfig, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao iniciar o barramento SPI: %s", esp_err_to_name(ret));
            return ret;
        }
        spiInitialized_ = true;
    }

    sdspi_device_config_t slotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotConfig.gpio_cs = cs_;
    slotConfig.host_id = kSpiHost;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = kSpiHost;

    esp_vfs_fat_mount_config_t mountConfig = {};
    mountConfig.format_if_mount_failed = false;   // nunca apagar um cartão com dados
    mountConfig.max_files = kMaxOpenFiles;
    mountConfig.allocation_unit_size = kAllocUnitBytes;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slotConfig, &mountConfig, &card_);
    if (ret != ESP_OK) {
        card_ = nullptr;
        ESP_LOGW(TAG, "Falha ao montar o SD: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD montado em %s.", kMountPoint);
    return ESP_OK;
}

// =====================================================
// unmount()
// esp_vfs_fat_sdcard_unmount também remove o cartão do barramento,
// então em seguida o barramento pode ser liberado. Uma nova chamada
// a mount() reinicia tudo do zero (útil após remover o cartão).
// =====================================================
esp_err_t SdCard::unmount()
{
    esp_err_t ret = ESP_OK;

    if (card_ != nullptr) {
        ret = esp_vfs_fat_sdcard_unmount(kMountPoint, card_);
        card_ = nullptr;   // o handle não é mais válido, mesmo em erro
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Falha ao desmontar o SD: %s", esp_err_to_name(ret));
        }
    }

    if (spiInitialized_) {
        esp_err_t busRet = spi_bus_free(kSpiHost);
        if (busRet == ESP_OK) {
            spiInitialized_ = false;
        } else {
            ESP_LOGW(TAG, "Falha ao liberar o barramento SPI: %s", esp_err_to_name(busRet));
            if (ret == ESP_OK) ret = busRet;
        }
    }

    return ret;
}
