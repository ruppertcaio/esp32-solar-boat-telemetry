#ifndef SENSOR_CURRENT_H
#define SENSOR_CURRENT_H

#include <cmath>

#include "SensorBase.h"
#include "ADS1115.h"
#include "Average.h"

// =====================================================
// CALIBRAÇÃO – sensor de corrente de efeito Hall
// Valores nominais; ajustar na bancada para o sensor instalado.
// =====================================================
constexpr float kHallOffsetV     = 2.5f;    // saída do sensor com 0 A [V]
constexpr float kHallAmpsPerVolt = 100.0f;  // ganho [A/V] (= 10 mV/A)

// -----------------------------------------------------
// Classe de leitura de corrente
// Implementa SensorBase e usa média móvel para suavização
// -----------------------------------------------------
class SensorCurrent : public SensorBase {
public:
    // Janela menor que a das tensões (original: filterWindow * 0.5 = 5)
    static constexpr size_t kWindow = 5;    // janela da média móvel [amostras]

    // Construtor:
    // ads   → ponteiro para ADS1115
    // ch    → canal do ADS a ser lido
    SensorCurrent(ADS1115* ads, ADS1115::Channel ch)
        : ads_(ads), channel_(ch) {}

    // -------------------------------------------------
    // Leitura da corrente
    // -------------------------------------------------
    float readValue() override {

        // Leitura da tensão bruta do ADS1115
        float voltage = ads_->readChannel(channel_);

        // Falha de I2C: mantém o último valor filtrado
        if (std::isnan(voltage)) return filter_.value();

        // Converte tensão em corrente usando offset e escala
        float current = (voltage - kHallOffsetV) * kHallAmpsPerVolt;

        // Aplica média móvel
        filter_.addSample(current);

        // Retorna valor filtrado
        return filter_.value();
    }

private:
    ADS1115* ads_;                     // Driver ADS1115
    ADS1115::Channel channel_;         // Canal de leitura
    Average<float, kWindow> filter_;   // Filtro de média móvel
};

#endif
