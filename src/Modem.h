/*
  This file is part of the MKR NB library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _MODEM_INCLUDED_H
#define _MODEM_INCLUDED_H

#include <stdarg.h>
#include <stdio.h>

#include <Arduino.h>



#ifdef ARDUINO_ARCH_AVR                    // Arduino AVR boards (Uno, Pro Micro, etc.)
#define SOFTWARE_SERIAL_ENABLED // Enable software serial
#endif

#ifdef ARDUINO_ARCH_ESP8266
#define SOFTWARE_SERIAL_ENABLED // Enable software serial
#endif

#ifdef ARDUINO_ARCH_SAMD                    // Arduino SAMD boards (SAMD21, etc.)
#define SOFTWARE_SERIAL_ENABLEDx // Disable software serial
#endif

#ifdef ARDUINO_ARCH_APOLLO3                // Arduino Apollo boards (Artemis module, RedBoard Artemis, etc)
#define SOFTWARE_SERIAL_ENABLED // Enable software serial
#endif

#ifdef ARDUINO_ARCH_STM32                  // STM32 based boards (Disco, Nucleo, etc)
#define SOFTWARE_SERIAL_ENABLED // Enable software serial
#endif

#ifdef SOFTWARE_SERIAL_ENABLED
#include <SoftwareSerial.h>
#endif



class ModemUrcHandler {
public:
    virtual void handleUrc(const String &urc) = 0;
};

struct SerialState {
    unsigned long baud;
    bool isBegin;
};

class SerialStateUpdateHandler {
public:
    virtual void updateState(SerialState desiredState) = 0;
    virtual ~SerialStateUpdateHandler() = default;
};

class Modem {
public:
    Modem(Stream &uart, unsigned long baud, int resetPin, int powerOnPin, SerialStateUpdateHandler *handler);

    Modem(HardwareSerial &uart, unsigned long baud, int resetPin, int powerOnPin);

#ifdef SOFTWARE_SERIAL_ENABLED
    Modem(SoftwareSerial &uart, unsigned long baud, int resetPin, int powerOnPin);
#endif

    virtual ~Modem();

    int begin(bool restart = true);

    void end();

    void debug();

    void debug(Print &p);

    void noDebug();

    int autosense(unsigned long timeout = 10000);

    int noop();

    int reset();

    size_t write(uint8_t c);

    size_t write(const uint8_t *, size_t);

    size_t write_P(const uint8_t *raw_PGM, size_t size);

    void send(const char *command);

    void send(const String &command) { send(command.c_str()); }

    void sendf(const char *fmt, ...);

    int waitForPrompt(unsigned long timeout = 500);

    int waitForResponse(unsigned long timeout = 200, String *responseDataStorage = nullptr);

    int ready();

    void poll();

    void setResponseDataStorage(String *responseDataStorage);

    void addUrcHandler(ModemUrcHandler *handler);

    void removeUrcHandler(ModemUrcHandler *handler);

    void setBaudRate(unsigned long baud);

private:
    Stream *_uart;
    SerialStateUpdateHandler* _handler;
    unsigned long _baud;
    uint8_t _resetPin;
    uint8_t _powerOnPin;
    unsigned long _lastResponseOrUrcMillis;

    enum {
        AT_COMMAND_IDLE,
        AT_RECEIVING_RESPONSE
    } _atCommandState;
    int _ready;
    String _buffer;
    String *_responseDataStorage;

#define MAX_URC_HANDLERS 8 // 7 sockets + GPRS
    ModemUrcHandler *_urcHandlers[MAX_URC_HANDLERS] = {nullptr};
    Print *_debugPrint = nullptr;

    void powerOn(bool restart) const;
    void powerOff() const;
};

#endif
