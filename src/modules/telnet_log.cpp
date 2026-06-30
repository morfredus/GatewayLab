#include "telnet_log.h"

TelnetLog telnetLog;

void TelnetLog::begin(uint16_t port) {
    _port    = port;
    _mutex   = xSemaphoreCreateMutex();
    _started = true;
}

void TelnetLog::loop() {
    // Sans état en UDP : rien à faire.
}

void TelnetLog::write(const char* data, size_t len) {
    if (!_started) return;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) != pdTRUE) return;

    // Broadcast sur le sous-réseau local (suppose un masque /24, le cas
    // courant en LAN domestique) — aucun client à suivre, aucune connexion
    // à maintenir.
    IPAddress broadcast = WiFi.localIP();
    broadcast[3] = 255;

    _udp.beginPacket(broadcast, _port);
    _udp.write((const uint8_t*)data, len);
    _udp.endPacket();

    xSemaphoreGive(_mutex);
}
