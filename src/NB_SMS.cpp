/*
  This file is part of the MKR NB library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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

#include "NB_SMS.h"

enum {
    SMS_STATE_IDLE,
    SMS_STATE_LIST_MESSAGES,
    SMS_STATE_WAIT_LIST_MESSAGES_RESPONSE
};

NB_SMS::NB_SMS(Modem &modem, bool synch) :
        _modem(modem),
        _synch(synch),
        _state(SMS_STATE_IDLE),
        _smsTxActive(false) {
}

size_t NB_SMS::write(uint8_t c) {
    if (_smsTxActive) {
        return _modem.write(c);
    }

    return 0;
}

int NB_SMS::beginSMS(const char *to) {
    _modem.sendf("AT+CMGS=\"%s\"", to);
    if (_modem.waitForResponse(100) == 2) {
        _smsTxActive = false;

        return (_synch) ? 0 : 2;
    }

    _smsTxActive = true;

    return 1;
}

int NB_SMS::ready() {
    int ready = _modem.ready();

    if (ready == 0) {
        return 0;
    }

    switch (_state) {
        case SMS_STATE_IDLE:
        default: {
            break;
        }

        case SMS_STATE_LIST_MESSAGES: {
            _modem.setResponseDataStorage(&_incomingBuffer);
            _modem.send("AT+CMGL=\"REC UNREAD\"");
            _state = SMS_STATE_WAIT_LIST_MESSAGES_RESPONSE;
            ready = 0;
            break;
        }

        case SMS_STATE_WAIT_LIST_MESSAGES_RESPONSE: {
            _state = SMS_STATE_IDLE;
            break;
        }
    }

    return ready;
}

int NB_SMS::endSMS() {
    int r;

    if (_smsTxActive) {
        _modem.write(26);

        if (_synch) {
            while ((r = _modem.ready()) == 0) {
                delay(100);
            }
        } else {
            r = _modem.ready();
        }

        return r;
    } else {
        return (_synch ? 0 : 2);
    }
}

int NB_SMS::available() {
    if (_incomingBuffer.length() != 0) {
        int nextMessageIndex = _incomingBuffer.indexOf("\r\n+CMGL: ");

        if (nextMessageIndex != -1) {
            _incomingBuffer.remove(0, nextMessageIndex + 2);
        } else {
            _incomingBuffer = "";
        }
    }

    if (_incomingBuffer.length() == 0) {
        int r;

        if (_state == SMS_STATE_IDLE) {
            _state = SMS_STATE_LIST_MESSAGES;
        }

        if (_synch) {
            while ((r = ready()) == 0) {
                delay(100);
            }
        } else {
            r = ready();
        }

        if (r != 1) {
            return 0;
        }
    }

    if (_incomingBuffer.startsWith("+CMGL: ")) {
        _smsDataIndex = _incomingBuffer.indexOf('\n') + 1;

        _smsDataEndIndex = _incomingBuffer.indexOf("\r\n+CMGL: ");
        if (_smsDataEndIndex == -1) {
            _smsDataEndIndex = _incomingBuffer.length() - 1;
        }

        return (_smsDataEndIndex - _smsDataIndex) + 1;
    } else {
        _incomingBuffer = "";
    }

    return 0;
}

int NB_SMS::remoteNumber(char *number, int nlength) {
#define PHONE_NUMBER_START_SEARCH_PATTERN "\"REC UNREAD\",\""
    int phoneNumberStartIndex = _incomingBuffer.indexOf(PHONE_NUMBER_START_SEARCH_PATTERN);

    if (phoneNumberStartIndex != -1) {
        int i = phoneNumberStartIndex + sizeof(PHONE_NUMBER_START_SEARCH_PATTERN) - 1;

        while (i < (int) _incomingBuffer.length() && nlength > 1) {
            char c = _incomingBuffer[i];

            if (c == '"') {
                break;
            }

            *number++ = c;
            nlength--;
            i++;
        }

        *number = '\0';
        return 1;
    } else {
        *number = '\0';
    }

    return 2;
}

int NB_SMS::read() {
    int bufferLength = _incomingBuffer.length();

    if (_smsDataIndex < bufferLength && _smsDataIndex <= _smsDataEndIndex) {
        return _incomingBuffer[_smsDataIndex++];
    }

    return -1;
}

int NB_SMS::peek() {
    if (_smsDataIndex < (int) _incomingBuffer.length() && _smsDataIndex <= _smsDataEndIndex) {
        return _incomingBuffer[_smsDataIndex];
    }

    return -1;
}

void NB_SMS::flush() {
    int smsIndexStart = _incomingBuffer.indexOf(' ');
    int smsIndexEnd = _incomingBuffer.indexOf(',');

    if (smsIndexStart != -1 && smsIndexEnd != -1) {
        while (_modem.ready() == 0);

        _modem.sendf("AT+CMGD=%s", _incomingBuffer.substring(smsIndexStart + 1, smsIndexEnd).c_str());

        if (_synch) {
            _modem.waitForResponse(55000);
        }
    }
}
