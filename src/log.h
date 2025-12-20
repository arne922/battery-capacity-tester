#pragma once
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>


// -----------------------------------------------------------------------------
// Minimal BT_LOG* macro shim: identical macro names, Serial output.
// Do NOT include <esp_log.h> in files that include this header.
// -----------------------------------------------------------------------------

// Choose compile-time level (override via build_flags: -DBT_LOG_LEVEL=5)
#ifndef BT_LOG_LEVEL
#define BT_LOG_LEVEL 5   // 1=E,2=W,3=I,4=D,5=V
#endif

static inline void _esp_serial_log(char level, const char* tag, const char* fmt, ...) {
  char msg[256];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  Serial.print('[');
  Serial.print(millis());
  Serial.print("][");
  Serial.print(level);
  Serial.print("][");
  Serial.print(tag ? tag : "?");
  Serial.print("] ");
  Serial.println(msg);
}

// identical macro names:
#if BT_LOG_LEVEL >= 1
  #define BT_LOGE(tag, fmt, ...) _esp_serial_log('E', tag, fmt, ##__VA_ARGS__)
#else
  #define BT_LOGE(tag, fmt, ...)
#endif

#if BT_LOG_LEVEL >= 2
  #define BT_LOGW(tag, fmt, ...) _esp_serial_log('W', tag, fmt, ##__VA_ARGS__)
#else
  #define BT_LOGW(tag, fmt, ...)
#endif

#if BT_LOG_LEVEL >= 3
  #define BT_LOGI(tag, fmt, ...) _esp_serial_log('I', tag, fmt, ##__VA_ARGS__)
#else
  #define BT_LOGI(tag, fmt, ...)
#endif

#if BT_LOG_LEVEL >= 4
  #define BT_LOGD(tag, fmt, ...) _esp_serial_log('D', tag, fmt, ##__VA_ARGS__)
#else
  #define BT_LOGD(tag, fmt, ...)
#endif

#if BT_LOG_LEVEL >= 5
  #define BT_LOGV(tag, fmt, ...) _esp_serial_log('V', tag, fmt, ##__VA_ARGS__)
#else
  #define BT_LOGV(tag, fmt, ...)
#endif
