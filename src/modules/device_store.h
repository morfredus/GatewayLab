/**
 * DeviceStore — Persistance LittleFS des équipements connus
 *
 * Sauvegarde les NetworkDevice dans /devices.json entre les boots.
 * Au démarrage d'un scan, les devices connus sont chargés avec online=false.
 * En fin de scan, la liste complète (online + offline) est re-sauvegardée.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include "network_scanner.h"

// load() et save() sont appeles depuis PLUSIEURS taches (scan, rescan approfondi,
// handlers web favori/alias/notes) et partagent LittleFS avec device_history et
// boot_log. La serialisation passe par le verrou GLOBAL fslock (voir
// utils/fs_lock.h), pas par un mutex propre a ce store : deux fichiers
// DIFFERENTS ouverts en meme temps corrompent quand meme l'etat de LittleFS.
class DeviceStore {
public:
    bool begin();
    std::vector<NetworkDevice> load();
    void save(const std::vector<NetworkDevice>& devices);
    bool isMounted() const { return _mounted; }

private:
    static constexpr const char* PATH = "/devices.json";
    bool _mounted = false;
};

extern DeviceStore deviceStore;
