#include "SensorTemperature.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --------------------------------------------------------------
// Construtor
// Inicializa:
//   - OneWireBus no pino informado
//   - Ds18b20 usando o barramento 1-Wire
// --------------------------------------------------------------
SensorTemperature::SensorTemperature(gpio_num_t pin)
    : oneWire_(pin), ds18b20_(&oneWire_)
{
}

// --------------------------------------------------------------
// begin()
// Verifica se o DS18B20 responde (pulso de presença).
// --------------------------------------------------------------
esp_err_t SensorTemperature::begin() {
    return ds18b20_.begin();
}

// --------------------------------------------------------------
// readValue()
// - Solicita a conversão de temperatura ao DS18B20
// - Aguarda o tempo necessário da conversão (~750ms)
// - Lê a temperatura em °C
// - Trata leituras inválidas (sem presença ou CRC inválido)
// - Aplica filtro de média móvel para suavizar valores
// --------------------------------------------------------------
float SensorTemperature::readValue() {

    // Solicita conversão de temperatura
    ds18b20_.startConversion();

    // Aguardando tempo de conversão do DS18B20
    vTaskDelay(pdMS_TO_TICKS(750));

    // Lê temperatura do sensor
    float t = ds18b20_.readTempC();

    // Se valor inválido, retorna último filtrado
    if (t <= -100.0f) {
        return filter_.value();
    }

    // Adiciona nova leitura ao filtro
    filter_.addSample(t);

    // Retorna valor suavizado
    return filter_.value();
}
