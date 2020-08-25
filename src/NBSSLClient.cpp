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

#include "utility/NBRootCerts.h"

#include "Modem.h"

#include "NBSSLClient.h"

enum {
    SSL_CLIENT_STATE_LOAD_ROOT_CERT,
    SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE,
    SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE,
    SSL_CLIENT_LOADED
};


NBSSLClient::NBSSLClient(Modem &modem, NBSecurityData* certs[], size_t numCerts, bool synch) : NBClient(modem, synch) {
    for (size_t i = 0; i < numCerts; ++i) {
        addSecurityData(certs[i]);
    }
}



NBSSLClient::NBSSLClient(Modem &modem, bool synch) :
        NBSSLClient(modem, nullptr, 0, synch) {}


NBSSLClient::~NBSSLClient() = default;

int NBSSLClient::ready() {
    if (_state == SSL_CLIENT_LOADED) {
        // root certs loaded already, continue to regular NBClient
        return NBClient::ready();
    }

    int ready = _modem.ready();
    if (ready == 0) {
        // a command is still running
        return 0;
    }

    switch (_state) {
        case SSL_CLIENT_STATE_LOAD_ROOT_CERT: {
            if (_securityData[_certIndex]->size) {
                // load the next root cert
                _modem.sendf("AT+USECMNG=0,%d,\"%s\",%d", _securityData[_certIndex]->certType, _securityData[_certIndex]->name,
                             _securityData[_certIndex]->size);
                if (_modem.waitForPrompt() != 1) {
                    // failure
                    ready = -1;
                } else {
                    // send the cert contents
                    _modem.write_P(_securityData[_certIndex]->data, _securityData[_certIndex]->size);
                    _state = SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE;
                    ready = 0;
                }
            } else {
                // remove the next root cert name
                _modem.sendf("AT+USECMNG=2,%s,\"%s\"", _securityData[_certIndex]->certType, _securityData[_certIndex]->name);

                _state = SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE;
                ready = 0;
            }
            break;
        }

        case SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE:
        case SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE: {
            if (_state == SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE /*ignore error  if deleting, root cert might not exist */ && ready > 1) {
                // error
            } else {
                switch (_securityData[_certIndex]->certType) {
                    case NBSecurityData::CA:
                        break;
                    case NBSecurityData::CC:
                    {
                        _modem.sendf("AT+USECPRF=0,5,\"%s\"",_securityData[_certIndex]->name);
                        break;
                    }
                    case NBSecurityData::PK:
                    {
                        _modem.sendf("AT+USECPRF=0,6,\"%s\"",_securityData[_certIndex]->name);
                        break;
                    }
                }
                _certIndex++;
                if (_certIndex == _numCerts) {
                    // all certs loaded, move on to setting up security profile
                    _state = SSL_CLIENT_LOADED;
                } else {
                    // load next
                    _state = SSL_CLIENT_STATE_LOAD_ROOT_CERT;
                }
                ready = 0;
            }
            break;
        }
    }

    return ready;
}

int NBSSLClient::connect(IPAddress ip, uint16_t port) {
    return connectSSL(ip, port);
}

int NBSSLClient::connect(const char *host, uint16_t port) {
    return connectSSL(host, port);
}

bool NBSSLClient::addSecurityData(NBSecurityData *datum) {
    for (auto & securityDatum : _securityData) {
        if (securityDatum == nullptr) {
            securityDatum = datum;
            _numCerts++;
            return true;
        }
    }
    return false;
}
