/**
 * TelnetLog — Miroir du moniteur série en UDP broadcast (port TELNET_LOG_PORT)
 *
 * Permet de connecter un terminal comme YAT en UDP plutôt qu'en USB série :
 * diffusion en broadcast sur le sous-réseau local, sans connexion ni état —
 * aucun cycle accept/déconnexion/reconnexion pouvant perturber l'affichage
 * côté client. Écho passif (aucune commande interprétée, c'est un miroir en
 * lecture seule de ce que Serial.print* aurait affiché). Le port série reste
 * actif en parallèle, sans modification de comportement.
 */
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class TelnetLog {
public:
    void begin(uint16_t port);

    // Diffuse une ligne en broadcast UDP sur le sous-réseau local (no-op si
    // begin() n'a pas été appelé). Non bloquant : un récepteur absent ou
    // lent ne ralentit jamais le scan ni le log série (l'UDP est sans accusé
    // de réception).
    // Thread-safe : Log::* est appelé depuis la boucle principale (core 1) ET
    // depuis les tâches de scan (core 0) — le mutex protège l'unique socket
    // WiFiUDP partagé contre des envois concurrents.
    void write(const char* data, size_t len);

    // Sans état en UDP — conservé pour l'API uniforme avec les autres
    // modules (appelé depuis loop() dans main.cpp, no-op ici).
    void loop();

private:
    WiFiUDP  _udp;
    uint16_t _port    = 0;
    bool     _started = false;
    SemaphoreHandle_t _mutex = nullptr;
};

extern TelnetLog telnetLog;
