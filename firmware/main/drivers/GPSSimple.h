#ifndef GPSSimple_H
#define GPSSimple_H

#include <string>

#include "NmeaParser.h"

#include "driver/uart.h"
#include "esp_err.h"

// --------------------------------------------------------------
// GPS via UART (polling). Monta as linhas NMEA recebidas e
// delega a interpretação ao NmeaParser.
// --------------------------------------------------------------
class GPSSimple {
public:
    GPSSimple(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate = 9600);

    esp_err_t begin();   // Configura e instala o driver UART
    void update();       // Lê bytes disponíveis e processa linhas completas

    double getLatitude() const { return parser.latitude(); }
    double getLongitude() const { return parser.longitude(); }
    double getSpeedKmph() const { return parser.speedKmh(); }
    int getSatellites() const { return parser.satellites(); }
    bool hasFix() const { return parser.hasFix(); }

private:

    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int baud_rate;

    std::string nmea_buffer;
    NmeaParser parser;

    void handleNMEALine(const std::string &line);
};

#endif
