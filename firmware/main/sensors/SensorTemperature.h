#ifndef SENSOR_TEMPERATURE_H
#define SENSOR_TEMPERATURE_H

#include "SensorBase.h"
#include "Average.h"
#include "OneWireBus.h"
#include "Ds18b20.h"

#include "driver/gpio.h"
#include "esp_err.h"

// --------------------------------------------------------------
// Classe responsável pela leitura do sensor DS18B20
// --------------------------------------------------------------
// Utiliza:
//   - Protocolo 1-Wire (classe OneWireBus)
//   - Driver Ds18b20 para conversão e comandos
//   - Filtro de média (Average) para suavizar leituras ruidosas
// --------------------------------------------------------------
// Esta classe implementa a interface SensorBase, garantindo
// compatibilidade com o sistema modular de sensores do projeto.
// --------------------------------------------------------------
class SensorTemperature : public SensorBase {
public:
    static constexpr size_t kWindow = 20;   // janela da média móvel [amostras]

    // ----------------------------------------------------------
    // Construtor
    // pin -> GPIO conectado ao DS18B20
    // ----------------------------------------------------------
    explicit SensorTemperature(gpio_num_t pin);

    // ----------------------------------------------------------
    // Inicialização do sensor (verifica presença no barramento)
    // Deve ser chamada antes de qualquer leitura.
    // ----------------------------------------------------------
    esp_err_t begin();

    // ----------------------------------------------------------
    // Leitura da temperatura em °C
    // Retorna o valor filtrado pelo algoritmo de média móvel.
    // ----------------------------------------------------------
    float readValue() override;

private:

    // Instância do barramento 1-Wire (um fio)
    OneWireBus oneWire_;

    // Driver do sensor DS18B20
    Ds18b20 ds18b20_;

    // Filtro de média para suavizar a leitura
    Average<float, kWindow> filter_;
};

#endif
