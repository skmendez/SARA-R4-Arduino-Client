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

#include "NBPIN.h"

NBPIN::NBPIN(Modem &modem) :
        _modem(modem),

_pinUsed(false)
{
}

void NBPIN::begin() {
    _modem.begin();
}

int NBPIN::isPIN() {
    String response;

    for (unsigned long start = millis(); (millis() - start) < 1000;) {
        _modem.send("AT+CPIN?");

        if (_modem.waitForResponse(10000, &response) == 1) {
            if (response.startsWith("+CPIN: ")) {
                if (response.endsWith("READY")) {
                    return 0;
                } else if (response.endsWith("SIM PIN")) {
                    return 1;
                } else if (response.endsWith("SIM PUK")) {
                    return -1;
                } else {
                    return -2;
                }
            }
        }

        delay(100);
    }

    return -2;
}

int NBPIN::checkPIN(String pin) {
    _modem.sendf("AT+CPIN=\"%s\"", pin.c_str());
    if (_modem.waitForResponse(10000) == 1) {
        return 0;
    }

    return -1;
}

int NBPIN::checkPUK(String puk, String pin) {
    _modem.sendf("AT+CPIN=\"%s\",\"%s\"", puk.c_str(), pin.c_str());
    if (_modem.waitForResponse(10000) == 1) {
        return 0;
    }

    return -1;
}

void NBPIN::changePIN(String old, String pin) {
    _modem.sendf("AT+CPWD=\"SC\",\"%s\",\"%s\"", old.c_str(), pin.c_str());
    if (_modem.waitForResponse(10000) == 1) {
        Serial.println("Pin changed succesfully.");
    } else {
        Serial.println("ERROR");
    }
}

void NBPIN::switchPIN(String pin) {
    String response;

    _modem.send("AT+CLCK=\"SC\",2");
    if (_modem.waitForResponse(180000, &response) != 1) {
        Serial.println("ERROR");
        return;
    }

    if (response == "+CLCK: 0") {
        _modem.sendf("AT+CLCK=\"SC\",1,\"%s\"", pin.c_str());
        if (_modem.waitForResponse(180000, &response) == 1) {
            Serial.println("OK. PIN lock on.");
            _pinUsed = true;
        } else {
            Serial.println("ERROR");
            _pinUsed = false;
        }
    } else if (response == "+CLCK: 1") {
        _modem.sendf("AT+CLCK=\"SC\",0,\"%s\"", pin.c_str());
        if (_modem.waitForResponse(180000, &response) == 1) {
            Serial.println("OK. PIN lock off.");
            _pinUsed = false;
        } else {
            Serial.println("ERROR");
            _pinUsed = true;
        }
    } else {
        Serial.println("ERROR");
    }
}

int NBPIN::checkReg() {
    for (unsigned long start = millis(); (millis() - start) < 10000L;) {
        _modem.send("AT+CREG?");

        String response = "";

        if (_modem.waitForResponse(100, &response) == 1) {
            if (response.startsWith("+CREG: ")) {
                if (response.endsWith(",1")) {
                    return 0;
                } else if (response.endsWith(",5")) {
                    return 1;
                }
            }
        }

        delay(100);
    }

    return -1;
}

bool NBPIN::getPINUsed() {
    return _pinUsed;
}

void NBPIN::setPINUsed(bool used) {
    _pinUsed = used;
}
