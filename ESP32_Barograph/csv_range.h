// csv_range.h - выборка записей CSV по времени без загрузки файла в ОЗУ.
// Строки в файле идут по возрастанию timestamp, поэтому начало нужного периода
// находится бинарным поиском по смещению в файле (~15 обращений к карте вместо чтения всего файла).
// Код шаблонный: "файл" - любой тип с seek(size_t), read(uint8_t*, size_t), что позволяет проверять его на ПК.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace CsvRange {

// Построчное чтение с буфером 512 байт (чтение по одному байту с SD слишком медленное).
template <class F>
class Reader {
 public:
  explicit Reader(F& f) : f_(f) {}

  void seek(size_t pos) { f_.seek(pos); pos_ = pos; off_ = len_ = 0; }
  size_t position() const { return pos_; }          // смещение следующего байта

  // Читает строку без \r\n (лишнее сверх n-1 отбрасывается). false - конец файла.
  bool line(char* out, size_t n) {
    size_t i = 0;
    bool any = false;
    int c;
    while ((c = next()) >= 0) {
      any = true;
      if (c == '\n') break;
      if (c != '\r' && i + 1 < n) out[i++] = (char)c;
    }
    out[i] = 0;
    return any;
  }

 private:
  int next() {
    if (off_ >= len_) {
      int r = f_.read(buf_, sizeof buf_);
      len_ = r > 0 ? (size_t)r : 0;
      off_ = 0;
      if (len_ == 0) return -1;
    }
    pos_++;
    return buf_[off_++];
  }

  F& f_;
  uint8_t buf_[512];
  size_t pos_ = 0, off_ = 0, len_ = 0;
};

// Первая корректная строка, начинающаяся в позиции >= x (x = 0 - с начала файла; заголовок пропускается).
// false - достигнут конец файла.
template <class F, class R, class P>
bool lineAtOrAfter(Reader<F>& rd, size_t x, P parse, R& rec) {
  char buf[100];
  if (x == 0) rd.seek(0);
  else { rd.seek(x - 1); if (!rd.line(buf, sizeof buf)) return false; }   // дочитываем начатую строку
  for (int tries = 0; tries < 4; tries++) {                              // заголовок/повреждённая строка пропускаются
    if (!rd.line(buf, sizeof buf)) return false;
    if (parse(buf, rec)) return true;
  }
  return false;
}

// Смещение, с которого можно безопасно начинать последовательное чтение, чтобы не пропустить ни одной
// записи с ts >= target (все строки до первой строки >= target лежат не раньше этого смещения - 1 строки).
template <class F, class R, class P>
size_t findStart(Reader<F>& rd, size_t size, uint32_t target, P parse) {
  size_t lo = 0, hi = size;
  R r;
  while (hi - lo > 1024) {
    size_t mid = lo + (hi - lo) / 2;
    if (lineAtOrAfter<F, R, P>(rd, mid, parse, r) && r.ts < target) lo = mid;
    else hi = mid;
  }
  return lo;
}

// Последовательный проход по записям с from <= ts <= to. cb(const R&) -> false: остановить.
// Возвращает число переданных записей.
template <class F, class R, class P, class CB>
size_t scan(Reader<F>& rd, size_t startOff, uint32_t from, uint32_t to, P parse, CB cb) {
  char buf[100];
  if (startOff == 0) rd.seek(0);
  else { rd.seek(startOff - 1); if (!rd.line(buf, sizeof buf)) return 0; }
  R r;
  size_t n = 0;
  while (rd.line(buf, sizeof buf)) {
    if (!parse(buf, r)) continue;
    if (r.ts > to) break;
    if (r.ts < from) continue;
    n++;
    if (!cb(r)) break;
  }
  return n;
}

// Выборочное чтение для очень больших периодов: points точек, равномерно распределённых по смещению
// в файле; в каждой читается до perPoint подряд идущих записей. Время каждой записи остаётся точным,
// пропускаются только промежуточные записи.
template <class F, class R, class P, class CB>
size_t sample(Reader<F>& rd, size_t offFrom, size_t offTo, uint32_t from, uint32_t to,
              size_t points, size_t perPoint, P parse, CB cb) {
  char buf[100];
  R r;
  size_t n = 0;
  uint32_t lastTs = 0;
  if (points == 0 || offTo <= offFrom) return 0;
  for (size_t i = 0; i < points; i++) {
    size_t off = offFrom + (size_t)((uint64_t)(offTo - offFrom) * i / points);
    if (off == 0) rd.seek(0);
    else { rd.seek(off - 1); if (!rd.line(buf, sizeof buf)) break; }
    size_t got = 0;
    while (got < perPoint && rd.line(buf, sizeof buf)) {
      if (!parse(buf, r)) continue;
      if (r.ts > to) return n;
      if (r.ts < from || r.ts <= lastTs) continue;
      lastTs = r.ts;
      got++; n++;
      if (!cb(r)) return n;
    }
  }
  return n;
}

}  // namespace CsvRange
