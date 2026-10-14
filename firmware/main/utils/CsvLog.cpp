#include "CsvLog.h"

#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>

constexpr int kMaxLogIndex = 9999;      // LOG9999.CSV: limite do nome 8.3
constexpr size_t kLogNameLen = 11;      // "LOGnnnn.CSV"

const char *csvHeader() {
    return "uptime_ms,vbat_v,vaux_v,current_a,temp_c,lat,lng,speed_kmh,sats\n";
}

// =====================================================
// Acrescenta ",<valor>" (ou só "," se não finito) em buf[pos].
// Retorna false se não couber.
// =====================================================
static bool appendField(char *buf, size_t len, size_t &pos, double value, int decimals) {
    int n = std::isfinite(value)
                ? std::snprintf(buf + pos, len - pos, ",%.*f", decimals, value)
                : std::snprintf(buf + pos, len - pos, ",");
    if (n < 0 || static_cast<size_t>(n) >= len - pos) return false;
    pos += static_cast<size_t>(n);
    return true;
}

int formatCsvLine(char *buf, size_t len, uint32_t uptimeMs,
                  const SensorData &sensors, const GPSInfo &gps) {
    if (buf == nullptr || len == 0) return -1;

    int n = std::snprintf(buf, len, "%" PRIu32, uptimeMs);
    if (n < 0 || static_cast<size_t>(n) >= len) return -1;
    size_t pos = static_cast<size_t>(n);

    bool ok = appendField(buf, len, pos, sensors.voltageBat, 3) &&
              appendField(buf, len, pos, sensors.voltageAux, 3) &&
              appendField(buf, len, pos, sensors.current, 3) &&
              appendField(buf, len, pos, sensors.temperature, 2) &&
              appendField(buf, len, pos, gps.fix ? gps.lat : NAN, 6) &&
              appendField(buf, len, pos, gps.fix ? gps.lng : NAN, 6) &&
              appendField(buf, len, pos, gps.fix ? gps.speed : NAN, 1);
    if (!ok) return -1;

    n = std::snprintf(buf + pos, len - pos, ",%d\n", gps.sats);
    if (n < 0 || static_cast<size_t>(n) >= len - pos) return -1;
    return static_cast<int>(pos + static_cast<size_t>(n));
}

bool nextLogFileName(char *out, size_t len, int lastIndex) {
    if (out == nullptr || lastIndex < 0 || lastIndex >= kMaxLogIndex) return false;
    int n = std::snprintf(out, len, "LOG%04d.CSV", lastIndex + 1);
    return n > 0 && static_cast<size_t>(n) < len;
}

// Compara n caracteres ignorando maiúsculas/minúsculas (upper já em maiúsculas)
static bool matchesNoCase(const char *s, const char *upper, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (std::toupper(static_cast<unsigned char>(s[i])) != upper[i]) return false;
    }
    return true;
}

int parseLogIndex(const char *name) {
    if (name == nullptr || std::strlen(name) != kLogNameLen) return -1;
    if (!matchesNoCase(name, "LOG", 3) || !matchesNoCase(name + 7, ".CSV", 4)) return -1;

    int index = 0;
    for (size_t i = 3; i < 7; i++) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return -1;
        index = index * 10 + (name[i] - '0');
    }
    return index;
}
