#ifndef SENSOR_AUX_VOLTAGE_H
#define SENSOR_AUX_VOLTAGE_H

#include <cmath>

#include "SensorBase.h"
#include "ADS1115.h"
#include "Average.h"

// -----------------------------------------------------
// Sensor de tensão auxiliar
// Utiliza ADS1115 + fator de escala (divisor resistivo)
// e filtro de média móvel
// -----------------------------------------------------
class SensorAuxVoltage : public SensorBase {
public:
    static constexpr size_t kWindow = 10;   // janela da média móvel [amostras]

    // Construtor:
    // ads    → ponteiro para o driver do ADS1115
    // ch     → canal do ADS usado para leitura
    // factor → fator do divisor resistivo (Vreal = Vadc * factor)
    SensorAuxVoltage(ADS1115 *ads, ADS1115::Channel ch, float factor)
        : adc_(ads), channel_(ch), factorScale_(factor) {}

    // -------------------------------------------------
    // Leitura da tensão auxiliar
    // -------------------------------------------------
    float readValue() override {

        float vadc = adc_->readChannel(channel_);

        // Falha de I2C: mantém o último valor filtrado
        if (std::isnan(vadc)) return filter_.value();

        // Converte em tensão real via fator de escala e aplica a média móvel
        filter_.addSample(vadc * factorScale_);

        // Retorna o valor filtrado
        return filter_.value();
    }

private:
    ADS1115 *adc_;                     // Ponteiro para ADC ADS1115
    ADS1115::Channel channel_;         // Canal de leitura no ADS1115
    float factorScale_;                // Fator de escala do divisor resistivo
    Average<float, kWindow> filter_;   // Filtro de média móvel
};

#endif
