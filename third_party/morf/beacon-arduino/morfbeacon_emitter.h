// -----------------------------------------------------------------------------
// morfBeacon — emetteur de presence pour cibles embarquees (ESP32 / Arduino)
// Copyright (C) 2026 morfredus
// SPDX-License-Identifier: GPL-3.0-only
//
// Emet le heartbeat « morfbeacon/1 », exactement comme la bibliotheque Qt, pour
// qu'un equipement embarque soit decouvert par le MEME mecanisme que les
// services Linux et Windows.
//
// Pourquoi une seconde implementation
// -----------------------------------
// La bibliotheque Qt ne peut pas tourner sur un ESP32. La frontiere de
// plateforme rend cette duplication inevitable — c'est la meme raison qui fait
// coexister un ecouteur C++ dans morfMonitor et un ecouteur Python dans
// RaspberryDashboard.
//
// Ce qui est partage n'est donc pas du code mais le PROTOCOLE. Les deux
// emetteurs vivent volontairement dans le meme depot : lire l'un a cote de
// l'autre est le seul garde-fou contre une divergence silencieuse du format.
// Toute evolution du datagramme doit toucher les deux fichiers.
//
// Sans dependance autre qu'Arduino + WiFi : pas d'ArduinoJson, le datagramme
// etant trop simple pour justifier une bibliotheque et devant rester compact.
//
// Usage :
//     #include "morfbeacon_emitter.h"
//     morfbeacon::Emitter beacon;
//
//     void setup() {
//         beacon.appName     = "MeteoHub";
//         beacon.version     = PROJECT_VERSION;
//         beacon.statusPort  = 80;          // port du serveur HTTP
//         beacon.webUi       = true;        // declare la capacite « web_ui »
//         beacon.begin();
//     }
//     void loop() { beacon.update(); }      // n'emet que toutes les 15 s
// -----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

namespace morfbeacon {

class Emitter {
public:
    // --- Identite annoncee (LIBRE, modifiable par l'utilisateur) ------------
    String appName  = "Device";
    String version  = "0.0.0";
    String state    = "ok";        // ok | warning | starting | error
    String instance;               // defaut : appName@hostname

    // --- Capacites (STABLES, jamais renommees) ------------------------------
    // Un consommateur cherche une CAPACITE, jamais un nom : morfSystem etant
    // sous licence GPL, chacun peut renommer son application, et une detection
    // fondee sur le nom casserait au premier renommage.
    bool   webUi = false;          // declare « web_ui »
    String extraCapabilities;      // autres capacites, separees par des virgules

    // --- Reseau -------------------------------------------------------------
    uint16_t udpPort            = 45454;   // port du parc morfSystem
    uint16_t statusPort         = 80;      // ou repond /status
    uint32_t broadcastIntervalMs = 15000;  // periode d'annonce

    void begin() {
        _udp.begin(0);                     // port source quelconque : on emet
        _startedMs = millis();
        _lastSendMs = 0;                   // force une annonce immediate
    }

    // A appeler dans loop(). Ne fait rien tant que l'intervalle n'est pas
    // ecoule : le cout par tour de boucle est une soustraction.
    void update() {
        if (WiFi.status() != WL_CONNECTED)
            return;                        // rien a annoncer sans reseau
        const uint32_t now = millis();
        if (_lastSendMs != 0 && (now - _lastSendMs) < broadcastIntervalMs)
            return;
        _lastSendMs = now;
        send();
    }

    // Emission immediate (changement d'etat notable, par exemple).
    void send() {
        const String payload = buildDatagram();
        // Broadcast dirige du sous-reseau plutot que 255.255.255.255 : plus
        // fiable, et non filtre par la plupart des points d'acces.
        IPAddress bcast = WiFi.localIP();
        const IPAddress mask = WiFi.subnetMask();
        for (int i = 0; i < 4; ++i)
            bcast[i] = (bcast[i] & mask[i]) | ~mask[i];

        _udp.beginPacket(bcast, udpPort);
        _udp.print(payload);
        _udp.endPacket();
    }

private:
    String buildDatagram() const {
        const String host = WiFi.getHostname() ? String(WiFi.getHostname()) : String("esp32");
        const uint32_t uptime = (millis() - _startedMs) / 1000UL;

        String caps;
        if (webUi)
            caps = "\"web_ui\"";
        if (extraCapabilities.length()) {
            for (int start = 0; start < (int)extraCapabilities.length();) {
                int comma = extraCapabilities.indexOf(',', start);
                if (comma < 0) comma = extraCapabilities.length();
                String one = extraCapabilities.substring(start, comma);
                one.trim();
                if (one.length()) {
                    if (caps.length()) caps += ",";
                    caps += "\"" + escape(one) + "\"";
                }
                start = comma + 1;
            }
        }

        String o = "{";
        o += "\"proto\":\"morfbeacon/1\"";
        o += ",\"app\":\""     + escape(appName) + "\"";
        o += ",\"host\":\""    + escape(host) + "\"";
        o += ",\"version\":\"" + escape(version) + "\"";
        o += ",\"state\":\""   + escape(state) + "\"";
        o += ",\"status_port\":" + String(statusPort);
        o += ",\"instance\":\"" +
             escape(instance.length() ? instance : (appName + "@" + host)) + "\"";
        // Emises seulement si declarees, pour que le datagramme reste court.
        if (caps.length())
            o += ",\"capabilities\":[" + caps + "]";
        o += ",\"uptime_s\":" + String(uptime);
        // ts : secondes Unix si l'heure est connue, 0 sinon. Un ESP32 sans NTP
        // n'a pas d'horloge : mieux vaut 0, lisible comme « inconnu », qu'une
        // date de 1970 qu'un consommateur prendrait pour une mesure.
        const time_t nowSec = time(nullptr);
        o += ",\"ts\":" + String(nowSec > 1600000000 ? (uint32_t)nowSec : 0);
        o += "}";
        return o;
    }

    // Echappement JSON minimal : les champs sont des identifiants et des
    // libelles, jamais du texte libre, mais un guillemet suffirait a produire un
    // datagramme invalide que le consommateur rejetterait en silence.
    static String escape(const String& in) {
        String out;
        out.reserve(in.length());
        for (size_t i = 0; i < in.length(); ++i) {
            const char c = in[i];
            if (c == '"' || c == '\\') { out += '\\'; out += c; }
            else if (c == '\n')        { out += "\\n"; }
            else if ((unsigned char)c < 0x20) { /* ignore les caracteres de controle */ }
            else                       { out += c; }
        }
        return out;
    }

    WiFiUDP  _udp;
    uint32_t _startedMs  = 0;
    uint32_t _lastSendMs = 0;
};

// -----------------------------------------------------------------------------
// Corps de reponse pour GET /status, au format attendu par un consommateur.
//
// Le heartbeat annonce la CAPACITE ; ce document en donne le DETAIL. Un service
// qui declare « web_ui » sans servir ce bloc annonce une interface que personne
// ne saura ouvrir.
// -----------------------------------------------------------------------------
inline String buildStatusJson(const Emitter& e,
                              const String& webUiPath  = "/",
                              const String& webUiLabel = String(),
                              const String& webUiDesc  = String(),
                              const String& metricsJson = String()) {
    const String host = WiFi.getHostname() ? String(WiFi.getHostname()) : String("esp32");
    String o = "{";
    o += "\"app\":\""     + e.appName + "\"";
    o += ",\"host\":\""   + host + "\"";
    o += ",\"version\":\"" + e.version + "\"";
    o += ",\"state\":\""  + e.state + "\"";
    o += ",\"uptime_s\":" + String(millis() / 1000UL);
    if (metricsJson.length())
        o += ",\"metrics\":" + metricsJson;
    if (e.webUi) {
        o += ",\"web_ui\":{\"path\":\"" + webUiPath + "\"";
        o += ",\"label\":\"" + (webUiLabel.length() ? webUiLabel : e.appName) + "\"";
        o += ",\"port\":" + String(e.statusPort);
        if (webUiDesc.length())
            o += ",\"description\":\"" + webUiDesc + "\"";
        o += "}";
    }
    o += "}";
    return o;
}

} // namespace morfbeacon
