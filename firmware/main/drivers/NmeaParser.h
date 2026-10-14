#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <string>
#include <vector>

// --------------------------------------------------------------
// Parser NMEA 0183 (RMC e GGA)
// Classe pura (sem includes do ESP-IDF) para permitir teste no host.
// Aceita qualquer talker ID ($GPRMC, $GNRMC, $GLGGA, ...): o tipo
// da sentença é identificado pelos caracteres 3-5 do identificador.
// --------------------------------------------------------------
class NmeaParser {
public:
    static constexpr double kKnotsToKmh = 1.852;   // 1 nó = 1,852 km/h

    // Processa uma linha completa (sem "\r\n").
    // Retorna false somente se o checksum for inválido; sentenças
    // de outros tipos são ignoradas e retornam true.
    bool parseLine(const std::string &line);

    double latitude() const { return latitude_; }    // graus decimais (S negativo)
    double longitude() const { return longitude_; }  // graus decimais (W negativo)
    double speedKmh() const { return speedKmh_; }
    int satellites() const { return satellites_; }
    bool hasFix() const { return hasFix_; }

    // Valida "$...*HH". Linha sem '*' é aceita (compatibilidade com
    // módulos que não enviam checksum).
    static bool validateChecksum(const std::string &line);

    // Converte ddmm.mmmm (lat) ou dddmm.mmmm (lon) para graus decimais.
    // Hemisfério S ou W torna o valor negativo.
    static double toDecimalDegrees(const std::string &coord, const std::string &hemisphere);

private:
    double latitude_ = 0.0;
    double longitude_ = 0.0;
    double speedKmh_ = 0.0;
    int satellites_ = 0;
    bool hasFix_ = false;

    void parseRmc(const std::vector<std::string> &tokens);
    void parseGga(const std::vector<std::string> &tokens);
};

#endif
