/**
 * Test natif de src/utils/request_guard.h (controle d'origine CSRF + comparaison du jeton).
 *
 *     pio test -e native
 *
 * Aucune carte necessaire : le header est du C++ standard pur, compile ici avec le compilateur
 * de l'hote.
 */

#include <unity.h>
#include "../../src/utils/request_guard.h"

using namespace reqguard;

void setUp() {}
void tearDown() {}

// Client hors navigateur (curl, morfMonitor, scripts) : pas d'Origin -> accepte
void test_sans_origin_accepte() {
    TEST_ASSERT_TRUE(originAllowed("", "gatewaylab.local"));
    TEST_ASSERT_TRUE(originAllowed("", ""));
}

// Page de l'interface servie par la carte elle-meme : Origin == Host
void test_meme_origine_accepte() {
    TEST_ASSERT_TRUE(originAllowed("http://gatewaylab.local", "gatewaylab.local"));
    TEST_ASSERT_TRUE(originAllowed("http://192.168.1.42", "192.168.1.42"));
    TEST_ASSERT_TRUE(originAllowed("http://192.168.1.42:8080", "192.168.1.42:8080"));
    TEST_ASSERT_TRUE(originAllowed("HTTP://GatewayLab.LOCAL", "gatewaylab.local"));
}

// Page tierce visitee depuis le LAN : refusee
void test_autre_origine_refusee() {
    TEST_ASSERT_FALSE(originAllowed("http://evil.example", "gatewaylab.local"));
    TEST_ASSERT_FALSE(originAllowed("http://192.168.1.99", "192.168.1.42"));
    TEST_ASSERT_FALSE(originAllowed("http://gatewaylab.local:8080", "gatewaylab.local"));
    TEST_ASSERT_FALSE(originAllowed("http://gatewaylab.local.evil.example", "gatewaylab.local"));
}

// Origin opaque (iframe sandboxee, fichier local) ou Host manquant : echec ferme
void test_cas_anormaux_refuses() {
    TEST_ASSERT_FALSE(originAllowed("null", "gatewaylab.local"));
    TEST_ASSERT_FALSE(originAllowed("http://gatewaylab.local", ""));
}

void test_jeton() {
    TEST_ASSERT_TRUE(tokenEquals("secret-123", "secret-123"));
    TEST_ASSERT_FALSE(tokenEquals("secret-124", "secret-123"));
    TEST_ASSERT_FALSE(tokenEquals("secret-12", "secret-123"));
    TEST_ASSERT_FALSE(tokenEquals("secret-1234", "secret-123"));
    TEST_ASSERT_FALSE(tokenEquals("", "secret-123"));
    // Jeton attendu vide = authentification desactivee : ne valide jamais rien
    TEST_ASSERT_FALSE(tokenEquals("", ""));
    TEST_ASSERT_FALSE(tokenEquals("x", ""));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_sans_origin_accepte);
    RUN_TEST(test_meme_origine_accepte);
    RUN_TEST(test_autre_origine_refusee);
    RUN_TEST(test_cas_anormaux_refuses);
    RUN_TEST(test_jeton);
    return UNITY_END();
}
