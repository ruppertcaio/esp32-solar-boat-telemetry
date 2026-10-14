#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

// ---------------------------------------------------------
// Classe base abstrata para todos os sensores do sistema
// ---------------------------------------------------------
// Cada sensor deve herdar desta classe e implementar
// obrigatoriamente o método readValue(), que retorna
// o valor físico lido (tensão, corrente, temperatura, etc.).
// ---------------------------------------------------------
class SensorBase {
public:

    // Destrutor virtual — garante destruição correta
    // em caso de utilização via ponteiros para a classe base.
    virtual ~SensorBase() {}

    // -----------------------------------------------------
    // Método virtual puro (obrigatório para sensores reais)
    // Cada sensor derivado deve implementar sua própria
    // rotina de leitura.
    // -----------------------------------------------------
    virtual float readValue() = 0;
};

#endif
