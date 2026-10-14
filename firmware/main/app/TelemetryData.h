#ifndef TELEMETRY_DATA_H
#define TELEMETRY_DATA_H

// --------------------------------------------------------------
// Estruturas de dados compartilhadas entre as tasks.
// Header puro (sem includes do ESP-IDF) para permitir teste no host.
// --------------------------------------------------------------

// Estrutura com dados dos sensores analógicos
struct SensorData {
    float voltageBat = 0.0f;     // Tensão da bateria
    float voltageAux = 0.0f;     // Tensão auxiliar
    float current = 0.0f;        // Corrente medida
    float temperature = 0.0f;    // Temperatura DS18B20
};

// Estrutura com dados do GPS
struct GPSInfo {
    double lat = 0.0f;   // Latitude
    double lng = 0.0f;   // Longitude
    int sats = 0;        // Número de satélites
    double speed = 0.0f; // Velocidade km/h
    bool fix = false;    // Fix válido: sem ele posição/velocidade não valem
};

#endif
