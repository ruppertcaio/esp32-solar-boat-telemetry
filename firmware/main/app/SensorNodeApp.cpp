#include "SensorNodeApp.h"

#include <dirent.h>
#include <unistd.h>

#include "CsvLog.h"
#include "esp_timer.h"

static const char *TAG = "SensorNodeApp";

// =====================================================
// CALIBRAÇÃO – divisor resistivo das entradas de tensão
// Vreal = Vadc * (R1 + R2) / R1, com R1 = 1k e R2 = 10k
// =====================================================
constexpr float kDividerRatio = 11.0f;

// =====================================================
// LOG NO SD
// =====================================================
constexpr uint32_t kLogPeriodMs   = 1000;   // uma linha CSV por segundo
constexpr uint32_t kSdRetryMs     = 5000;   // nova tentativa após falha (cartão removido)
constexpr size_t   kCsvLineMaxLen = 160;    // folga sobre a linha típica (~60 caracteres)
constexpr size_t   kLogPathMaxLen = 32;     // "/sdcard/LOGnnnn.CSV" + '\0'

// fflush + fsync a cada N linhas. Compromisso: N pequeno perde menos
// dados num corte de energia (até N s de log), mas cada fsync grava o
// setor de dados e a FAT, o que desgasta o flash do cartão e pode
// bloquear a task por dezenas de ms. Com N = 10 perde-se no máximo ~10 s.
constexpr int kSyncEveryLines = 10;

// =====================================================
// CONSTRUTOR
// =====================================================
SensorNodeApp::SensorNodeApp()
    : ads_(ADS_ADDR),                                   // ADS1115 (bus definido em start)
      lcd_(DISP_ADDR),                                  // LCD (bus definido em start)
      gps_(UART_NUM_2, GPS_TX, GPS_RX, GPS_BAUD),       // Inicializa GPS
      sd_(SD_MOSI, SD_MISO, SD_SCK, SD_CS),             // SD (montado em start)
      batterySensor_(&ads_, ADS1115::CH0, kDividerRatio),
      auxSensor_(&ads_, ADS1115::CH1, kDividerRatio),
      currentSensor_(&ads_, ADS1115::CH3),
      tempSensor_(PIN_TEMP)
{
}

// =====================================================
// START – Inicializa hardware e cria tasks
// =====================================================
void SensorNodeApp::start() {
    // Cria mutex para proteger dados compartilhados
    mutex_ = xSemaphoreCreateMutex();
    if (mutex_ == nullptr) {
        ESP_LOGE(TAG, "Falha ao criar mutex; sistema não iniciado.");
        return;
    }

    initI2C();     // Inicia barramento I2C
    initLCD();     // Inicia display
    initADS();     // Inicia ADS1115
    bool gpsOk = initGPS();  // Inicia GPS

    // Inicia o DS18B20
    if (tempSensor_.begin() != ESP_OK) {
        ESP_LOGW(TAG, "DS18B20 sem pulso de presença no GPIO%d.", (int)PIN_TEMP);
    }

    initSD();      // Por último: sem cartão o sistema segue sem log

    // Cria tasks do FreeRTOS
    if (xTaskCreate(taskSensors, "taskSensors", 4096, this, 2, &taskSensorsHandle_) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar taskSensors.");
    }
    if (gpsOk && xTaskCreate(taskGPS, "taskGPS", 4096, this, 2, &taskGPSHandle_) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar taskGPS.");
    }
    if (xTaskCreate(taskDisplay, "taskDisplay", 4096, this, 1, &taskDisplayHandle_) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar taskDisplay.");
    }
    if (xTaskCreate(taskLogger, "taskLogger", 4096, this, 1, &taskLoggerHandle_) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar taskLogger.");
    }
}

// =====================================================
// Inicialização do I2C
// =====================================================
void SensorNodeApp::initI2C() {
    i2c_master_bus_config_t conf = {};
    conf.i2c_port = I2C_NUM_0;
    conf.sda_io_num = I2C_SDA;
    conf.scl_io_num = I2C_SCL;
    conf.clk_source = I2C_CLK_SRC_DEFAULT;
    conf.glitch_ignore_cnt = 7;
    conf.flags.enable_internal_pullup = true;  // a velocidade (100 kHz) é definida por dispositivo

    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &i2cBus_));

    ESP_LOGI(TAG, "I2C iniciado.");
}

// =====================================================
// Inicialização do LCD
// =====================================================
void SensorNodeApp::initLCD() {
    esp_err_t ret = lcd_.begin(i2cBus_, DISP_COLS, DISP_ROWS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD não encontrado em 0x%02X: %s", DISP_ADDR, esp_err_to_name(ret));
        return;
    }
    lcd_.printLine(0, "Telemetria     (1/2)"); // Texto inicial
}

// =====================================================
// Inicialização do ADS1115 e GPS
// (begin() de cada driver já registra falhas no log)
// =====================================================
void SensorNodeApp::initADS() { ads_.begin(i2cBus_); }
bool SensorNodeApp::initGPS() { return gps_.begin() == ESP_OK; }

// =====================================================
// Inicialização do cartão SD
// =====================================================
void SensorNodeApp::initSD() {
    if (sd_.mount() != ESP_OK) {
        ESP_LOGW(TAG, "SD indisponível; seguindo sem log (nova tentativa a cada %u s).",
                 (unsigned)(kSdRetryMs / 1000));
    }
}

// =====================================================
// Abre o próximo LOGnnnn.CSV (maior índice no cartão + 1)
// e grava o cabeçalho. Em qualquer falha desmonta o cartão,
// para a próxima tentativa recomeçar do zero.
// =====================================================
FILE *SensorNodeApp::openNewLogFile() {
    if (!sd_.isMounted() && sd_.mount() != ESP_OK) return nullptr;

    // Sem listar o diretório não se sabe o próximo índice; abrir
    // às cegas com "w" poderia sobrescrever um log existente.
    DIR *dir = opendir(SdCard::kMountPoint);
    if (dir == nullptr) {
        ESP_LOGW(TAG, "Falha ao listar %s.", SdCard::kMountPoint);
        sd_.unmount();
        return nullptr;
    }
    int lastIndex = 0;
    while (struct dirent *entry = readdir(dir)) {
        int index = parseLogIndex(entry->d_name);
        if (index > lastIndex) lastIndex = index;
    }
    closedir(dir);

    char name[16];
    if (!nextLogFileName(name, sizeof(name), lastIndex)) {
        ESP_LOGE(TAG, "Limite de arquivos LOGnnnn.CSV atingido; log desativado.");
        logFull_ = true;
        sd_.unmount();
        return nullptr;
    }

    char path[kLogPathMaxLen];
    snprintf(path, sizeof(path), "%s/%s", SdCard::kMountPoint, name);

    FILE *file = fopen(path, "w");
    if (file == nullptr || fputs(csvHeader(), file) < 0) {
        ESP_LOGW(TAG, "Falha ao criar %s.", path);
        if (file != nullptr) fclose(file);
        sd_.unmount();
        return nullptr;
    }

    ESP_LOGI(TAG, "Gravando log em %s.", path);
    return file;
}

// =====================================================
// TASK: Leitura dos sensores analógicos + temperatura
// =====================================================
void SensorNodeApp::taskSensors(void *arg) {
    SensorNodeApp *app = static_cast<SensorNodeApp*>(arg);

    while (true) {

        // Leitura dos sensores
        float vbat        = app->batterySensor_.readValue();
        float vaux        = app->auxSensor_.readValue();
        float current     = app->currentSensor_.readValue();
        float temperature = app->tempSensor_.readValue();

        // Atualiza dados protegidos por mutex
        xSemaphoreTake(app->mutex_, portMAX_DELAY);
        app->sensorData_.voltageBat  = vbat;
        app->sensorData_.voltageAux  = vaux;
        app->sensorData_.current     = current;
        app->sensorData_.temperature = temperature;
        xSemaphoreGive(app->mutex_);

        vTaskDelay(pdMS_TO_TICKS(500)); // Período de leitura
    }
}

// =====================================================
// TASK: Atualização dos dados do GPS
// =====================================================
void SensorNodeApp::taskGPS(void *arg) {
    SensorNodeApp *app = static_cast<SensorNodeApp*>(arg);

    while (true) {
        app->gps_.update(); // Atualiza buffer interno do GPS

        // Copia dados com mutex
        xSemaphoreTake(app->mutex_, portMAX_DELAY);
        app->gpsInfo_.lat   = app->gps_.getLatitude();
        app->gpsInfo_.lng   = app->gps_.getLongitude();
        app->gpsInfo_.sats  = app->gps_.getSatellites();
        app->gpsInfo_.speed = app->gps_.getSpeedKmph();
        app->gpsInfo_.fix   = app->gps_.hasFix();
        xSemaphoreGive(app->mutex_);

        vTaskDelay(pdMS_TO_TICKS(300)); // Taxa de atualização do GPS
    }
}

// =====================================================
// TASK: Atualização do display LCD
// Cada linha é reescrita inteira (completada com espaços),
// sem clear(), para não piscar.
// =====================================================
void SensorNodeApp::taskDisplay(void *arg) {
    SensorNodeApp *app = static_cast<SensorNodeApp*>(arg);
    Hd44780I2c &lcd = app->lcd_;

    while (true) {

        for(int i = 0; i<20; i++){

            // Lê dados limpos usando mutex
            xSemaphoreTake(app->mutex_, portMAX_DELAY);
            float vbat   = app->sensorData_.voltageBat;
            float current = app->sensorData_.current;
            float aux    = app->sensorData_.voltageAux;
            float temp   = app->sensorData_.temperature;

            double lng = app->gpsInfo_.lng;
            double lat = app->gpsInfo_.lat;

            int sats     = app->gpsInfo_.sats;
            int vel      = app->gpsInfo_.speed;
            xSemaphoreGive(app->mutex_);

            // Evita exibir valores inválidos
            if (vbat < 0)   vbat = 0;
            if (current < 0) current = 0;
            if (aux < 0)    aux = 0;
            if (temp < 0)   temp = 0;

            if(i<=15){
                lcd.printLine(0, "Telemetria     (1/2)");
                lcd.printLine(1, "Vel: %dKm/h Sat: %d", vel, sats);
                lcd.printLine(2, "Vb: %.2fV", vbat);
                lcd.printLine(3, "I: %.2fA", current);
            }
            else{
                lcd.printLine(0, "Telemetria     (2/2)");
                lcd.printLine(1, "Aux: %.2fV T: %.1fC", aux, temp);
                lcd.printLine(2, "Lng: %.8f", lng);
                lcd.printLine(3, "Lat: %.8f", lat);
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Atualização visual
        }
    }
}

// =====================================================
// TASK: Log CSV no cartão SD (uma linha por segundo)
// O mutex só protege a cópia dos dados; a escrita no SD,
// que pode demorar, acontece fora dele.
// Em falha de escrita (ex.: cartão removido) fecha o arquivo,
// desmonta e tenta remontar + abrir um arquivo novo a cada
// kSdRetryMs.
// =====================================================
void SensorNodeApp::taskLogger(void *arg) {
    SensorNodeApp *app = static_cast<SensorNodeApp*>(arg);

    // Períodos a esperar antes de uma nova tentativa de abrir o log
    constexpr int kRetryPeriods = kSdRetryMs / kLogPeriodMs - 1;

    FILE *file = nullptr;
    int linesSinceSync = 0;
    int waitPeriods = 0;
    char line[kCsvLineMaxLen];
    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kLogPeriodMs));

        if (file == nullptr) {
            if (waitPeriods > 0) {
                waitPeriods--;
                continue;
            }
            file = app->openNewLogFile();
            // Montagem/listagem pode passar de 1 s: evita rajada de iterações
            lastWake = xTaskGetTickCount();
            if (file == nullptr) {
                if (app->logFull_) vTaskDelete(nullptr);   // sem nome livre: não insiste
                waitPeriods = kRetryPeriods;
                continue;
            }
            linesSinceSync = 0;
        }

        // Copia um snapshot sob o mutex
        xSemaphoreTake(app->mutex_, portMAX_DELAY);
        SensorData sensors = app->sensorData_;
        GPSInfo gps = app->gpsInfo_;
        xSemaphoreGive(app->mutex_);

        uint32_t uptimeMs = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        if (formatCsvLine(line, sizeof(line), uptimeMs, sensors, gps) < 0) {
            ESP_LOGW(TAG, "Linha CSV truncada; descartada.");
            continue;
        }

        bool ok = fputs(line, file) >= 0;
        if (ok && ++linesSinceSync >= kSyncEveryLines) {
            ok = fflush(file) == 0 && fsync(fileno(file)) == 0;
            linesSinceSync = 0;
            lastWake = xTaskGetTickCount();   // fsync lento não gera rajada
        }

        if (!ok) {
            ESP_LOGW(TAG, "Falha de escrita no SD; nova tentativa em %u s.",
                     (unsigned)(kSdRetryMs / 1000));
            fclose(file);
            file = nullptr;
            app->sd_.unmount();
            waitPeriods = kRetryPeriods;
        }
    }
}
