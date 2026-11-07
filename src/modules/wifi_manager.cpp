/**
 * WiFiManager - Implémentation
 *
 * Bibliothèques utilisées :
 *   WiFiMulti    - gestion de plusieurs réseaux, connexion au meilleur signal
 *   ESPmDNS      - résolution de noms sur le réseau local (gatewaylab.local)
 *   Preferences  - persistance NVS des réseaux enregistrés
 *   DNSServer +
 *   WebServer    - portail captif de configuration (mode point d'accès)
 */

#include "wifi_manager.h"
#include <algorithm>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "app_config.h"    // WIFI_CONNECT_TIMEOUT, MDNS_HOSTNAME, ENABLE_MDNS
#include "../utils/logger.h"
#include "../utils/request_guard.h"   // controle d'origine (CSRF) du portail

// secrets.h est optionnel : seul DEFAULT_WIFI_SSID/PASSWORD y est défini,
// utilisé uniquement en développement quand aucun réseau n'est en NVS.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

static const char* TAG            = "WiFi";
static const char* NVS_NAMESPACE  = "wifi";
static const char* NVS_KEY        = "networks";
static const char* AP_SSID        = "GatewayLab-Setup";
static const IPAddress AP_IP(192, 168, 4, 1);

// Instance interne de WiFiMulti - non exposée hors de ce fichier
static WiFiMulti _multi;

// Reconnexion automatique (mode station) : délai PROGRESSIF entre deux tentatives. Un routeur
// qui redémarre met souvent plusieurs minutes : marteler toutes les 30 s ne sert à rien et
// la tentative de WiFiMulti::run() bloque la boucle principale quelques secondes à chaque fois.
// 30 s -> 60 s -> 120 s -> 240 s -> plafond 300 s ; remis à 30 s dès que la connexion revient.
static constexpr unsigned long RECONNECT_DEBOUNCE_MS = 30000;
static constexpr unsigned long RECONNECT_MAX_MS      = 300000;
static unsigned long _reconnectDelayMs     = RECONNECT_DEBOUNCE_MS;
static unsigned long _lastReconnectAttempt = 0;

// Portail de configuration (mode AP) : après un échec au boot, le portail ne doit pas rester
// définitif (coupure de courant de la box au redémarrage de la carte, par exemple). Le WiFi
// est retenté en arrière-plan avec un délai croissant : 2, 5 puis 10 minutes (plafond).
static constexpr unsigned long AP_RETRY_DELAYS_MS[] = {120000UL, 300000UL, 600000UL};
static constexpr size_t        AP_RETRY_STEPS = sizeof(AP_RETRY_DELAYS_MS) / sizeof(AP_RETRY_DELAYS_MS[0]);
static size_t        _apRetryStep   = 0;
static unsigned long _apNextRetryAt = 0;
// Quand un client est connecté au portail (quelqu'un saisit son WiFi), on repousse la tentative
// pour ne pas lui couper la page en pleine saisie.
static constexpr unsigned long AP_RETRY_BUSY_DEFER_MS = 30000;

// Callback conservé pour le rappeler lors d'une reconnexion automatique
static WiFiManager::Callback _storedCb;

// État précédent : permet de détecter la transition déconnecté → connecté
static bool _wasConnected = false;

// true tant que le portail de configuration (point d'accès) est actif
static bool _apMode = false;

static DNSServer _dns;
static WebServer _portal(80);

// Instance globale exportée
WiFiManager wifiMgr;

// ---------------------------------------------------------------------------
// Persistance NVS - liste des réseaux enregistrés, sérialisée en JSON
// ---------------------------------------------------------------------------
static std::vector<WifiCredential> _loadNetworks() {
    std::vector<WifiCredential> list;

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);   // lecture seule
    String json = prefs.getString(NVS_KEY, "[]");
    prefs.end();

    JsonDocument doc;
    if (deserializeJson(doc, json) == DeserializationError::Ok) {
        for (JsonObject o : doc.as<JsonArray>()) {
            WifiCredential c;
            c.ssid     = o["ssid"].as<String>();
            c.password = o["password"].as<String>();
            if (!c.ssid.isEmpty()) list.push_back(c);
        }
    }
    return list;
}

static bool _saveNetworks(const std::vector<WifiCredential>& list) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& c : list) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]     = c.ssid;
        o["password"] = c.password;
    }
    String json;
    serializeJson(doc, json);

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // lecture/écriture
    size_t written = prefs.putString(NVS_KEY, json);
    prefs.end();
    return written == (size_t)json.length();
}

std::vector<WifiCredential> WiFiManager::savedNetworks() const {
    return _loadNetworks();
}

bool WiFiManager::addNetwork(const String& ssid, const String& password) {
    if (ssid.isEmpty()) return false;

    auto list = _loadNetworks();
    for (auto& c : list) {
        if (c.ssid == ssid) {
            c.password = password;   // réseau déjà connu : mise à jour du mot de passe
            return _saveNetworks(list);
        }
    }
    list.push_back({ssid, password});
    return _saveNetworks(list);
}

bool WiFiManager::removeNetwork(const String& ssid) {
    auto list = _loadNetworks();
    size_t before = list.size();
    list.erase(std::remove_if(list.begin(), list.end(),
        [&](const WifiCredential& c) { return c.ssid == ssid; }), list.end());
    if (list.size() == before) return false;   // SSID introuvable
    return _saveNetworks(list);
}

// Réseaux à essayer : NVS d'abord (portail web), valeurs de développement de secrets.h
// seulement si la NVS est vide. Partagé par begin() et par les nouvelles tentatives du portail.
static std::vector<WifiCredential> _collectNetworks() {
    std::vector<WifiCredential> networks = _loadNetworks();
#ifdef DEFAULT_WIFI_SSID
    if (networks.empty()) networks.push_back({DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD});
#endif
    return networks;
}

// ---------------------------------------------------------------------------
// Portail de configuration - page HTML servie en mode point d'accès
// Page volontairement autonome (CSS inline) : indépendante du pipeline
// minify_web.py, car servie avant toute connexion réseau.
// ---------------------------------------------------------------------------
static const char PORTAL_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="fr"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Gateway Lab - Configuration WiFi</title>
<style>
body{font-family:system-ui,sans-serif;background:#0f172a;color:#e2e8f0;display:flex;
  flex-direction:column;align-items:center;padding:2rem 1rem;margin:0}
.card{background:#1e293b;border-radius:12px;padding:1.5rem;max-width:380px;width:100%}
h1{font-size:1.05rem;margin:0 0 0.4rem}
p.hint{font-size:0.8rem;color:#94a3b8;margin-bottom:1rem}
label{display:block;margin:0.8rem 0 0.3rem;font-size:0.85rem;color:#94a3b8}
select,input{width:100%;padding:0.6rem;border-radius:6px;border:1px solid #334155;
  background:#0f172a;color:#e2e8f0;box-sizing:border-box;font-size:0.9rem}
button{width:100%;margin-top:1.2rem;padding:0.7rem;border:none;border-radius:6px;
  background:#3b82f6;color:#fff;font-weight:600;cursor:pointer;font-size:0.95rem}
button:hover{background:#2563eb}
#msg{margin-top:1rem;font-size:0.85rem;text-align:center;min-height:1.1rem}
</style></head><body>
<div class="card">
  <h1>Configuration WiFi - Gateway Lab</h1>
  <p class="hint">Choisissez votre réseau WiFi domestique. L'ESP32 redémarrera et s'y connectera automatiquement.</p>
  <form id="f">
    <label>Réseau WiFi détecté</label>
    <select id="ssidSel"><option value="">Recherche en cours...</option></select>
    <label>Ou saisissez le SSID manuellement</label>
    <input id="ssidManual" placeholder="Nom du réseau (SSID)">
    <label>Mot de passe</label>
    <input id="password" type="password" placeholder="Mot de passe WiFi">
    <button type="submit">Enregistrer et connecter</button>
  </form>
  <p id="msg"></p>
</div>
<script>
fetch('/scan').then(function(r){return r.json();}).then(function(list){
  var sel=document.getElementById('ssidSel');
  sel.innerHTML='<option value="">-- Choisir un réseau --</option>';
  list.forEach(function(n){
    var o=document.createElement('option');
    o.value=n.ssid; o.textContent=n.ssid+' ('+n.rssi+' dBm)';
    sel.appendChild(o);
  });
}).catch(function(){
  document.getElementById('ssidSel').innerHTML='<option value="">Aucun réseau détecté</option>';
});
document.getElementById('f').addEventListener('submit',function(e){
  e.preventDefault();
  var ssid=document.getElementById('ssidManual').value || document.getElementById('ssidSel').value;
  var password=document.getElementById('password').value;
  var msg=document.getElementById('msg');
  if(!ssid){ msg.textContent='Veuillez choisir ou saisir un réseau.'; return; }
  msg.textContent='Enregistrement...';
  var fd=new FormData(); fd.append('ssid',ssid); fd.append('password',password);
  fetch('/save',{method:'POST',body:fd}).then(function(r){return r.json();}).then(function(d){
    msg.textContent = d.status==='ok'
      ? 'Enregistré - redémarrage de l’ESP32...'
      : 'Erreur : '+(d.error||'inconnue');
  }).catch(function(){ msg.textContent='Erreur réseau.'; });
});
</script></body></html>
)HTML";

static void _portalHandleRoot() {
    _portal.send_P(200, "text/html", PORTAL_PAGE);
}

// Scan des réseaux visibles - utilisé par la page de configuration pour
// proposer une liste déroulante plutôt qu'une saisie manuelle uniquement
static void _portalHandleScan() {
    int n = WiFi.scanNetworks();
    // ArduinoJson échappe les guillemets, antislashs et caractères de contrôle d'un SSID :
    // la concaténation à la main produisait un JSON invalide (donc une liste vide) pour un
    // réseau nommé par exemple « Chez "Paul" ».
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = WiFi.SSID(i);
        o["rssi"] = WiFi.RSSI(i);
    }
    WiFi.scanDelete();   // libère les résultats du scan (RAM)
    String json;
    serializeJson(doc, json);
    _portal.send(200, "application/json", json);
}

// Enregistrement des identifiants saisis puis redémarrage - au prochain
// boot, begin() trouvera le réseau en NVS (priorité 1) et s'y connectera
static void _portalHandleSave() {
    // CSRF : une page web visitée par un client connecté au portail ne peut pas y écrire un réseau
    if (!reqguard::originAllowed(_portal.header("Origin").c_str(), _portal.hostHeader().c_str())) {
        _portal.send(403, "application/json", "{\"error\":\"cross_origin_forbidden\"}");
        return;
    }
    String ssid     = _portal.arg("ssid");
    String password = _portal.arg("password");
    if (ssid.isEmpty()) {
        _portal.send(400, "application/json", "{\"error\":\"ssid requis\"}");
        return;
    }
    wifiMgr.addNetwork(ssid, password);
    _portal.send(200, "application/json", "{\"status\":\"ok\"}");
    Log::i(TAG, "Réseau \"%s\" enregistré via le portail - redémarrage", ssid.c_str());
    delay(500);
    ESP.restart();
}

// Portail captif : toute requête vers un domaine inconnu est redirigée
// vers la page de configuration (déclenche le popup "Se connecter au réseau")
static void _portalHandleNotFound() {
    _portal.sendHeader("Location", String("http://") + AP_IP.toString(), true);
    _portal.send(302, "text/plain", "");
}

static void _startPortal() {
    _apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    delay(100);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));

    _dns.start(53, "*", AP_IP);   // résout tous les domaines vers le portail

    _portal.on("/",     HTTP_GET,  _portalHandleRoot);
    _portal.on("/scan", HTTP_GET,  _portalHandleScan);
    _portal.on("/save", HTTP_POST, _portalHandleSave);
    _portal.onNotFound(_portalHandleNotFound);
    static const char* kPortalHeaders[] = {"Origin"};
    _portal.collectHeaders(kPortalHeaders, 1);
    _portal.begin();

    _apRetryStep   = 0;
    _apNextRetryAt = millis() + AP_RETRY_DELAYS_MS[0];

    Log::i(TAG, "Portail de configuration actif - SSID \"%s\" - http://%s",
           AP_SSID, AP_IP.toString().c_str());
}

// ---------------------------------------------------------------------------
// mDNS
// ---------------------------------------------------------------------------
static void _startMdns() {
#ifdef ENABLE_MDNS
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Log::i(TAG, "mDNS actif : http://%s.local", MDNS_HOSTNAME);
    }
#endif
}

// ---------------------------------------------------------------------------
// Connexion STA - enregistre tous les réseaux fournis et attend jusqu'au
// timeout configuré ; WiFiMulti choisit celui au signal le plus fort
// ---------------------------------------------------------------------------
// WiFiMulti::addAP() n'évite pas les doublons : sans ce filtre, chaque nouvelle tentative du
// portail ajouterait une seconde fois les mêmes réseaux à la liste.
static void _addApOnce(const WifiCredential& c) {
    static std::vector<String> known;
    String key = c.ssid + '\n' + c.password;
    for (const auto& k : known) if (k == key) return;
    known.push_back(key);
    _multi.addAP(c.ssid.c_str(), c.password.c_str());
}

// keepPortal = true : tentative faite pendant que le portail est actif (mode AP+STA). Le
// portail et le DNS captif continuent d'être servis pendant l'attente au lieu d'être gelés.
static bool _tryConnect(const std::vector<WifiCredential>& networks, bool keepPortal = false) {
    WiFi.mode(keepPortal ? WIFI_AP_STA : WIFI_STA);
    for (const auto& c : networks) {
        _addApOnce(c);
    }

    Log::i(TAG, "Connexion en cours...");
    unsigned long start = millis();
    while (_multi.run() != WL_CONNECTED &&
           millis() - start < WIFI_CONNECT_TIMEOUT) {
        if (keepPortal) {
            // 10 x 50 ms = 500 ms, comme le delay(500) du mode normal, mais portail vivant
            for (int i = 0; i < 10; i++) {
                _dns.processNextRequest();
                _portal.handleClient();
                delay(50);
            }
        } else {
            delay(500);
        }
        Serial.print('.');
    }
    Serial.println();
    return WiFi.isConnected();
}

// Nouvelle tentative de connexion depuis le portail : bascule en mode station si un réseau
// connu est (re)apparu. Appelée par loop() en mode AP.
static void _portalRetryTick() {
    unsigned long now = millis();
    if ((long)(now - _apNextRetryAt) < 0) return;

    // Quelqu'un est connecté au portail : ne pas lui couper la page, repousser.
    if (WiFi.softAPgetStationNum() > 0) {
        _apNextRetryAt = now + AP_RETRY_BUSY_DEFER_MS;
        return;
    }

    std::vector<WifiCredential> networks = _collectNetworks();
    if (!networks.empty()) {
        Log::i(TAG, "Portail actif - nouvelle tentative de connexion WiFi...");
        if (_tryConnect(networks, true)) {
            Log::i(TAG, "Réseau retrouvé : \"%s\" - IP %s - fermeture du portail",
                   WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            _dns.stop();
            _portal.stop();
            WiFi.softAPdisconnect(true);   // coupe le point d'accès, garde la station
            WiFi.mode(WIFI_STA);
            _apMode = false;
            _wasConnected = true;
            _reconnectDelayMs = RECONNECT_DEBOUNCE_MS;
            _startMdns();
            // Mêmes services que lors d'une connexion réussie au boot (web, NTP, OTA...) :
            // leurs begin() sont idempotents, ils sont déjà rappelés à chaque reconnexion.
            if (_storedCb) _storedCb(true);
            return;
        }
        // Echec : on reste en portail seul (la tentative a pu laisser le mode AP+STA)
        WiFi.disconnect(false);
        WiFi.mode(WIFI_AP);
    }

    // Prochain essai plus tard : 2 min, 5 min, puis toutes les 10 min.
    if (_apRetryStep + 1 < AP_RETRY_STEPS) _apRetryStep++;
    _apNextRetryAt = millis() + AP_RETRY_DELAYS_MS[_apRetryStep];
}

void WiFiManager::begin(Callback cb) {
    _storedCb = cb;

    // Priorité 1 : réseaux enregistrés en NVS (configurés via le portail web).
    // Priorité 2 : valeurs de développement (include/secrets.h), seulement si aucun réseau
    // n'a encore été configuré par l'utilisateur.
    std::vector<WifiCredential> networks = _collectNetworks();
#ifdef DEFAULT_WIFI_SSID
    if (_loadNetworks().empty() && !networks.empty())
        Log::i(TAG, "Aucun réseau en NVS - utilisation de DEFAULT_WIFI_SSID (secrets.h)");
#endif

    if (!networks.empty() && _tryConnect(networks)) {
        Log::i(TAG, "Connecté à \"%s\" - IP %s - RSSI %d dBm",
               WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        _startMdns();
        _wasConnected = true;
        if (cb) cb(true);
        return;
    }

    // Priorité 3 : aucun réseau disponible - portail de configuration
    Log::w(TAG, "Aucun réseau disponible - démarrage du portail de configuration");
    if (cb) cb(false);
    _startPortal();
}

void WiFiManager::loop() {
    if (_apMode) {
        _dns.processNextRequest();
        _portal.handleClient();
        _portalRetryTick();   // le portail n'est pas définitif : retente le WiFi avec backoff
        return;
    }

    bool connected = WiFi.isConnected();

    if (!connected) {
        unsigned long now = millis();
        // Tentative de reconnexion seulement si le délai de debounce est écoulé
        if (now - _lastReconnectAttempt >= _reconnectDelayMs) {
            _lastReconnectAttempt = now;
            const unsigned long next = min(_reconnectDelayMs * 2, RECONNECT_MAX_MS);
            Log::w(TAG, "WiFi perdu - tentative de reconnexion (suivante dans %lu s)...",
                   next / 1000);
            _multi.run();   // Retourne immédiatement, la connexion est async
            _reconnectDelayMs = next;   // backoff progressif
        }
    } else if (!_wasConnected) {
        _reconnectDelayMs = RECONNECT_DEBOUNCE_MS;   // connexion rétablie : backoff remis à zéro
        // Transition déconnecté → connecté : relancer mDNS et tous les services
        Log::i(TAG, "WiFi rétabli sur \"%s\" - IP %s",
               WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        _startMdns();
        if (_storedCb) _storedCb(true);
    }

    _wasConnected = connected;
}

bool   WiFiManager::isConnected() const { return !_apMode && WiFi.isConnected(); }
bool   WiFiManager::isApMode()    const { return _apMode; }
String WiFiManager::ssid()        const { return WiFi.SSID(); }
String WiFiManager::localIP()     const {
    return _apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}
int8_t WiFiManager::rssi()        const { return (int8_t)WiFi.RSSI(); }
String WiFiManager::hostname()    const { return MDNS_HOSTNAME; }
