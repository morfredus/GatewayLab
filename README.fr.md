# Gateway Lab

*Lire dans une autre langue : [English](README.md) · **Français** (ce document).*

![Version](https://img.shields.io/badge/version-1.11.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/framework-Arduino%20%2F%20PlatformIO-00979D)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-en%20d%C3%A9veloppement-yellow)

*Passerelle autonome d'inventaire et de découverte réseau, propulsée par ESP32-S3.*

Gateway Lab est une passerelle réseau autonome qui découvre, identifie et conserve
l'historique des équipements présents sur un réseau local domestique.

Conçu autour d'un ESP32-S3, le projet privilégie la simplicité de déploiement,
l'autonomie, la faible consommation et la conservation locale des données.

---

## Principales fonctionnalités

- Découverte multi-protocoles (ARP, ICMP, mDNS, SSDP, DNS-SD, NetBIOS...)
- Inventaire persistant des équipements détectés
- Classification automatique (fabricant, catégorie, type)
- Historique des connexions, déconnexions et changements
- Favoris et notes utilisateur
- Surveillance continue du réseau et score de stabilité par équipement
- Export CSV et JSON, sauvegarde et restauration
- Interface web responsive, mise à jour OTA
- Fonctionnement autonome, sans cloud

> Liste **exhaustive** des fonctionnalités : voir **[docs/FEATURES.md](docs/FEATURES.md)**.

---

## Aperçu de l'interface

### Accueil
Informations réseau, état de connexion, diagnostics système et accès rapide aux principales fonctions.

![Accueil](docs/pictures/Gateway_Lab_Accueil.png)

### Équipements
Inventaire des appareils détectés avec filtres, favoris, notes, niveau de confiance et outils d'administration.

![Équipements](docs/pictures/Gateway_Lab_Equipements.png)

### Historique
Journal chronologique des nouveaux équipements, reconnexions, déconnexions et changements détectés.

![Historique](docs/pictures/Gateway_Lab_Historique.png)

### Topologie
Vue simplifiée de la passerelle, des points d'accès détectés et des équipements rattachés.

![Topologie](docs/pictures/Gateway_Lab_Topologie.png)

### Système
Configuration WiFi, surveillance automatique, NeoPixel d'état, sauvegarde/restauration, OTA et état système.

![Système](docs/pictures/Gateway_Lab_Systeme.png)

---

## Sécuriser la carte (jeton optionnel)

Par défaut, la carte accepte les mises à jour du firmware et les changements de Wi-Fi de n'importe quel
poste du réseau local. Pour les protéger, ajouter cette ligne à `include/secrets.h` (jamais versionné) :

```cpp
#define GATEWAYLAB_AUTH_TOKEN "une-longue-chaine-aleatoire"
```

Avec le jeton défini :

- la mise à jour web du firmware (`/update`), la modification des réseaux Wi-Fi, la restauration des
  paramètres, la restauration de l'inventaire et le redémarrage exigent l'en-tête `X-Auth-Token`
  (l'interface web demande le jeton une fois par onglet) ;
- ArduinoOTA utilise la même chaîne comme mot de passe (ajouter `--auth=une-longue-chaine-aleatoire`
  aux `upload_flags` de l'environnement PlatformIO pour les envois sans fil) ; un
  `GATEWAYLAB_OTA_PASSWORD` distinct peut le remplacer pour ArduinoOTA seulement ;
- la sauvegarde des paramètres contient les mots de passe Wi-Fi uniquement sur demande (`?full=1`)
  avec un jeton valide.

Sans jeton, le comportement précédent est conservé, un avertissement est écrit dans le journal à chaque
démarrage, la sauvegarde des paramètres ne liste que les SSID (aucun mot de passe) et la restauration des
paramètres ignore les réseaux Wi-Fi. Dans tous les cas, toute requête d'écriture (POST, PUT, PATCH, DELETE)
dont l'en-tête `Origin` diffère de l'en-tête `Host` est refusée avec `403 cross_origin_forbidden` : une page
web visitée depuis le LAN ne peut pas agir sur la carte. Les scripts et outils qui n'envoient pas
d'en-tête `Origin` ne sont pas concernés.

---

## Installation rapide

1. Cloner le dépôt et compiler avec PlatformIO (`pio run --target upload`)
2. Se connecter au réseau WiFi `GatewayLab-Setup`
3. Configurer le WiFi depuis le navigateur
4. Accéder à Gateway Lab via `http://gatewaylab.local`

Guide détaillé : [INSTALLATION.md](INSTALLATION.md) · Guide développeur : [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)

---

## Matériel cible

**ESP32-S3 DevKitC-1 N16R8** - 16 Mo Flash, 8 Mo PSRAM, dual core 240 MHz.

---

## Documentation

| Fichier | Description |
|---|---|
| [INSTALLATION.md](INSTALLATION.md) | Installation et première configuration |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Compilation, flash et développement |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Architecture interne, structure du projet, pipeline web |
| [docs/API.md](docs/API.md) | Référence complète de l'API REST |
| [docs/FEATURES.md](docs/FEATURES.md) | Liste exhaustive des fonctionnalités |
| [docs/PROTOCOLS.md](docs/PROTOCOLS.md) | Protocoles réseau et sources d'identification |
| [docs/WIFI_SETUP.md](docs/WIFI_SETUP.md) | Fonctionnement détaillé du portail WiFi |
| [docs/WARNINGS.md](docs/WARNINGS.md) | Décisions d'architecture et limitations connues |
| [CHANGELOG.md](CHANGELOG.md) | Historique des versions |
| [ROADMAP.md](ROADMAP.md) | Évolutions prévues |

---

## Contraintes de développement

- `include/board_config.h` - ne pas modifier
- `include/secrets.h` - ne jamais committer
- CSS uniquement dans `web_src/styles.css`
- HTML uniquement dans `web_src/*.html` (jamais de `<style>` ou `<script>` inline)
- JavaScript uniquement dans `web_src/*.js`
- Versioning uniquement dans `platformio.ini` via `PROJECT_VERSION`
- Après toute modification de `web_src/` ou `data/oui.json` → il suffit de compiler : `pio run` régénère les en-têtes (ignorés par Git)

Détail du pipeline web et de la structure du projet : [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## Licence

Projet personnel open source publié à des fins d'apprentissage, d'expérimentation
et de partage de connaissances autour de l'ESP32, du réseau et des systèmes
embarqués.
