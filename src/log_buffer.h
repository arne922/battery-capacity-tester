#pragma once
#include <stdint.h>
#include <stddef.h>
#include "config.h"

class Print;

// Generic value container for store().
// Only the field matching the column type is used.
union ColValue {
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  float    f32;
};

// Typed, schema-driven ring buffer.
// - No downsampling, no aggregation.
// - Stores packed bytes per row (u8/u16/u32/f32) to save RAM.
class LogBuffer {
public:
  LogBuffer(uint8_t* storage,
            size_t storageBytes,
            const ColDef* schema,
            size_t schemaCols);

  void clear();

  // Store one row. valuesCount must match schemaCols.
  bool store(const ColValue* values, size_t valuesCount);

  size_t size() const { return size_; }
  size_t capacity() const { return capRows_; }
  bool empty() const { return size_ == 0; }

  // Print CSV (header + rows) using schema names.
  void printCsv(Print& out) const;

private:
  uint8_t* buf_ = nullptr;
  size_t bufBytes_ = 0;

  const ColDef* schema_ = nullptr;
  size_t cols_ = 0;

  size_t rowBytes_ = 0;
  size_t capRows_ = 0;

  size_t head_ = 0; // next write row index
  size_t size_ = 0; // number of valid rows

  size_t oldestRow_() const;

  size_t colSize_(ColType t) const;
  void   encodeRow_(uint8_t* dst, const ColValue* values) const;
  void   printRowCsv_(Print& out, const uint8_t* row) const;
  void   printCell_(Print& out, ColType t, const uint8_t* p) const;
};
