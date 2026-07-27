# Fonctionnalités - Gateway Lab

Liste **exhaustive** des fonctionnalités actuelles. Le README n'en présente qu'un
résumé ; le détail complet vit ici.

| Fonctionnalité           | Détail                                                            |
| ------------------------ | ----------------------------------------------------------------- |
| WiFi multi-réseaux       | Connexion automatique au réseau enregistré au meilleur signal     |
| Portail de configuration | Point d'accès `GatewayLab-Setup` + page web si aucun réseau n'est connu |
| Persistance NVS          | Réseaux WiFi enregistrés survivant aux redémarrages/coupures      |
| mDNS                     | Accessible via `gatewaylab.local`                             |
| Interface web            | Pages Accueil / Équipements / Historique / Topologie / Système (réseau WiFi, OTA, état système) |
| Scan réseau LAN          | Sweep ARP du sous-réseau local                                    |
| Tâche FreeRTOS dédiée    | Scan asynchrone sur Core 0                                        |
| Résolution hostnames     | mDNS passif + DNS inverse PTR                                     |
| Détection box FAI        | Orange, Free, SFR, Bouygues                                       |
| Découverte SSDP/UPnP     | M-SEARCH multicast, parsing XML, catégorisation automatique       |
| API Philips Hue Bridge   | Modèle, firmware, sans authentification (`/api/config`)           |
| API Synology DSM         | Confirmation NAS, modèle depuis XML UPnP                          |
| API Freebox              | Modèle exact (Ultra/Pop/Révolution), version FreeboxOS            |
| Découverte DNS-SD        | 31 types de services, badges HTTP/SSH/AirPlay/Cast/Sonos/HomeKit  |
| Découverte NetBIOS       | Node Status (UDP 137) - hostnames PC Windows / Samba              |
| Scan de ports TCP        | 14 ports, bannières HTTP/SSH/FTP, détection API IoT (Shelly/Tasmota/FritzBox) |
| Inventaire réseau        | IP, nom, fabricant, modèle, catégorie, OS, services, ports, MAC, source |
| Nom humain du matériel   | Intitulé déduit (modèle/fabricant/catégorie) affiché au-dessus du hostname brut quand celui-ci reste générique, repris dans l'export JSON (`hostnameDisplay`) - l'export CSV garde le hostname brut sur une seule ligne |
| Auto-détection ESP32     | Le Gateway apparaît dans sa propre liste                          |
| Identification OUI       | Base externalisée générée automatiquement                         |
| Persistance LittleFS     | Statistiques online/offline, état conservé entre redémarrages     |
| Historique des équipements | Synchronisation NTP, firstSeen/lastSeen/seenCount, journal chronologique (`/history`) |
| Alias utilisateur        | Renommage manuel d'un équipement, prioritaire sur le hostname     |
| Classification intelligente | Affine la catégorie d'un équipement à partir de plusieurs signaux |
| Niveau de confiance      | Score 0-100% expliquant la fiabilité de l'identification, par source |
| Détection des changements | Comparaison automatique entre deux scans (IP, fabricant, catégorie, ports...) |
| Sauvegarde / restauration | Export et import JSON complet de l'inventaire et de l'historique |
| Réinitialisation des équipements | RAZ de l'inventaire, avec options de conservation (alias, fabricant connu) |
| Réinterrogation ciblée   | Rafraîchit un seul équipement (IP) sans relancer un scan complet, en deux passes au choix - rapide (1-3s, ARP/ICMP + PTR DNS) ou approfondie (<3s si rien d'exploitable, sinon quelques secondes) - qui n'interrogent jamais que l'IP visée (aucun SSDP, DNS-SD ou WS-Discovery global) ; tâche asynchrone avec barre de progression et journal d'enrichissement affichés sous la ligne de l'équipement |
| Sous-catégorie (`type`)  | Précision du type d'équipement au sein d'une catégorie (ex: Caméra, Imprimante) |
| Score de confiance unique | Calcul prudent (minimum des signaux fabricant/catégorie), avec détail par champ au survol |
| Sondage SNMP             | `sysDescr` (UDP 161) en requête unicast ciblée, lors de la passe précise approfondie (profil Imprimante/Inconnu avec service exploitable) - fabricant/modèle en texte clair |
| API appareils multimédia | Cast, Sonos, Roku, Samsung Smart TV - sondées en requête unicast ciblée, lors de la passe précise approfondie (profil Streaming/Domotique avec service exploitable) |
| Sondage broker MQTT      | CONNECT anonyme + topics `$SYS/broker/version`/`clients/connected`, requête TCP unicast ciblée, lors de la passe précise approfondie (profil Domotique/Inconnu avec port 1883 ouvert) |
| Fingerprinting DHCP passif | Écoute continue des paquets DHCPDISCOVER/REQUEST (UDP 67) des autres équipements - hostname (option 12) et OS déclaré via vendor class (option 60), sans jamais émettre de requête ni ajouter de coût au scan |
| Découverte Matter (DNS-SD) | Détection des appareils Matter commissionnables (`_matterc._udp`) |
| Filtrage de l'historique | Filtres par type d'événement (nouveau, reconnexion, déconnexion, changement) et par favoris uniquement |
| Effacement de l'historique | Vide le journal après téléchargement automatique d'une sauvegarde JSON |
| OTA Web                  | Upload firmware depuis le navigateur                              |
| ArduinoOTA               | Mise à jour réseau depuis PlatformIO                              |
| Favoris et notes d'inventaire | Marquer un équipement en favori et lui attacher des notes libres datées (entretien, firmware, etc.) |
| Cartouche diagnostics    | Heap libre, PSRAM libre, espace LittleFS, temps moyen d'un scan / d'une passe précise - affichée sur la page Accueil |
| NeoPixel d'état          | Bleu pulsé (démarrage), bleu fixe (prêt), vert clignotant (scan), jaune clignotant (nouvel équipement), violet (portail WiFi), cyan (sauvegarde) - luminosité réglable depuis la page Système, persistée |
| Bouton BOOT               | Appui court = lance un scan, maintien 3 s = sauvegarde immédiate |
| Filtres équipements      | Filtre par type, fabricant, Favoris, En Ligne, Nouveau (détecté depuis moins de 24h) (page Équipements, côté client) |
| Menu Données              | Sur la page Équipements : Export CSV / Export JSON (inventaire) uniquement - la Sauvegarde / Restauration des paramètres de fonctionnement a été déplacée sur la page Système (Patch 1) |
| Sauvegarde des paramètres | Réseaux WiFi enregistrés, luminosité NeoPixel, état et fréquence de la surveillance automatique (`/api/system/backup`, `/api/system/restore`) - distincte de la sauvegarde de l'inventaire, accessible depuis la page Système (Patch 1) |
| Page Topologie           | Vue simplifiée (passerelle/routeurs vs reste des équipements), première étape avant la cartographie graphique (roadmap v0.4.x) |
| Mode dégradé mémoire     | Heap critique (< `HEAP_CRITICAL_BYTES`) → refuse scans/rescans/notes/historique/config sans redémarrer ; inventaire déjà acquis consultable ; redémarrage manuel depuis la page Système |
| Bornes mémoire           | Listes/historique/notes plafonnés (`MAX_TRACKED_DEVICES`, `MAX_HISTORY_EVENTS`, `MAX_NOTES_PER_DEVICE`, `MAX_NOTE_LENGTH`) pour garantir un fonctionnement stable sur la durée |
| Surveillance continue    | Sweep ARP léger périodique (`serviceMonitor()`), activable/désactivable et fréquence configurable de 5 min à 1 h depuis la page Système (persisté NVS, Patch 1) - détection de présence uniquement : aucun scan rapide/approfondi automatique, aucune découverte SSDP/DNS-SD/SNMP lancée hors scan complet ou rescan manuel (Patch 2) |
| Score de stabilité       | Compteurs de présence/absence/reconnexion par équipement, score 0-100% pour les équipements fixes (équipements mobiles non pénalisés), évènements d'historique (`reconnected`, `disappeared`, `identification_improved`, `mobile_left`, `mobile_returned`, `offline_brief`) |
| Historique sans bruit (Patch 1.1.1) | Absences courtes (<30 min) d'un mobile journalisées (`offline_brief`) en plus du `reconnected` qui suit ; chaînes de reconnexions sans déconnexion explicite regroupées sur la page Historique en une seule entrée « Connexion instable détectée » |
| Classification mobile/fixe | Détection automatique par catégorie/type, override manuel via `POST /api/mobility` |
| Tableau de bord réseau   | `GET /api/network/health` : équipements présents/connus, nouveautés/reconnexions/instabilités des dernières 24h, classement des équipements les moins stables |
| API REST                 | `/api/status`, `/api/devices`, `/api/devices/reset`, `/api/devices/rescan`, `/api/devices/rescan/status`, `/api/scan`, `/api/alias`, `/api/favorite`, `/api/notes`, `/api/diagnostics`, `/api/history`, `/api/backup`, `/api/restore`, `/api/devices/export.csv`, `/api/system/backup`, `/api/system/restore`, `/api/system/health`, `/api/system/restart`, `/api/wifi`, `/api/led/brightness`, `/api/mobility`, `/api/network/health`, `/api/monitor` |

---

