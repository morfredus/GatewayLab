/**
 * OtaManager - Implémentation
 *
 * Bibliothèques utilisées :
 *   ArduinoOTA - mise à jour réseau depuis PlatformIO ou Arduino IDE
 *   Update     - écriture du firmware en flash (utilisée pour la web OTA)
 */

#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <Update.h>
#include "app_config.h"         // ENABLE_OTA
#include "../utils/logger.h"
#include "boot_log.h"
#include "http_guard.h"         // CSRF + jeton d'administration optionnel

// secrets.h est optionnel (ignore par Git) : GATEWAYLAB_AUTH_TOKEN (jeton d'administration,
// aussi mot de passe ArduinoOTA) et GATEWAYLAB_OTA_PASSWORD (mot de passe OTA distinct) y
// sont definissables. Voir README.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

static const char* TAG = "OTA";

// Instance globale exportée
OtaManager otaMgr;

// true tant qu'une session Update est ouverte et saine pour l'upload web en cours.
// Statique partagee : le serveur HTTP traite une seule requete a la fois (boucle loop()).
static bool _otaWriting = false;

void OtaManager::begin(const char* hostname) {
#ifdef ENABLE_OTA
    ArduinoOTA.setHostname(hostname);

    // Mot de passe ArduinoOTA : sans lui, n'importe quel poste du reseau peut flasher la carte.
    // Optionnel pour ne pas casser les installations existantes ; httpGuard::logStatus()
    // (appele au demarrage du serveur web) avertit quand rien n'est configure.
#if defined(GATEWAYLAB_OTA_PASSWORD)
    ArduinoOTA.setPassword(GATEWAYLAB_OTA_PASSWORD);
#elif defined(GATEWAYLAB_AUTH_TOKEN)
    ArduinoOTA.setPassword(GATEWAYLAB_AUTH_TOKEN);
#endif

    if (!_callbacksRegistered) {
        // Callbacks d'état - enregistrés une seule fois pour éviter l'empilement
        ArduinoOTA.onStart([]() {
            Log::i(TAG, "ArduinoOTA: réception du firmware...");
        });
        ArduinoOTA.onEnd([]() {
            Log::i(TAG, "ArduinoOTA: installation terminée, redémarrage...");
        });
        ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
            Log::d(TAG, "Progression : %u%%", p * 100 / t);
        });
        ArduinoOTA.onError([](ota_error_t err) {
            Log::e(TAG, "Erreur OTA [%u] - vérifiez la connexion réseau", (unsigned)err);
        });
        _callbacksRegistered = true;
    }

    // Rappelé à chaque reconnexion WiFi pour ré-ouvrir le port UDP et re-publier mDNS
    ArduinoOTA.begin();
#if defined(GATEWAYLAB_OTA_PASSWORD) || defined(GATEWAYLAB_AUTH_TOKEN)
    Log::i(TAG, "ArduinoOTA actif (nom réseau : %s) - mot de passe requis", hostname);
#else
    Log::w(TAG, "ArduinoOTA actif SANS mot de passe (nom réseau : %s)", hostname);
#endif
#endif
}

void OtaManager::loop() {
#ifdef ENABLE_OTA
    // Vérification des paquets OTA entrants - doit être appelé à chaque itération
    // de loop() pour ne pas rater le début d'une mise à jour
    ArduinoOTA.handle();
#endif
}

void OtaManager::registerRoutes(WebServer& server) {
    // Le formulaire d'upload est intégré à la page /wifi (Système) -
    // seule la réception du fichier reste gérée ici.
    // Réception du fichier firmware .bin uploadé depuis le navigateur
    // Deux handlers : le premier s'exécute à la fin, le second pendant l'upload
    //
    // Garde-fous : origine (CSRF) et jeton d'administration (si defini) sont verifies AVANT
    // d'ecrire le moindre octet en flash - dans le handler d'upload (qui ne peut pas
    // repondre en cours de flux) puis dans le handler de fin (qui envoie le 401/403).
    server.on("/update", HTTP_POST,
        // Handler de fin : confirme le succès ou l'échec, puis redémarre
        [&server]() {
            const httpGuard::Verdict v = httpGuard::check(server, true);
            if (v != httpGuard::Verdict::Ok) {
                httpGuard::reject(server, v);
                return;
            }
            server.sendHeader("Connection", "close");
            // Echec : on ne redemarre pas, la carte continue sur son firmware actuel
            // (la partition cible n'a pas ete validee, Update.abort() l'a abandonnee).
            if (!_otaWriting || Update.hasError()) {
                _otaWriting = false;
                server.send(500, "text/plain", "FAIL");
                return;
            }
            server.send(200, "text/plain", "OK");
#ifdef BOOT_LOG_ENABLED
            bootLog.service(true);   // Instantané heap/uptime à jour avant ce redémarrage volontaire
#endif
            delay(500);   // Délai pour que le navigateur reçoive la réponse
            ESP.restart();
        },
        // Handler de données : reçoit les chunks du firmware et les écrit en flash
        [&server]() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                _otaWriting = false;
                if (httpGuard::check(server, true) != httpGuard::Verdict::Ok) {
                    Log::w(TAG, "Web OTA: refusée (origine ou jeton), aucune écriture");
                    return;
                }
                Log::i(TAG, "Web OTA: début de réception - %s", upload.filename.c_str());
                // Démarrage de la mise à jour (taille inconnue = mise à jour en streaming).
                // Si begin() échoue (pas de 2e partition, session coincée), on n'écrit rien.
                if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    _otaWriting = true;
                } else {
                    Update.printError(Serial);
                    Update.abort();   // libère une éventuelle session restée ouverte
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (!_otaWriting) return;   // refusée ou begin() échoué : rien à écrire
                // Écriture du chunk reçu en mémoire flash
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                    Update.abort();
                    _otaWriting = false;
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (!_otaWriting) return;
                // Finalisation et vérification de l'intégrité du firmware
                if (Update.end(true)) {
                    Log::i(TAG, "Web OTA: %u octets écrits - succès", upload.totalSize);
                } else {
                    Update.printError(Serial);
                    Update.abort();
                    _otaWriting = false;
                }
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                // Transfert interrompu par le navigateur : abandonner la session en cours
                if (_otaWriting) Update.abort();
                _otaWriting = false;
                Log::w(TAG, "Web OTA: transfert interrompu");
            }
        }
    );
}
