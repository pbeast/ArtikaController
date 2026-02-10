#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <Arduino.h>

#define LOG_BUFFER_LINES 250
#define LOG_MAX_LINE_LENGTH 200

class LogBuffer : public Print {
public:
  LogBuffer() : _head(0), _count(0), _nextId(1), _pendingCR(false) {}

  // Print interface
  size_t write(uint8_t c) override {
    Serial.write(c);

    if (c == '\n') {
      // \n or \r\n — push the accumulated line
      pushLine(_currentLine);
      _currentLine = "";
      _pendingCR = false;
      return 1;
    }

    if (c == '\r') {
      // Defer action: might be \r\n (println) or standalone \r (OTA overwrite)
      _pendingCR = true;
      return 1;
    }

    // Regular character — if a bare \r preceded this, it was an OTA-style
    // overwrite, so clear the line before appending
    if (_pendingCR) {
      _currentLine = "";
      _pendingCR = false;
    }

    if (_currentLine.length() < LOG_MAX_LINE_LENGTH) {
      _currentLine += (char)c;
    }

    return 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    for (size_t i = 0; i < size; i++) {
      write(buffer[i]);
    }
    return size;
  }

  // Returns JSON: {"n":<nextId>,"l":["line1","line2",...]}
  String getLogsSince(unsigned long sinceId) {
    String json = "{\"n\":";
    json += String(_nextId);
    json += ",\"l\":[";

    bool first = true;

    if (_count > 0) {
      // Calculate the oldest available ID
      unsigned long oldestId = _nextId - _count;

      // If sinceId is before our oldest, start from oldest
      if (sinceId < oldestId) {
        sinceId = oldestId;
      }

      for (unsigned long id = sinceId; id < _nextId; id++) {
        int idx = idToIndex(id);
        if (!first) json += ",";
        first = false;
        json += "\"";
        jsonEscape(json, _lines[idx]);
        json += "\"";
      }
    }

    json += "]}";
    return json;
  }

private:
  String _lines[LOG_BUFFER_LINES];
  unsigned long _ids[LOG_BUFFER_LINES];
  int _head;           // Next write position
  int _count;          // Number of lines stored
  unsigned long _nextId; // Monotonically increasing sequence ID
  String _currentLine;
  bool _pendingCR;

  void pushLine(const String& line) {
    _lines[_head] = line;
    _ids[_head] = _nextId++;
    _head = (_head + 1) % LOG_BUFFER_LINES;
    if (_count < LOG_BUFFER_LINES) _count++;
  }

  int idToIndex(unsigned long id) {
    // The oldest entry is at (_head - _count + LOG_BUFFER_LINES) % LOG_BUFFER_LINES
    unsigned long oldestId = _nextId - _count;
    int offset = (int)(id - oldestId);
    return (_head - _count + offset + LOG_BUFFER_LINES) % LOG_BUFFER_LINES;
  }

  void jsonEscape(String& out, const String& s) {
    for (unsigned int i = 0; i < s.length(); i++) {
      char c = s[i];
      if (c == '"') {
        out += "\\\"";
      } else if (c == '\\') {
        out += "\\\\";
      } else if (c < 0x20) {
        // Control characters - use unicode escape
        out += "\\u00";
        out += "0123456789abcdef"[(c >> 4) & 0xf];
        out += "0123456789abcdef"[c & 0xf];
      } else {
        out += c;
      }
    }
  }
};

extern LogBuffer Log;

#endif
