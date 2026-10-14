#include "NmeaParser.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>

// =====================================================
// Funções auxiliares internas
// =====================================================
namespace {

// Separa a sentença em campos por ',' (para no '*' do checksum)
std::vector<std::string> splitNmeaLine(const std::string &line) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : line) {
        if (c == '*') break;
        if (c == ',') {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

// Valor de um dígito hexadecimal, ou -1 se inválido
int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// "$GPRMC" -> true para type "RMC" (qualquer talker ID de 2 letras)
bool isSentence(const std::string &id, const char *type) {
    return id.size() == 6 && id[0] == '$' && id.compare(3, 3, type) == 0;
}

} // namespace

// =====================================================
// Processamento de uma linha
// =====================================================
bool NmeaParser::parseLine(const std::string &line) {
    if (!validateChecksum(line)) return false;

    auto tokens = splitNmeaLine(line);
    if (tokens.empty()) return true;

    if (isSentence(tokens[0], "RMC")) {
        parseRmc(tokens);
    } else if (isSentence(tokens[0], "GGA")) {
        parseGga(tokens);
    }
    return true;
}

// =====================================================
// RMC: $xxRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,...
// tokens[2] = status (A = válido)
// tokens[3] = latitude, tokens[4] = N/S
// tokens[5] = longitude, tokens[6] = E/W
// tokens[7] = velocidade em nós
// =====================================================
void NmeaParser::parseRmc(const std::vector<std::string> &tokens) {
    if (tokens.size() < 8) return;

    hasFix_ = (tokens[2] == "A");

    if (!tokens[3].empty() && !tokens[4].empty()) {
        latitude_ = toDecimalDegrees(tokens[3], tokens[4]);
    }
    if (!tokens[5].empty() && !tokens[6].empty()) {
        longitude_ = toDecimalDegrees(tokens[5], tokens[6]);
    }
    if (!tokens[7].empty()) {
        speedKmh_ = std::atof(tokens[7].c_str()) * kKnotsToKmh;
    }
}

// =====================================================
// GGA: $xxGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,quality,numSat,...
// tokens[2] = latitude, tokens[3] = N/S
// tokens[4] = longitude, tokens[5] = E/W
// tokens[6] = qualidade do fix (0 = inválido), tokens[7] = satélites
// =====================================================
void NmeaParser::parseGga(const std::vector<std::string> &tokens) {
    if (tokens.size() < 8) return;

    if (!tokens[2].empty() && !tokens[3].empty()) {
        latitude_ = toDecimalDegrees(tokens[2], tokens[3]);
    }
    if (!tokens[4].empty() && !tokens[5].empty()) {
        longitude_ = toDecimalDegrees(tokens[4], tokens[5]);
    }
    if (!tokens[6].empty()) {
        hasFix_ = (std::atoi(tokens[6].c_str()) > 0);
    }
    if (!tokens[7].empty()) {
        satellites_ = std::atoi(tokens[7].c_str());
    }
}

// =====================================================
// Conversão ddmm.mmmm / dddmm.mmmm -> graus decimais
// =====================================================
double NmeaParser::toDecimalDegrees(const std::string &coord, const std::string &hemisphere) {
    if (coord.empty()) return 0.0;

    // Graus: 2 dígitos (lat, parte inteira com 4) ou 3 (lon, parte inteira com 5)
    size_t dot = coord.find('.');
    size_t intPartLen = (dot == std::string::npos) ? coord.size() : dot;
    size_t degLen = (intPartLen > 4) ? 3 : 2;
    if (coord.size() <= degLen) return 0.0;

    double degrees = std::atof(coord.substr(0, degLen).c_str());
    double minutes = std::atof(coord.substr(degLen).c_str());
    double decimal = degrees + (minutes / 60.0);

    if (!hemisphere.empty()) {
        char h = static_cast<char>(std::toupper(static_cast<unsigned char>(hemisphere[0])));
        if (h == 'S' || h == 'W') decimal = -decimal;
    }
    return decimal;
}

// =====================================================
// Checksum: XOR de todos os bytes entre '$' e '*'
// =====================================================
bool NmeaParser::validateChecksum(const std::string &line) {
    if (line.empty()) return false;

    size_t dollar = line.find('$');
    size_t star = line.find('*');
    if (dollar == std::string::npos || star == std::string::npos) {
        return false;   // NEO-6M sempre envia checksum: linha sem '*' é descartada
    }
    if (star < dollar || star + 2 >= line.size()) return false;

    int hi = hexValue(line[star + 1]);
    int lo = hexValue(line[star + 2]);
    if (hi < 0 || lo < 0) return false;

    uint8_t cs = 0;
    for (size_t i = dollar + 1; i < star; ++i) {
        cs ^= static_cast<uint8_t>(line[i]);
    }
    return cs == ((hi << 4) | lo);
}
