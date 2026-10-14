#ifndef SENSOR_BATTERY_VOLTAGE_H
#define SENSOR_BATTERY_VOLTAGE_H

#include <cmath>

#include "SensorBase.h"
#include "ADS1115.h"
#include "Average.h"

// -----------------------------------------------------
// Sensor de tensão da bateria
// Leitura via ADS1115 + aplicação de fator de escala
// + filtro de média móvel
// -----------------------------------------------------
class SensorBatteryVoltage : public SensorBase {
public:
    static constexpr size_t kWindow = 10;   // janela da média móvel [amostras]

    // Construtor:
    // ads    → ponteiro para o driver ADS1115
    // ch     → canal usado no ADS1115
    // factor → fator do divisor resistivo (escala Vreal = Vadc * factor)
    SensorBatteryVoltage(ADS1115* ads, ADS1115::Channel ch, float factor)
        : adc_(ads), channel_(ch), factorScale_(factor) {}

    // -------------------------------------------------
    // Leitura da tensão da bateria
    // -------------------------------------------------
    float readValue() override {

        float vadc = adc_->readChannel(channel_);

        // Falha de I2C: mantém o último valor filtrado
        if (std::isnan(vadc)) return filter_.value();

        // Aplica o fator do divisor resistivo e a média móvel
        filter_.addSample(vadc * factorScale_);

        // Retorna valor filtrado
        return filter_.value();
    }

private:
    ADS1115 *adc_;                     // Ponteiro para o ADS1115
    ADS1115::Channel channel_;         // Canal do ADS usado para leitura
    float factorScale_;                // Fator do divisor resistivo
    Average<float, kWindow> filter_;   // Filtro de média móvel
};

#endif
