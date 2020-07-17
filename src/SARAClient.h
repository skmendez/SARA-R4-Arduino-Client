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

#ifndef _SARAClient_H_INCLUDED
#define _SARAClient_H_INCLUDED

#include "NB.h"
#include "NB_SMS.h"
#include "GPRS.h"
#include "NBClient.h"
#include "NBModem.h"
#include "NBScanner.h"
#include "NBPIN.h"

#include "NBSSLClient.h"
#include "NBUdp.h"

#ifdef TRAVIS_CI

#define BAUD 115200

#ifndef
#define MODEM_SERIAL Serial
#define POWER_PIN 5
#define RESET_PIN 6
#endif

#ifdef SARA_PWR_ON
#define MODEM_SERIAL SerialSARA
#define POWER_PIN SARA_PWR_ON
#define RESET_PIN SARA_RESETN
#endif

Modem MODEM(MODEM_SERIAL, BAUD, POWER_PIN, RESET_PIN); // NOLINT(cert-err58-cpp)
#endif

#endif // _SARAClient_H_INCLUDED
