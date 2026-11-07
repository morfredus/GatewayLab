/**
 * http_guard - Implémentation (voir http_guard.h)
 */

#include "http_guard.h"
#include "../utils/logger.h"
#include "../utils/request_guard.h"

// secrets.h est optionnel (ignoré par Git) : il peut y définir GATEWAYLAB_AUTH_TOKEN.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

static const char* TAG = "HttpGuard";

namespace httpGuard {

void attach(WebServer& server) {
    static const char* kKeys[] = {"Origin", "X-Auth-Token"};
    server.collectHeaders(kKeys, sizeof(kKeys) / sizeof(kKeys[0]));
}

bool authEnabled() {
#ifdef GATEWAYLAB_AUTH_TOKEN
    return GATEWAYLAB_AUTH_TOKEN[0] != '\0';
#else
    return false;
#endif
}

bool tokenValid(WebServer& server) {
#ifdef GATEWAYLAB_AUTH_TOKEN
    return reqguard::tokenEquals(server.header("X-Auth-Token").c_str(), GATEWAYLAB_AUTH_TOKEN);
#else
    (void)server;
    return false;
#endif
}

Verdict check(WebServer& server, bool needToken) {
    // CSRF : hostHeader() est mémorisé par le serveur, Origin vient de collectHeaders().
    if (!reqguard::originAllowed(server.header("Origin").c_str(), server.hostHeader().c_str()))
        return Verdict::CrossOrigin;
    if (needToken && authEnabled() && !tokenValid(server))
        return Verdict::TokenRequired;
    return Verdict::Ok;
}

void reject(WebServer& server, Verdict verdict) {
    if (verdict == Verdict::CrossOrigin) {
        Log::w(TAG, "Ecriture refusee : origine differente de l'hote (%s)",
               server.uri().c_str());
        server.send(403, "application/json", "{\"error\":\"cross_origin_forbidden\"}");
    } else if (verdict == Verdict::TokenRequired) {
        Log::w(TAG, "Route sensible refusee : jeton absent ou invalide (%s)",
               server.uri().c_str());
        server.send(401, "application/json", "{\"error\":\"auth_required\"}");
    }
}

bool enforce(WebServer& server, bool needToken) {
    const Verdict v = check(server, needToken);
    if (v == Verdict::Ok) return true;
    reject(server, v);
    return false;
}

void logStatus() {
    if (authEnabled()) {
        Log::i(TAG, "Jeton d'administration actif (OTA, /update, Wi-Fi, restauration, redemarrage)");
    } else {
        Log::w(TAG, "ATTENTION : aucune authentification - OTA, /update et reglages Wi-Fi ouverts "
                    "a tout le reseau local. Definir GATEWAYLAB_AUTH_TOKEN dans include/secrets.h");
    }
}

} // namespace httpGuard
