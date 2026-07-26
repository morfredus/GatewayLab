/**
 * fs_lock — Verrou global du système de fichiers LittleFS
 *
 * LittleFS n'est PAS réentrant. Or plusieurs tâches FreeRTOS ouvrent, écrivent
 * et ferment des fichiers : le scan et le rescan approfondi (device_store,
 * device_history), les handlers web (favori / alias / notes, restauration), le
 * journal de redémarrage (boot_log). Deux séquences open→…→close qui se
 * chevauchent — même sur des fichiers DIFFÉRENTS — corrompent l'état interne
 * partagé de LittleFS, ce qui déclenche l'assert `lfs_file_close` /
 * `lfs_mlist_isopen` et fait paniquer l'appareil.
 *
 * Toute opération fichier du firmware passe par ce même verrou. Recursif : un
 * appelant qui le détient déjà (ex. addEvent qui appelle save) peut le reprendre
 * sans se bloquer lui-même.
 *
 * Usage :
 *     #include "utils/fs_lock.h"
 *     void MaClasse::save() {
 *         fslock::Guard lock;          // pris ici, rendu en sortie de portée
 *         File f = LittleFS.open(...);
 *         ...
 *     }
 */
#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace fslock {

// Le mutex, créé à la première demande. Appeler handle() une fois au tout début
// de setup() (avant de lancer la moindre tâche) garantit une initialisation
// mono-thread.
inline SemaphoreHandle_t handle() {
    static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    return m;
}

// Garde RAII : prend le verrou à la construction, le rend à la destruction —
// donc à chaque sortie de fonction, y compris les returns anticipés.
struct Guard {
    Guard()  { SemaphoreHandle_t m = handle(); if (m) xSemaphoreTakeRecursive(m, portMAX_DELAY); }
    ~Guard() { SemaphoreHandle_t m = handle(); if (m) xSemaphoreGiveRecursive(m); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
};

} // namespace fslock
