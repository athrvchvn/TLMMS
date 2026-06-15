#include "session_manager.h"
#include "config.h"
#include <Arduino.h>
#include <time.h>
#include <string.h>

static bool    _active     = false;
static uint32_t _startMs   = 0;
static uint32_t _startTs   = 0;  // Unix timestamp
static char    _roll[10]   = {};
static uint8_t _machineId  = 0;

static SessionLog _lastLog;

void Session::begin() {
    _active = false;
    memset(_roll, 0, sizeof(_roll));
}

void Session::start(const char* roll, uint8_t machineId) {
    _active    = true;
    _startMs   = millis();
    _startTs   = (uint32_t)time(nullptr);
    _machineId = machineId;
    strncpy(_roll, roll, 9);
    _roll[9] = '\0';
}

void Session::end(const char* reason) {
    _lastLog.startTs     = _startTs;
    _lastLog.durationSec = elapsedSeconds();
    strncpy(_lastLog.rollNumber, _roll, 9);
    _lastLog.rollNumber[9] = '\0';
    _lastLog.machineId  = _machineId;
    strncpy(_lastLog.endReason, reason, sizeof(_lastLog.endReason) - 1);
    _lastLog.endReason[sizeof(_lastLog.endReason) - 1] = '\0';

    _active = false;
    memset(_roll, 0, sizeof(_roll));
}

bool Session::isActive() {
    return _active;
}

uint32_t Session::elapsedSeconds() {
    if (!_active) return 0;
    return (millis() - _startMs) / 1000;
}

uint32_t Session::remainingSeconds(uint16_t limitMinutes) {
    uint32_t limitSec = (uint32_t)limitMinutes * 60;
    uint32_t elapsed  = elapsedSeconds();
    return (elapsed >= limitSec) ? 0 : (limitSec - elapsed);
}

bool Session::isWarning(uint16_t limitMinutes) {
    if (!_active) return false;
    return remainingSeconds(limitMinutes) <= SESSION_WARNING_SEC;
}

bool Session::isExpired(uint16_t limitMinutes) {
    if (!_active) return false;
    return elapsedSeconds() >= (uint32_t)limitMinutes * 60;
}

void Session::getLog(SessionLog* out) {
    *out = _lastLog;
}

const char* Session::getRoll() {
    return _roll;
}

uint32_t Session::startTimestamp() {
    return _startTs;
}
