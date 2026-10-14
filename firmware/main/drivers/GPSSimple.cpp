#include "GPSSimple.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static const char *TAG = "GPSSimple";

GPSSimple::GPSSimple(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate)
    : uart_num(uart_num), tx_pin(tx_pin), rx_pin(rx_pin), baud_rate(baud_rate) {}

esp_err_t GPSSimple::begin() {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };

    esp_err_t ret = uart_param_config(uart_num, &uart_config);
    if (ret == ESP_OK) {
        ret = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (ret == ESP_OK) {
        ret = uart_driver_install(uart_num, 2048, 0, 0, NULL, 0);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar UART%d: %s", (int)uart_num, esp_err_to_name(ret));
    }
    return ret;
}

void GPSSimple::update() {
    uint8_t data[256];
    int len = uart_read_bytes(uart_num, data, sizeof(data), 20 / portTICK_PERIOD_MS);

    if (len > 0) {
        for (int i = 0; i < len; i++) {
            char c = static_cast<char>(data[i]);

            if (c == '\n') {

                if (!nmea_buffer.empty() && nmea_buffer.back() == '\r') {
                    nmea_buffer.pop_back();
                }

                if (!nmea_buffer.empty()) {
                    handleNMEALine(nmea_buffer);
                }
                nmea_buffer.clear();
            } else {

                if (c != '\r') {
                    nmea_buffer += c;
                }

                // Sentença NMEA tem no máximo 82 caracteres: descarta lixo sem '\n'
                if (nmea_buffer.size() > 82) {
                    nmea_buffer.clear();
                }
            }
        }
    }
}

void GPSSimple::handleNMEALine(const std::string &line) {
    if (!parser.parseLine(line)) {
        ESP_LOGW(TAG, "Checksum inválido: %s", line.c_str());
    }
}
