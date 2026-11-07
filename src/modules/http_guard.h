/**
 * http_guard - Protection des écritures HTTP : CSRF (Origin/Host) et jeton d'administration
 *
 * Deux protections indépendantes :
 *
 *   1. CSRF, TOUJOURS active sur POST/PUT/PATCH/DELETE : une page web visitée depuis le LAN ne
 *      peut pas écrire sur la carte (en-tête Origin comparé à Host). Les clients hors navigateur
 *      (curl, scripts, outils du parc) n'envoient pas Origin et passent.
 *
 *   2. Jeton d'administration, OPTIONNEL : actif seulement si include/secrets.h définit
 *      GATEWAYLAB_AUTH_TOKEN. Exigé (en-tête X-Auth-Token) sur les routes sensibles :
 *      mise à jour du firmware, ajout/suppression de réseaux Wi-Fi, restauration, redémarrage.
 *      Sans la macro, le comportement historique est conservé (routes ouvertes) et un
 *      avertissement est écrit dans le journal au démarrage.
 *
 * Le même jeton sert de mot de passe à ArduinoOTA (voir ota_manager.cpp).
 */

#pragma once
#include <Arduino.h>
#include <WebServer.h>

namespace httpGuard {

// À appeler une fois AVANT WebServer::begin() : demande au serveur de conserver les en-têtes
// Origin et X-Auth-Token (il ignore les autres par défaut).
void attach(WebServer& server);

// true si un jeton d'administration est configuré à la compilation.
bool authEnabled();

// true si la requête courante porte le bon jeton. Toujours false si l'authentification est
// désactivée : l'appelant combine avec authEnabled() selon ce qu'il veut protéger.
bool tokenValid(WebServer& server);

// Résultat d'un contrôle, sans rien envoyer : utilisable depuis un gestionnaire d'upload,
// où répondre au milieu du flux serait incorrect.
enum class Verdict { Ok, CrossOrigin, TokenRequired };

// needToken : exiger le jeton (ignoré si l'authentification est désactivée).
Verdict check(WebServer& server, bool needToken);

// check() puis envoi de la réponse d'erreur (403 cross_origin_forbidden / 401 auth_required).
// Retourne true si la requête peut être traitée.
bool enforce(WebServer& server, bool needToken);

// Envoie la réponse d'erreur correspondant à un verdict non Ok.
void reject(WebServer& server, Verdict verdict);

// Journalise (une fois) l'état de l'authentification : avertissement si elle est absente.
void logStatus();

} // namespace httpGuard
