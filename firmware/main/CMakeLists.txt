idf_component_register(
    SRCS
        main.cpp

        # App
        app/SensorNodeApp.cpp

        # Drivers
        drivers/ADS1115.cpp
        drivers/Ds18b20.cpp
        drivers/GPSSimple.cpp
        drivers/Hd44780I2c.cpp
        drivers/NmeaParser.cpp
        drivers/OneWireBus.cpp

        # Sensors
        sensors/SensorTemperature.cpp

    INCLUDE_DIRS
        .
        app
        drivers
        sensors
        utils

    REQUIRES
        esp_driver_i2c
        esp_driver_uart
        esp_driver_gpio
        esp_rom
        esp_common
        freertos
        log
)
