// storage_manager.h - история измерений: кольцевой буфер в ОЗУ + SD (CSV) или LittleFS
#pragma once
#include <Arduino.h>
#include "config.h"

// Компактная запись истории (12 байт)
struct Record {
  uint32_t ts;          // Unix time, UTC
  int32_t  pressurePa;  // давление, Па
  int16_t  tempC100;    // температура, 0.01 C
  uint16_t humRH100;    // влажность, 0.01 %RH; HUM_INVALID если нет
};
static const uint16_t HUM_INVALID = 0xFFFF;

inline float recPressureMmHg(const Record& r) { return r.pressurePa * 0.00750061683f; }
inline float recTempC(const Record& r)        { return r.tempC100 / 100.0f; }
inline bool  recHumValid(const Record& r)     { return r.humRH100 != HUM_INVALID; }
inline float recHum(const Record& r)          { return r.humRH100 / 100.0f; }

namespace StorageManager {
  void   begin(uint32_t recordIntervalS);        // LittleFS + SD + загрузка истории
  void   update();                               // flush, проверка SD, повторное подключение SD
  bool   add(const Record& r);                   // false: дубликат / время пошло назад
  void   setRecordInterval(uint32_t seconds);    // пересчёт ёмкости буфера

  size_t count();                                // записей в ОЗУ
  size_t capacity();
  bool   get(size_t index, Record& out);         // 0 = самая старая
  uint32_t lastTimestamp();

  // Период, за который есть измерения (первая и последняя метка времени). false - данных нет.
  bool     range(uint32_t& firstTs, uint32_t& lastTs);

  // Обход записей с from <= ts <= to по возрастанию времени. Источник - полный CSV на SD (если карта есть),
  // иначе буфер в ОЗУ. cb возвращает false, чтобы остановить обход. Если записей на SD слишком много для
  // полного чтения, читается выборка около `points` точек (sampled = true).
  typedef bool (*RangeCb)(const Record& r, void* ctx);
  void     forRange(uint32_t from, uint32_t to, size_t points, RangeCb cb, void* ctx,
                    bool& sampled, bool& fromSd);

  bool        sdOnline();
  const char* backend();                         // "sd" или "flash"
  uint64_t    sdCardBytes();
  uint32_t    csvBytes();
  bool        flashOk();
  size_t      flashRecords();
}
