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
    SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE
};


NBSSLClient::NBSSLClient(Modem &modem, NBSecurityData certs[], size_t numCerts, bool synch) : NBClient(modem, synch) {
    for (int i = 0; i < numCerts; ++i) {
        addRootCert(&certs[i]);
    }
}



NBSSLClient::NBSSLClient(Modem &modem, bool synch) :
        NBSSLClient(modem, {}, 0, synch) {}


NBSSLClient::~NBSSLClient() {
}

int NBSSLClient::ready() {
    if (_rootCertsLoaded) {
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
            if (_certs[_certIndex].size) {
                // load the next root cert
                _modem.sendf("AT+USECMNG=0,%d,\"%s\",%d", _certs[_certIndex].certType, _certs[_certIndex].name,
                             _certs[_certIndex].size);
                if (_modem.waitForPrompt() != 1) {
                    // failure
                    ready = -1;
                } else {
                    // send the cert contents
                    _modem.write(_certs[_certIndex].data, _certs[_certIndex].size);
                    _state = SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE;
                    ready = 0;
                }
            } else {
                // remove the next root cert name
                _modem.sendf("AT+USECMNG=2,0,\"%s\"", _certs[_certIndex].name);

                _state = SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE;
                ready = 0;
            }
            break;
        }

        case SSL_CLIENT_STATE_WAIT_LOAD_ROOT_CERT_RESPONSE: {
            if (ready > 1) {
                // error
            } else {
                _certIndex++;
                if (_certIndex == _numCerts) {
                    // all certs loaded
                    _rootCertsLoaded = true;
                } else {
                    // load next
                    _state = SSL_CLIENT_STATE_LOAD_ROOT_CERT;
                }
                ready = 0;
            }
            break;
        }

        case SSL_CLIENT_STATE_WAIT_DELETE_ROOT_CERT_RESPONSE: {
            // ignore ready response, root cert might not exist
            _certIndex++;

            if (_certIndex == _numCerts) {
                // all certs loaded
                _rootCertsLoaded = true;
            } else {
                // load next
                _state = SSL_CLIENT_STATE_LOAD_ROOT_CERT;
            }

            ready = 0;
            break;
        }
    }

    return ready;
}

int NBSSLClient::connect(IPAddress ip, uint16_t port) {
    _certIndex = 0;
    _state = SSL_CLIENT_STATE_LOAD_ROOT_CERT;

    return connectSSL(ip, port);
}

int NBSSLClient::connect(const char *host, uint16_t port) {
    _certIndex = 0;
    _state = SSL_CLIENT_STATE_LOAD_ROOT_CERT;

    return connectSSL(host, port);
}

bool NBSSLClient::addRootCert(NBSecurityData *cert) {
    for (auto & _rootCert : _rootCerts) {
        if (_rootCert == nullptr) {
            _rootCert = cert;
            _numCerts++;
            return true;
        }
    }
    return false;
}
