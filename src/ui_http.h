#pragma once
#include <stdint.h>
#include <WString.h>

class WebServer;
class StateMachine;
class Hw;
class LogBuffer;
class Core;

// Simple HTTP adapter: serves UI, accepts commands/config, exposes telemetry, provides download.
class UiHttp {
public:
  UiHttp(WebServer& server, StateMachine& sm, Core& core, Hw& hw, LogBuffer& log);

  // Call once from setup()
  void begin();

  // Call regularly from loop()
  void tick();

private:
  WebServer& server_;
  StateMachine& sm_;
  Core& core_;
  Hw& hw_;
  LogBuffer& log_;

  void setupRoutes();

  // Route handlers
  void handleRoot();
  void handleStatus();
  void handleControl();
  void handleConfig();
  void handleDownload();
  void handleGetConfig();


  // Helpers
  static bool readJsonBody(WebServer& s, String& out);

  // Tiny JSON-ish extractors (no ArduinoJson)
  static bool extractNumber(const String& body, const char* key, long& out);
  static bool extractNumber(const String& body, const char* key, float& out);
};



// #pragma once
// #include <stdint.h>
// #include <WString.h>   

// class WebServer;
// class StateMachine;
// class Hw;
// class LogBuffer;

// // Simple HTTP adapter: serves UI, accepts commands/config, exposes telemetry, provides download.
// class UiHttp {
// public:
//   UiHttp(WebServer& server, StateMachine& sm, Hw& hw, LogBuffer& log);

//   // Call once from setup()
//   void begin();

//   // Call regularly from loop()
//   void tick();

// private:
//   WebServer& server_;
//   StateMachine& sm_;
//   Hw& hw_;
//   LogBuffer& log_;

//   void setupRoutes();

//   // Route handlers
//   void handleRoot();
//   void handleStatus();
//   void handleControl();
//   void handleConfig();
//   void handleDownload();

//   // Helpers
//   static bool readJsonBody(WebServer& s, String& out);
// };
