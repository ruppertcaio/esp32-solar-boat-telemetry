// Testes no host das partes puras do firmware (NmeaParser, Average, CRC-8, CSV).
// Retorna != 0 se algum CHECK falhar.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Average.h"
#include "Crc8.h"
#include "CsvLog.h"
#include "NmeaParser.h"

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static bool approx(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}

// =====================================================
// Sentenças NMEA de teste
// Checksum = XOR dos bytes entre '$' e '*'. Pares iguais se cancelam,
// então basta XOR dos caracteres que aparecem um número ímpar de vezes.
// =====================================================

// GGA clássica da documentação NMEA 0183. Ímpares: G P A 2 3 5 6 7 8 9 . N E
// 0x47^0x50^0x41^0x32^0x33^0x35^0x36^0x37^0x38^0x39^0x2E^0x4E^0x45 = 0x47
static const char *kGpGga =
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

// RMC clássica. Ímpares: G P R M C A N E W , . 0 4 5 7 8
// 0x47^0x50^0x52^0x4D^0x43^0x41^0x4E^0x45^0x57^0x2C^0x2E^0x30^0x34^0x35^0x37^0x38 = 0x6A
static const char *kGpRmc =
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

// XOR é linear: trocar o caractere a por b muda o checksum em a^b.
//   talker GP->GN: 'P'^'N' = 0x50^0x4E = 0x1E
//   lat N->S:      'N'^'S' = 0x4E^0x53 = 0x1D
//   lon E->W:      'E'^'W' = 0x45^0x57 = 0x12
// RMC: 0x6A^0x1E^0x1D^0x12 = 0x7B
static const char *kGnRmcSouthWest =
    "$GNRMC,123519,A,4807.038,S,01131.000,W,022.4,084.4,230394,003.1,W*7B";
// GGA: 0x47^0x1E = 0x59
static const char *kGnGga =
    "$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*59";

// 4807.038 = 48° + 7.038'/60 ; 01131.000 = 11° + 31'/60
static const double kLat = 48.0 + 7.038 / 60.0;   // 48.1173
static const double kLng = 11.0 + 31.0 / 60.0;    // 11.516666...

static void testNmeaChecksum() {
    CHECK(NmeaParser::validateChecksum(kGpGga));
    CHECK(NmeaParser::validateChecksum(kGpRmc));
    CHECK(NmeaParser::validateChecksum(kGnRmcSouthWest));
    CHECK(NmeaParser::validateChecksum(kGnGga));

    // Checksum errado, payload alterado e hex inválido
    CHECK(!NmeaParser::validateChecksum(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6B"));
    CHECK(!NmeaParser::validateChecksum(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,023.4,084.4,230394,003.1,W*6A"));
    CHECK(!NmeaParser::validateChecksum("$GPGGA,123519*G7"));
    CHECK(!NmeaParser::validateChecksum("$GPGGA,123519"));    // sem checksum
    CHECK(!NmeaParser::validateChecksum("$GPGGA,123519*4"));   // só 1 dígito
}

static void testNmeaRmc() {
    NmeaParser p;
    CHECK(p.parseLine(kGpRmc));
    CHECK(p.hasFix());
    CHECK(approx(p.latitude(), kLat));
    CHECK(approx(p.longitude(), kLng));
    CHECK(approx(p.speedKmh(), 22.4 * 1.852));   // nós -> km/h = 41.4848
    CHECK(approx(p.speedKmh(), 41.4848));

    // Talker $GN aceito; hemisférios S/W negativos
    NmeaParser q;
    CHECK(q.parseLine(kGnRmcSouthWest));
    CHECK(approx(q.latitude(), -kLat));
    CHECK(approx(q.longitude(), -kLng));
}

static void testNmeaGga() {
    NmeaParser p;
    CHECK(p.parseLine(kGpGga));
    CHECK(p.satellites() == 8);
    CHECK(p.hasFix());
    CHECK(approx(p.latitude(), kLat));
    CHECK(approx(p.longitude(), kLng));

    NmeaParser q;
    CHECK(q.parseLine(kGnGga));
    CHECK(q.satellites() == 8);
}

static void testNmeaRejects() {
    NmeaParser p;
    // Checksum inválido: retorna false e não altera o estado
    CHECK(!p.parseLine(
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48"));
    CHECK(p.satellites() == 0);
    CHECK(p.latitude() == 0.0);

    // Outros tipos de sentença são ignorados sem erro
    CHECK(p.parseLine("$GPGSV,1,1,00*79"));
    CHECK(p.satellites() == 0);
}

static void testCoordinateConversion() {
    CHECK(approx(NmeaParser::toDecimalDegrees("4807.038", "N"), 48.1173));
    CHECK(approx(NmeaParser::toDecimalDegrees("2330.5000", "S"), -(23.0 + 30.5 / 60.0)));
    CHECK(approx(NmeaParser::toDecimalDegrees("04638.1234", "W"), -46.63539));
    CHECK(approx(NmeaParser::toDecimalDegrees("01131.000", "E"), kLng));
    CHECK(NmeaParser::toDecimalDegrees("", "N") == 0.0);
}

static void testAverage() {
    Average<float, 4> avg;
    CHECK(avg.value() == 0.0f);         // vazio

    avg.addSample(1);
    avg.addSample(2);
    CHECK(approx(avg.value(), 1.5));    // janela parcial: média só das amostras existentes

    avg.addSample(3);
    avg.addSample(4);
    CHECK(approx(avg.value(), 2.5));    // janela cheia {1,2,3,4}

    avg.addSample(5);
    CHECK(approx(avg.value(), 3.5));    // descartou 1 -> {2,3,4,5}

    for (int i = 6; i <= 13; i++) avg.addSample(static_cast<float>(i));
    CHECK(approx(avg.value(), 11.5));   // após mais duas voltas -> {10,11,12,13}
}

static void testCrc8Maxim() {
    // Valor de verificação padrão do catálogo CRC-8/MAXIM-DOW: "123456789" -> 0xA1
    const uint8_t *check = reinterpret_cast<const uint8_t *>("123456789");
    CHECK(crc8Maxim(check, 9) == 0xA1);

    // Scratchpad do DS18B20 após power-on (85 °C = 0x0550): byte 8 é o CRC 0x1C
    // (conferido bit a bit com o algoritmo refletido 0x8C)
    uint8_t sp[9] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0x1C};
    CHECK(crc8Maxim(sp, 8) == sp[8]);
    CHECK(crc8Maxim(sp, 9) == 0x00);    // CRC sobre dados + CRC = 0

    sp[0] ^= 0x01;                       // um bit corrompido
    CHECK(crc8Maxim(sp, 8) != sp[8]);
}

// =====================================================
// Log CSV
// Valores escolhidos longe de empates de arredondamento:
//   12.6f = 12.6000003... -> "12.600"   5.05f = 5.0500001... -> "5.050"
//   3.2f  = 3.2000000...  -> "3.200"    25.5f é exato      -> "25.50"
//   12.34 -> "12.3" (%.1f); lat/lng já têm 6 casas
// =====================================================
static void testCsvLine() {
    CHECK(std::strcmp(csvHeader(),
        "uptime_ms,vbat_v,vaux_v,current_a,temp_c,lat,lng,speed_kmh,sats\n") == 0);

    SensorData s;
    s.voltageBat = 12.6f;
    s.voltageAux = 5.05f;
    s.current = 3.2f;
    s.temperature = 25.5f;
    GPSInfo g;
    g.lat = -23.55052;
    g.lng = -46.633308;
    g.speed = 12.34;
    g.sats = 7;
    g.fix = true;

    const char *expected = "12345,12.600,5.050,3.200,25.50,-23.550520,-46.633308,12.3,7\n";
    const int expectedLen = static_cast<int>(std::strlen(expected));

    char buf[160];
    CHECK(formatCsvLine(buf, sizeof(buf), 12345, s, g) == expectedLen);
    CHECK(std::strcmp(buf, expected) == 0);

    // Truncamento: precisa de strlen + 1 (terminador)
    CHECK(formatCsvLine(buf, 10, 12345, s, g) == -1);
    CHECK(formatCsvLine(buf, expectedLen, 12345, s, g) == -1);
    CHECK(formatCsvLine(buf, expectedLen + 1, 12345, s, g) == expectedLen);

    // NAN -> campo vazio (demais valores no padrão 0)
    SensorData n;
    n.voltageBat = NAN;
    n.temperature = NAN;
    CHECK(formatCsvLine(buf, sizeof(buf), 0, n, GPSInfo{}) > 0);
    CHECK(std::strcmp(buf, "0,,0.000,0.000,,,,,0\n") == 0);   // sem fix: posição vazia

    // Sem fix: lat/lng/velocidade vazios mesmo com valores antigos no struct
    g.fix = false;
    CHECK(formatCsvLine(buf, sizeof(buf), 1, s, g) > 0);
    CHECK(std::strcmp(buf, "1,12.600,5.050,3.200,25.50,,,,7\n") == 0);
}

static void testLogFileNames() {
    char name[16];
    CHECK(nextLogFileName(name, sizeof(name), 0));
    CHECK(std::strcmp(name, "LOG0001.CSV") == 0);
    CHECK(nextLogFileName(name, sizeof(name), 41));
    CHECK(std::strcmp(name, "LOG0042.CSV") == 0);
    CHECK(!nextLogFileName(name, sizeof(name), 9999));   // não cabe em 8.3
    CHECK(!nextLogFileName(name, 11, 0));                // sem espaço para '\0'

    CHECK(parseLogIndex("LOG0042.CSV") == 42);
    CHECK(parseLogIndex("log0007.csv") == 7);            // aceita minúsculas
    CHECK(parseLogIndex("DATA.TXT") == -1);
    CHECK(parseLogIndex("LOG042.CSV") == -1);            // 3 dígitos
    CHECK(parseLogIndex("LOG00A2.CSV") == -1);
    CHECK(parseLogIndex("LOG0042.TXT") == -1);
}

int main() {
    testNmeaChecksum();
    testNmeaRmc();
    testNmeaGga();
    testNmeaRejects();
    testCoordinateConversion();
    testAverage();
    testCrc8Maxim();
    testCsvLine();
    testLogFileNames();

    if (failures == 0) {
        std::printf("Todos os testes passaram.\n");
        return 0;
    }
    std::printf("%d falha(s).\n", failures);
    return 1;
}
