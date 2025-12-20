#include "log_buffer.h"
#include <Arduino.h> // for Print

// ---- Little-endian helpers -----------------------------------------------
// We store all multi-byte values in little-endian format to keep the layout
// deterministic and portable across compilers.

// Write 16-bit unsigned integer (little-endian)
static void writeU16LE(uint8_t* p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
}

// Write 32-bit unsigned integer (little-endian)
static void writeU32LE(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
  p[2] = (uint8_t)((v >> 16) & 0xFF);
  p[3] = (uint8_t)((v >> 24) & 0xFF);
}

// Read 16-bit unsigned integer (little-endian)
static uint16_t readU16LE(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

// Read 32-bit unsigned integer (little-endian)
static uint32_t readU32LE(const uint8_t* p) {
  return (uint32_t)p[0]
       | ((uint32_t)p[1] << 8)
       | ((uint32_t)p[2] << 16)
       | ((uint32_t)p[3] << 24);
}

// Write float as IEEE-754 32-bit (little-endian)
static void writeF32LE(uint8_t* p, float f) {
  union { float f; uint32_t u; } x;
  x.f = f;
  writeU32LE(p, x.u);
}

// Read float as IEEE-754 32-bit (little-endian)
static float readF32LE(const uint8_t* p) {
  union { float f; uint32_t u; } x;
  x.u = readU32LE(p);
  return x.f;
}

// --------------------------------------------------------------------------

LogBuffer::LogBuffer(uint8_t* storage,
                     size_t storageBytes,
                     const ColDef* schema,
                     size_t schemaCols)
  : buf_(storage),
    bufBytes_(storageBytes),
    schema_(schema),
    cols_(schemaCols) {

  // Compute packed row size from schema definition
  rowBytes_ = 0;
  for (size_t i = 0; i < cols_; ++i) {
    rowBytes_ += colSize_(schema_[i].type);
  }

  // Calculate how many rows fit into the provided RAM block
  capRows_ = (rowBytes_ > 0) ? (bufBytes_ / rowBytes_) : 0;

  clear();
}

void LogBuffer::clear() {
  // Reset ring buffer pointers
  head_ = 0;
  size_ = 0;
}

size_t LogBuffer::colSize_(ColType t) const {
  // Return storage size per column type
  switch (t) {
    case ColType::U8:  return 1;
    case ColType::U16: return 2;
    case ColType::U32: return 4;
    case ColType::F32: return 4;
  }
  return 0;
}

bool LogBuffer::store(const ColValue* values, size_t valuesCount) {
  // Validate buffer and schema
  if (!buf_ || !schema_ || cols_ == 0 || rowBytes_ == 0 || capRows_ == 0) {
    return false;
  }

  // Schema and provided values must match
  if (valuesCount != cols_) {
    return false;
  }

  // Compute destination row pointer
  uint8_t* row = buf_ + (head_ * rowBytes_);

  // Encode typed values into packed row
  encodeRow_(row, values);

  // Advance ring buffer write pointer
  head_ = (head_ + 1) % capRows_;

  // Increase valid row count until buffer is full
  if (size_ < capRows_) {
    size_++;
  }
  return true;
}

size_t LogBuffer::oldestRow_() const {
  // Oldest row is head - size (modulo capacity)
  return (head_ + capRows_ - size_) % capRows_;
}

void LogBuffer::encodeRow_(uint8_t* dst, const ColValue* values) const {
  // Pack all column values sequentially into the row buffer
  size_t off = 0;

  for (size_t i = 0; i < cols_; ++i) {
    const ColType t = schema_[i].type;

    switch (t) {
      case ColType::U8:
        dst[off] = values[i].u8;
        off += 1;
        break;

      case ColType::U16:
        writeU16LE(dst + off, values[i].u16);
        off += 2;
        break;

      case ColType::U32:
        writeU32LE(dst + off, values[i].u32);
        off += 4;
        break;

      case ColType::F32:
        writeF32LE(dst + off, values[i].f32);
        off += 4;
        break;
    }
  }
}

void LogBuffer::printCsv(Print& out) const {
  // Print CSV header using schema names
  for (size_t i = 0; i < cols_; ++i) {
    out.print(schema_[i].name);
    out.print((i + 1 < cols_) ? ',' : '\n');
  }

  // Print stored rows from oldest to newest
  if (empty()) return;

  const size_t oldest = oldestRow_();
  for (size_t k = 0; k < size_; ++k) {
    const size_t rowIndex = (oldest + k) % capRows_;
    const uint8_t* row = buf_ + (rowIndex * rowBytes_);
    printRowCsv_(out, row);
  }
}

void LogBuffer::printRowCsv_(Print& out, const uint8_t* row) const {
  // Print one CSV row by decoding each column
  size_t off = 0;

  for (size_t i = 0; i < cols_; ++i) {
    const ColType t = schema_[i].type;
    printCell_(out, t, row + off);
    off += colSize_(t);
    out.print((i + 1 < cols_) ? ',' : '\n');
  }
}

void LogBuffer::printCell_(Print& out, ColType t, const uint8_t* p) const {
  // Convert one packed cell into CSV text
  switch (t) {
    case ColType::U8:
      out.print((uint32_t)p[0]);
      break;

    case ColType::U16:
      out.print((uint32_t)readU16LE(p));
      break;

    case ColType::U32:
      out.print((uint32_t)readU32LE(p));
      break;

    case ColType::F32:
      out.print(readF32LE(p), 4);
      break;
  }
}
