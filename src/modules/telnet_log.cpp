#include "telnet_log.h"

TelnetLog telnetLog;

void TelnetLog::begin(uint16_t port) {
    _server = WiFiServer(port);
    _server.begin();
    _server.setNoDelay(true);
    _started = true;
}

void TelnetLog::loop() {
    if (!_started) return;

    // Un seul client a la fois — une nouvelle connexion remplace l'ancienne
    // (évite de garder un socket mort si YAT a été fermé sans déconnexion propre).
    if (_server.hasClient()) {
        if (_client && _client.connected()) _client.stop();
        _client = _server.available();
    }

    if (_client && !_client.connected()) _client.stop();
}

void TelnetLog::write(const char* data, size_t len) {
    if (!_started || !_client || !_client.connected()) return;
    _client.write((const uint8_t*)data, len);
}
