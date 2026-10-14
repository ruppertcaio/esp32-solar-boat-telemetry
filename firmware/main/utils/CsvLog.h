#ifndef CSV_LOG_H
#define CSV_LOG_H

#include <cstddef>
#include <cstdint>

#include "TelemetryData.h"

// --------------------------------------------------------------
// Formatação do log CSV gravado no cartão SD.
// Funções puras (sem includes do ESP-IDF) para permitir teste no host.
// --------------------------------------------------------------

// Cabeçalho do CSV, já com '\n'
const char *csvHeader();

// Formata uma linha (com '\n') em buf. Casas decimais fixas:
// tensão/corrente 3, temperatura 2, lat/lng 6, velocidade 1.
// Valor não finito (NAN/inf) vira campo vazio.
// Retorna o comprimento escrito ou -1 se não couber em buf.
int formatCsvLine(char *buf, size_t len, uint32_t uptimeMs,
                  const SensorData &sensors, const GPSInfo &gps);

// Nome 8.3 do próximo arquivo: lastIndex 0 -> "LOG0001.CSV".
// (FATFS do ESP-IDF vem sem nomes longos por padrão.)
// Retorna false se lastIndex estiver fora de 0..9998 ou out for pequeno.
bool nextLogFileName(char *out, size_t len, int lastIndex);

// Índice de um nome "LOGnnnn.CSV" (maiúsculas ou minúsculas; o FATFS
// devolve nomes curtos em maiúsculas). Retorna -1 se não for desse formato.
int parseLogIndex(const char *name);

#endif
