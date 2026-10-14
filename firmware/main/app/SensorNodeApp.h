#ifndef SENSOR_NODE_APP_H
#define SENSOR_NODE_APP_H

//===================== INCLUDES ==========================
#include <cstdio>

#include "ADS1115.h"
#include "Hd44780I2c.h"
#include "GPSSimple.h"
#include "SdCard.h"
#include "TelemetryData.h"

#include "SensorBatteryVoltage.h"
#include "SensorAuxVoltage.h"
#include "SensorCurrent.h"
#include "SensorTemperature.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

//===================== CONFIGURAÇÕES ==========================

// Barramento I2C (compartilhado por LCD e ADS1115)
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

// LCD I2C
#define DISP_ADDR 0x27
#define DISP_COLS 20
#define DISP_ROWS 4

// ADS1115
#define ADS_ADDR 0x48

// Sensor de temperatura DS18B20
#define PIN_TEMP GPIO_NUM_4

// GPS UART
#define GPS_RX GPIO_NUM_16
#define GPS_TX GPIO_NUM_17
#define GPS_BAUD 9600

// Cartão SD (SPI2_HOST), módulo alimentado em 3V3
#define SD_MOSI GPIO_NUM_23
#define SD_MISO GPIO_NUM_19
#define SD_SCK  GPIO_NUM_18
#define SD_CS   GPIO_NUM_5

//===================== ESTRUTURAS DE DADOS ==========================
// SensorData e GPSInfo ficam em TelemetryData.h (sem ESP-IDF, testável no host)

//===================== CLASSE PRINCIPAL ==========================

class SensorNodeApp {
public:
    SensorNodeApp();   // Construtor
    void start();      // Inicia o sistema e as tasks

private:

    //-------- Barramento I2C --------
    i2c_master_bus_handle_t i2cBus_ = nullptr;

    //-------- Drivers principais --------
    ADS1115 ads_;      // Conversor ADC ADS1115
    Hd44780I2c lcd_;   // Display LCD com I2C
    GPSSimple gps_;    // Módulo GPS
    SdCard sd_;        // Cartão SD (log CSV)

    //-------- Sensores derivados (ADS1115) --------
    SensorBatteryVoltage batterySensor_;
    SensorAuxVoltage auxSensor_;
    SensorCurrent currentSensor_;

    //-------- Sensor digital --------
    SensorTemperature tempSensor_;   // DS18B20 (1-Wire)

    //-------- Estruturas de dados --------
    SensorData sensorData_;   // Armazena medições
    GPSInfo gpsInfo_;         // Armazena informações do GPS

    //-------- Sincronização --------
    SemaphoreHandle_t mutex_ = nullptr; // Protege acesso aos dados

    //-------- Handles das tasks --------
    TaskHandle_t taskSensorsHandle_ = nullptr;
    TaskHandle_t taskGPSHandle_ = nullptr;
    TaskHandle_t taskDisplayHandle_ = nullptr;
    TaskHandle_t taskLoggerHandle_ = nullptr;

    //-------- Inicializações internas --------
    void initI2C();  // Configura o barramento I2C
    void initLCD();  // Inicia o LCD
    void initADS();  // Inicia o ADS1115
    bool initGPS();  // Inicia GPS (false se a UART falhar)
    void initSD();   // Monta o SD (falha só gera aviso; o logger tenta de novo)

    FILE *openNewLogFile();   // Próximo LOGnnnn.CSV com cabeçalho; nullptr em falha
    bool logFull_ = false;    // LOG9999.CSV já existe: logger desiste

    //-------- Tasks do FreeRTOS --------
    static void taskSensors(void *arg);   // Leitura dos sensores
    static void taskGPS(void *arg);       // Leitura do GPS
    static void taskDisplay(void *arg);   // Atualiza o LCD
    static void taskLogger(void *arg);    // Grava o log CSV no SD
};

#endif
