/**
 * request_guard - Contrôles purs des requêtes HTTP d'écriture (sans dépendance Arduino)
 *
 * Volontairement écrit en C++ standard pur : le même fichier est compilé par le firmware
 * et par le test natif (test/test_request_guard), ce qui permet de vérifier la logique
 * sans carte. L'envoi des réponses 401/403 reste dans modules/http_guard.cpp.
 */

#pragma once
#include <cstddef>
#include <string>

namespace reqguard {

// Partie « hôte[:port] » d'une valeur d'en-tête Origin.
//   "http://gatewaylab.local"      -> "gatewaylab.local"
//   "http://192.168.1.42:8080"     -> "192.168.1.42:8080"
inline std::string originAuthority(const std::string& origin) {
    size_t start = origin.find("://");
    start = (start == std::string::npos) ? 0 : start + 3;
    size_t end = origin.find('/', start);
    return origin.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

inline bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = static_cast<char>(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = static_cast<char>(y - 'A' + 'a');
        if (x != y) return false;
    }
    return true;
}

// Protection CSRF d'une écriture (POST, PUT, PATCH, DELETE).
//   - Pas d'en-tête Origin : client hors navigateur (curl, script, outil du parc) -> accepté.
//     Un navigateur envoie TOUJOURS Origin sur une écriture inter-origine.
//   - Origin présent : son hôte:port doit être celui de l'en-tête Host de la requête,
//     sinon une page web tierce visitée depuis le LAN tente d'écrire sur la carte.
//   - Origin présent mais Host absent : refusé (cas anormal, mieux vaut échouer fermé).
inline bool originAllowed(const std::string& origin, const std::string& host) {
    if (origin.empty()) return true;
    if (host.empty()) return false;
    return equalsIgnoreCase(originAuthority(origin), host);
}

// Comparaison du jeton d'administration à temps constant (indépendant du premier octet
// différent) : évite de deviner le jeton en mesurant le temps de réponse.
// Un jeton attendu vide ne valide jamais rien (l'authentification est alors désactivée
// en amont, ce n'est pas à cette fonction d'accepter une valeur vide).
inline bool tokenEquals(const std::string& given, const std::string& expected) {
    if (expected.empty()) return false;
    unsigned char diff = (given.size() == expected.size()) ? 0 : 1;
    for (size_t i = 0; i < expected.size(); ++i) {
        const char g = (i < given.size()) ? given[i] : '\0';
        diff = static_cast<unsigned char>(diff | (static_cast<unsigned char>(g) ^
                                                  static_cast<unsigned char>(expected[i])));
    }
    return diff == 0;
}

} // namespace reqguard
