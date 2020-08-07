/*
  This file is part of the MKR NB IoT library.
  Copyright (C) 2018 Arduino SA (http://www.arduino.cc/)

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

#ifndef _NB_SSL_CLIENT_H_INCLUDED
#define _NB_SSL_CLIENT_H_INCLUDED

#include "NBClient.h"

#include "utility/NBRootCerts.h"

class NBSSLClient : public NBClient {

public:
    NBSSLClient(Modem &modem, NBSecurityData* certs[], size_t numCerts, bool synch = true);
    explicit NBSSLClient(Modem &modem, bool synch = true);

    virtual ~NBSSLClient();

    virtual int ready();

    virtual int connect(IPAddress ip, uint16_t port);

    virtual int connect(const char *host, uint16_t port);

    bool addSecurityData(NBSecurityData *datum);

private:
#define MAX_ROOT_CERTS 4
    NBSecurityData *_securityData[MAX_ROOT_CERTS] = {};
    int _numCerts = 0;
    int _certIndex = 0;
    int _state = 0;
};

#endif
