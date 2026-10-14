#ifndef SENSOR_NODE_APP_H
#define SENSOR_NODE_APP_H

//===================== INCLUDES ==========================
#include "ADS1115.h"
#include "Hd44780I2c.h"
#include "GPSSimple.h"

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

//===================== ESTRUTURAS DE DADOS ==========================

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
};

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

    //-------- Inicializações internas --------
    void initI2C();  // Configura o barramento I2C
    void initLCD();  // Inicia o LCD
    void initADS();  // Inicia o ADS1115
    bool initGPS();  // Inicia GPS (false se a UART falhar)

    //-------- Tasks do FreeRTOS --------
    static void taskSensors(void *arg);   // Leitura dos sensores
    static void taskGPS(void *arg);       // Leitura do GPS
    static void taskDisplay(void *arg);   // Atualiza o LCD
};

#endif
