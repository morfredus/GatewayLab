#pragma once

// Dupliquez ce fichier en secrets.h pour le développement local.
// include/secrets.h est ignoré par Git (.gitignore) - ne le committez jamais.
//
// Depuis la v0.3.0, secrets.h n'est PLUS la méthode officielle de
// configuration WiFi : un portail de configuration web s'en charge
// automatiquement au premier démarrage (voir docs/WIFI_SETUP.md).
//
// DEFAULT_WIFI_SSID / DEFAULT_WIFI_PASSWORD restent utiles en développement
// pour éviter de ressaisir le WiFi à chaque flash : ils ne sont utilisés que
// si AUCUN réseau n'est encore enregistré en mémoire NVS de l'ESP32.
//
// Hiérarchie de configuration WiFi (priorité décroissante) :
//   1. Réseaux enregistrés en NVS (via le portail de configuration web)
//   2. DEFAULT_WIFI_SSID / DEFAULT_WIFI_PASSWORD ci-dessous (développement)
//   3. Portail de configuration (point d'accès "GatewayLab-Setup")

#define DEFAULT_WIFI_SSID     "MonWifi"
#define DEFAULT_WIFI_PASSWORD "MonMotDePasse"

// --- Protection de la carte (OPTIONNEL) ------------------------------------
// Sans cette ligne, OTA, /update et reglages Wi-Fi sont ouverts a tout le reseau local
// (un avertissement est ecrit dans le journal a chaque demarrage). Avec elle :
//   - ArduinoOTA exige ce mot de passe (pio : --auth=<jeton> dans upload_flags) ;
//   - /update, restauration, redemarrage et modification des reseaux Wi-Fi exigent
//     l'en-tete HTTP X-Auth-Token ; l'interface web le demande une fois par onglet ;
//   - la sauvegarde ?full=1 inclut les mots de passe Wi-Fi.
// GATEWAYLAB_OTA_PASSWORD (facultatif) remplace le jeton pour ArduinoOTA seulement.
// #define GATEWAYLAB_AUTH_TOKEN "une-longue-chaine-aleatoire"
// #define GATEWAYLAB_OTA_PASSWORD "mot-de-passe-ota"
