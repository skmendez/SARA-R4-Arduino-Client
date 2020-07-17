//
// Created by Sebastian on 7/17/2020.
//

#ifndef SARA_R4_ARDUINO_CLIENT_MODEMCONFIG_H
#define SARA_R4_ARDUINO_CLIENT_MODEMCONFIG_H

#include <SARAClient.h>

#define UART Serial
#define BAUD 115200
#define POWER_PIN 5
#define RESET_PIN 6

#ifdef SAMD21
#define UART SerialSARA
#define POWER_PIN SARA_PWR_ON
#define RESET_PIN SARA_RESETN
#endif

Modem MODEM(UART, BAUD, POWER_PIN, RESET_PIN); // NOLINT(cert-err58-cpp)

#endif //SARA_R4_ARDUINO_CLIENT_MODEMCONFIG_H
