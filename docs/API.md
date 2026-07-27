# API REST - Gateway Lab

Toutes les routes sont servies **localement** par l'ESP32 (aucun cloud). Les
réponses sont au format JSON, sauf les exports (CSV) et les pages HTML.

Cette référence était auparavant intégrée au README ; elle vit désormais ici.

## GET /

Page d'accueil

## GET /scan

Inventaire réseau

## GET /history

Vue chronologique des événements détectés

## GET /wifi

Page Système : réseau WiFi (état de connexion, réseaux enregistrés), LED
d'état, mise à jour du firmware (OTA) et état système (mémoire, équipements,
historique).

## GET /topology

Page Topologie : vue simplifiée (passerelle/routeurs détectés vs reste des
équipements), à partir des données déjà collectées par le scan. Première
étape avant la cartographie graphique prévue en roadmap (v0.4.x).

## GET /api/status

Retourne :

```json
{
  "ssid": "...",
  "ip": "...",
  "rssi": -42,
  "uptime": "...",
  "version": "...",
  "hostname": "...",
  "scanning": false
}
```

## GET /api/devices

Retourne :

```json
{
  "scanning": false,
  "devices": [...]
}
```

## POST /api/scan

Déclenche un scan réseau asynchrone.

## POST /api/alias

Définit ou efface l'alias d'un équipement (paramètres `mac` et `alias`).

## POST /api/devices/reset

RAZ de l'inventaire des équipements connus. Paramètres optionnels
`keepAlias` et `keepManufacturer` (`1`/`0`) pour conserver les équipements
disposant d'un alias et/ou d'un fabricant identifié.

## POST /api/devices/rescan

Réinterroge un seul équipement (paramètre `ip`) sans relancer un scan
complet, et sans jamais relancer de découverte multicast réseau (aucun
SSDP, DNS-SD ou WS-Discovery global n'est lancé depuis cette route - ce
sont des protocoles qui sondent tout le sous-réseau et ne peuvent pas être
restreints à une IP). Sonde ARP/ICMP ciblée, puis résolution de nom ;
paramètre `mode` optionnel :
- `quick` (par défaut, 1-3s) : ARP/ICMP, PTR DNS, mise à jour du
  hostname, vérification de présence - rien d'autre.
- `deep` (<3s si rien d'exploitable, sinon quelques secondes) : scan TCP
  unicast des ports de la cible (`kRescanTargetPorts`). Si aucun
  port/service exploitable n'est trouvé, la passe s'arrête immédiatement.
  Sinon, le profil d'équipement (Computer, NAS, Printer, Streaming,
  SmartHome, Mobile, Unknown) est déduit des ports découverts et seuls les
  modules pertinents pour ce profil sont lancés, toujours en requête
  unicast directe sur l'IP visée : NetBIOS, API multimédia
  (Cast/Sonos/Roku/Samsung) et SNMP.

Exécuté de façon asynchrone sur une tâche FreeRTOS dédiée - voir
`GET /api/devices/rescan/status` pour suivre la progression. Retourne une
erreur 409 si un scan complet ou une autre passe précise est déjà en
cours, ou 400 si l'IP est inconnue.

## GET /api/devices/rescan/status

Retourne l'état courant de la passe précise en cours (à interroger toutes
les 500 ms par l'interface) :

```json
{
  "running": true,
  "ok": false,
  "ip": "192.168.1.42",
  "step": "Services multimédia",
  "percent": 70,
  "mode": "deep",
  "profile": "Streaming",
  "log": ["Modèle détecté : Google Nest Hub", "Confiance : 30% → 70%"]
}
```

`running` repasse à `false` une fois la passe terminée ; `ok` indique si
l'équipement a répondu. `mode` et `profile` indiquent la passe et le
profil déduit pour cette réinterrogation. `log` contient le journal
d'enrichissement de la dernière passe terminée (ou
`["Aucune information supplémentaire détectée"]` si rien de nouveau n'a
été trouvé).

## POST /api/favorite

Marque ou démarque un équipement comme favori (paramètres `mac` ou `ip`,
et `favorite` : `1` pour marquer, `0` pour démarquer).

## POST /api/notes

Ajoute une note libre datée à un équipement (paramètres `mac` ou `ip`, et
`text`). Le timestamp (`ts`, epoch NTP) est attribué côté serveur - `0` si
l'horloge n'est pas encore synchronisée.

## DELETE /api/notes

Supprime une note d'un équipement (paramètres `mac` ou `ip`, et `ts` -
le timestamp de la note à supprimer).

## GET /api/diagnostics

Retourne l'état mémoire/stockage, les temps de scan moyens et les compteurs
affichés sur la page Système :

```json
{
  "freeHeap": 184320,
  "freePsram": 7340032,
  "fsUsedBytes": 12480,
  "fsTotalBytes": 1474560,
  "lastScanMs": 4210,
  "avgScanMs": 3980,
  "lastRescanMs": 1850,
  "avgRescanMs": 1720,
  "degraded": false,
  "degradedReason": "",
  "deviceCount": 23,
  "maxDevices": 64,
  "historyCount": 84
}
```

## GET /api/history

Retourne le journal chronologique des événements (les plus récents en premier).

## DELETE /api/history

Vide le journal chronologique. L'interface web télécharge une sauvegarde
JSON du journal avant d'appeler cette route.

## GET /api/backup

Télécharge un export JSON complet de l'inventaire, des alias et de l'historique.

## POST /api/restore

Restaure l'inventaire depuis un export JSON précédemment généré par `/api/backup`.

## GET /api/devices/export.csv

Télécharge l'inventaire au format CSV (une ligne par équipement : IP, MAC,
hôte, alias, fabricant, modèle, catégorie, type, OS, services, ports
ouverts, en ligne (`Yes`/`No`), favori (`Yes`/`No`), niveau de confiance,
notes utilisateur, première/dernière apparition (date lisible
`AAAA-MM-JJ HH:MM:SS`), compteur de vues). Chaque équipement occupe
exactement une ligne physique : tout retour à la ligne présent dans une
valeur (note libre saisie par l'utilisateur, hostname...) est aplati en
espace avant export, pour rester lisible même par un tableur ou un script
qui ne respecte pas les guillemets RFC4180. Utile pour une exploitation dans
un tableur ou un script externe - pour une sauvegarde/restauration complète
(format JSON, dates en epoch), utiliser `/api/backup`. Le fichier contient un
BOM UTF-8 en tête pour un affichage correct des accents dans Excel.

## GET /api/system/backup

Télécharge une sauvegarde JSON des **paramètres de fonctionnement du
projet** - distincte de `/api/backup` (qui sauvegarde l'inventaire des
équipements) : réseaux WiFi enregistrés (SSID + mot de passe), luminosité
NeoPixel, état et fréquence de la surveillance automatique du réseau
(`monitorEnabled`, `monitorIntervalMinutes` - Patch 1), et nom mDNS à titre
informatif (fixé à la compilation via `MDNS_HOSTNAME`, non restaurable).
Accessible depuis la carte « Sauvegarde / Restauration » de la page Système.

## POST /api/system/restore

Restaure les paramètres de fonctionnement depuis un export JSON généré par
`/api/system/backup`. Les réseaux WiFi du fichier sont ajoutés ou mis à jour
(jamais supprimés automatiquement) ; la luminosité NeoPixel et l'état/fréquence
de la surveillance automatique sont appliqués immédiatement si présents dans
le fichier.

## GET /api/system/health

Retourne l'état du mode dégradé mémoire :

```json
{
  "degraded": false,
  "reason": "",
  "freeHeap": 184320
}
```

`degraded=true` quand le heap libre est tombé sous `HEAP_CRITICAL_BYTES` -
les nouveaux scans, rescans, notes, journalisation d'historique et
modifications de configuration sont alors refusés jusqu'à ce que la mémoire
se rétablisse (`HEAP_RECOVERY_MARGIN`) ou qu'un redémarrage manuel soit
effectué.

## POST /api/system/restart

Redémarre l'ESP32 immédiatement (déclenché manuellement depuis le bouton
« Redémarrer l'appareil » de la page Système - aucun redémarrage
automatique n'est déclenché par le firmware lui-même).

## POST /update

Upload d'un firmware `.bin` (formulaire intégré à la page Système, `/wifi`).

## GET /api/led/brightness

Retourne la luminosité courante de la NeoPixel d'état : `{"brightness": 15}`.

## POST /api/led/brightness

Définit la luminosité (paramètre `value`, 0-100), persistée en NVS.

## GET /api/wifi

Retourne :

```json
{
  "connected": true,
  "ssid": "...",
  "ip": "...",
  "rssi": -42,
  "networks": [{"ssid": "Maison"}, {"ssid": "Atelier"}]
}
```

Les mots de passe enregistrés ne sont jamais renvoyés au navigateur.

## POST /api/wifi

Ajoute ou met à jour un réseau enregistré (paramètres `ssid` et `password`).

## DELETE /api/wifi

Supprime un réseau enregistré (paramètre `ssid`).

## POST /api/mobility

Force ou efface la classification mobile/fixe d'un équipement (paramètres
`mac` ou `ip`, et `mode` : vide pour revenir à la détection automatique,
`fixed` ou `mobile` pour forcer).

## GET /api/network/health

Tableau de bord réseau de la surveillance continue : équipements
présents/connus, compteurs des dernières 24h (nouveaux équipements,
reconnexions, instabilités) et classement des équipements les moins
stables (score de stabilité, équipements mobiles exclus).

## GET /api/monitor

Retourne l'état courant de la surveillance continue :
`{"enabled": true, "intervalMinutes": 5}`. Réglable depuis la page Système
(case d'activation + sélecteur 5 min → 1 h, Patch 1).

## POST /api/monitor

Définit l'état de la surveillance continue : paramètres optionnels `enabled`
(`1`/`0`/`true`/`false`) et `minutes` (borné à 1-60 côté API ; l'interface
propose 5/10/15/30/60 min), persistés en NVS. Lorsque `enabled=false`, la
surveillance ne fait plus aucun tick (Patch 1). Quand elle est active, elle
se limite à un sweep ARP de détection de présence - aucun scan rapide ou
approfondi n'est plus déclenché automatiquement (Patch 2) ; pour une
identification complète, lancer un scan manuel (`/scan`) ou une passe
précise sur un équipement (`/api/devices/rescan`).

---

