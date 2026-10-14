#include "SensorNodeApp.h"

extern "C" void app_main() {
    static SensorNodeApp app;
    app.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
