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

#include "Modem.h"

#define MODEM_MIN_RESPONSE_OR_URC_WAIT_TIME_MS 20
#define LTE_RESET_PULSE_PERIOD 10000

Modem::Modem(Stream &uart, unsigned long baud, int resetPin, int powerOnPin,
             SerialStateUpdateHandler *handler) :
        _uart(&uart),
        _handler(handler),
        _baud(baud),
        _resetPin(resetPin),
        _powerOnPin(powerOnPin),
        _lastResponseOrUrcMillis(0),
        _atCommandState(AT_COMMAND_IDLE),
        _ready(1),
        _responseDataStorage(nullptr) {
    _buffer.reserve(64);
}

template<class T>
class TStateUpdateHandler : public SerialStateUpdateHandler {
public:
    TStateUpdateHandler(T &uart) : _uart(uart) {}

    void updateState(SerialState desiredState) override {
        if (desiredState.isBegin) {
            _uart.begin(desiredState.baud);
        } else {
            _uart.end();
        }
    }

private:
    T &_uart;
};

Modem::Modem(HardwareSerial &uart, unsigned long baud, int resetPin, int powerOnPin) :
        Modem((Stream &) uart, baud, resetPin, powerOnPin, new TStateUpdateHandler<HardwareSerial>(uart)) {}

#ifdef SOFTWARE_SERIAL_ENABLED
Modem::Modem(SoftwareSerial &uart, unsigned long baud, int resetPin, int powerOnPin) :
        Modem((Stream &) uart, baud, resetPin, powerOnPin, new TStateUpdateHandler<SoftwareSerial>(uart)) {}
#endif

int Modem::begin(bool restart) {
    _handler->updateState({_baud > 115200 ? 115200 : _baud, true});
    // power on module
    powerOn(restart);

    if (!restart) {
        if (!autosense()) {
            return 0;
        }

        if (!reset()) {
            return 0;
        }
    }

    if (!autosense()) {
        return 0;
    }

    if (_baud > 115200) {
        sendf("AT+IPR=%ld", _baud);
        if (waitForResponse() != 1) {
            return 0;
        }

        _handler->updateState({_baud, false});
        delay(100);
        _handler->updateState({_baud, true});

        if (!autosense()) {
            return 0;
        }
    }

    return 1;
}

void Modem::end() {
    _handler->updateState({_baud, false});
    // power off module
    powerOff();
}

void Modem::debug() {
    debug(Serial);
}

void Modem::debug(Print &p) {
    _debugPrint = &p;
}

void Modem::noDebug() {
    _debugPrint = nullptr;
}

int Modem::autosense(unsigned long timeout) {
    for (unsigned long start = millis(); (millis() - start) < timeout;) {
        if (noop() == 1) {
            return 1;
        }

        delay(100);
    }

    return 0;
}

int Modem::noop() {
    send("AT");

    return (waitForResponse() == 1);
}

int Modem::reset() {
    send("AT+CFUN=15");

    return (waitForResponse(1000) == 1);
}

size_t Modem::write(uint8_t c) {
    return _uart->write(c);
}

size_t Modem::write(const uint8_t *buf, size_t size) {
    size_t result = _uart->write(buf, size);

    // the R410m echos the binary data, when we don't what it to so
    size_t ignoreCount = 0;

    while (ignoreCount < result) {
        if (_uart->available()) {
            _uart->read();

            ignoreCount++;
        }
    }

    return result;
}

size_t Modem::write_P(const uint8_t *raw_PGM, size_t size) {
    uint8_t buff[128] __attribute__ ((aligned(4)));
    auto len = size;
    size_t n = 0;
    while (n < len) {
        int to_write = std::min((size_t) sizeof(buff), len - n);
        memcpy_P(buff, raw_PGM, to_write);
        auto written = write(buff, to_write);
        n += written;
        raw_PGM += written;
        if (!written) {
            // Some error, write() should write at least 1 byte before returning
            break;
        }
    }
    return n;
}

void Modem::send(const char *command) {
    // compare the time of the last response or URC and ensure
    // at least 20ms have passed before sending a new command
    unsigned long delta = millis() - _lastResponseOrUrcMillis;
    if (delta < MODEM_MIN_RESPONSE_OR_URC_WAIT_TIME_MS) {
        delay(MODEM_MIN_RESPONSE_OR_URC_WAIT_TIME_MS - delta);
    }

    _uart->println(command);
    _uart->flush();
    _atCommandState = AT_COMMAND_IDLE;
    _ready = 0;
}

void Modem::sendf(const char *fmt, ...) {
    char buf[BUFSIZ];

    va_list ap;
    va_start((ap), (fmt));
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    send(buf);
}

int Modem::waitForPrompt(unsigned long timeout) {
    for (unsigned long start = millis(); (millis() - start) < timeout;) {
        while (_uart->available()) {
            char c = _uart->read();
            if (_debugPrint) {
                _debugPrint->print(c);
            }

            _buffer += c;

            if (_buffer.endsWith(">")) {
                return 1;
            }
        }
    }
    return -1;
}

int Modem::waitForResponse(unsigned long timeout, String *responseDataStorage) {
    _responseDataStorage = responseDataStorage;
    for (unsigned long start = millis(); (millis() - start) < timeout;) {
        int r = ready();

        if (r != 0) {
            _responseDataStorage = nullptr;
            return r;
        }
    }

    _responseDataStorage = nullptr;
    _buffer = "";
    return -1;
}

int Modem::ready() {
    poll();

    return _ready;
}

void Modem::poll() {
    while (_uart->available()) {
        char c = _uart->read();

        if (_debugPrint) {
            _debugPrint->write(c);
        }

        _buffer += c;

        switch (_atCommandState) {
            case AT_COMMAND_IDLE:
            default: {

                if (_buffer.startsWith("AT") && _buffer.endsWith("\r\n")) {
                    _atCommandState = AT_RECEIVING_RESPONSE;
                    _buffer = "";
                } else if (_buffer.endsWith("\r\n")) {
                    _buffer.trim();

                    if (_buffer.length()) {
                        _lastResponseOrUrcMillis = millis();

                        for (auto const & _urcHandler : _urcHandlers) {
                            if (_urcHandler != nullptr) {
                                _urcHandler->handleUrc(_buffer);
                            }
                        }
                    }

                    _buffer = "";
                }

                break;
            }

            case AT_RECEIVING_RESPONSE: {
                if (c == '\n') {
                    _lastResponseOrUrcMillis = millis();
                    int responseResultIndex;

                    if ((responseResultIndex = _buffer.lastIndexOf("OK\r\n")) != -1) {
                        _ready = 1;
                    } else if ((responseResultIndex = _buffer.lastIndexOf("ERROR\r\n")) != -1) {
                        _ready = 2;
                    } else if ((responseResultIndex = _buffer.lastIndexOf("NO CARRIER\r\n")) != -1) {
                        _ready = 3;
                    } else if ((responseResultIndex = _buffer.lastIndexOf("CME ERROR")) != -1) {
                        _ready = 4;
                    }

                    if (_ready != 0) {
                        if (_responseDataStorage != nullptr) {
                            if (_ready > 1) {
                                _buffer.substring(responseResultIndex);
                            } else {
                                _buffer.remove(responseResultIndex);
                            }
                            _buffer.trim();

                            *_responseDataStorage = _buffer;

                            _responseDataStorage = nullptr;
                        }

                        _atCommandState = AT_COMMAND_IDLE;
                        _buffer = "";
                        return;
                    }
                }
                break;
            }
        }
    }
}

void Modem::setResponseDataStorage(String *responseDataStorage) {
    _responseDataStorage = responseDataStorage;
}

void Modem::addUrcHandler(ModemUrcHandler *handler) {
    for (auto & _urcHandler : _urcHandlers) {
        if (_urcHandler == nullptr) {
            _urcHandler = handler;
            break;
        }
    }
}

void Modem::removeUrcHandler(ModemUrcHandler *handler) {
    for (auto & _urcHandler : _urcHandlers) {
        if (_urcHandler == handler) {
            _urcHandler = nullptr;
            break;
        }
    }
}

void Modem::setBaudRate(unsigned long baud) {
    _baud = baud;
}

void Modem::powerOn(bool restart) const {

    pinMode(_powerOnPin, OUTPUT);
    digitalWrite(_powerOnPin, LOW);
    //delay(LTE_SHIELD_POWER_PULSE_PERIOD);
    pinMode(_powerOnPin, INPUT); // Return to high-impedance, rely on SARA module internal pull-up

    if (_resetPin != 255 && restart) {
        pinMode(_resetPin, OUTPUT);
        digitalWrite(_resetPin, LOW);
        delay(LTE_RESET_PULSE_PERIOD);
        pinMode(_resetPin, INPUT); // Return to high-impedance, rely on SARA module internal pull-up
    }

    return;

    pinMode(_powerOnPin, OUTPUT);
    digitalWrite(_powerOnPin, HIGH);

    if (_resetPin == 255 && restart) {
        pinMode(_resetPin, OUTPUT);
        digitalWrite(_resetPin, HIGH);
        delay(100);
        digitalWrite(_resetPin, LOW);
    }
}

void Modem::powerOff() const {
    pinMode(_powerOnPin, INPUT); // Return to high-impedance, rely on SARA module internal pull-up
}

Modem::~Modem() {
    delete _handler;
}