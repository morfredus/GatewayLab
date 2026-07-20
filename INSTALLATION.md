# Installation de Gateway Lab

Ce guide explique comment installer et configurer Gateway Lab sur un ESP32-S3.

## Prérequis

### Matériel

* ESP32-S3
* 16 Mo de Flash recommandés
* 8 Mo de PSRAM recommandés
* Câble USB de données

### Logiciel

* [PlatformIO](https://platformio.org/) (extension VS Code ou `platformio-core`)
* Git

Aucun firmware précompilé n'est distribué pour le moment : Gateway Lab se
compile depuis les sources. Le guide développeur détaille l'environnement et le
dépannage : [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Compiler et flasher le firmware

### 1. Récupérer les sources

```bash
git clone git@github.com:morfredus/GatewayLab.git
cd GatewayLab
```

### 2. Connecter l'ESP32-S3

Branchez l'ESP32-S3 à l'ordinateur avec un câble USB **de données** (certains
câbles ne transportent que l'alimentation et l'appareil n'apparaîtra pas).

### 3. Compiler et téléverser

```bash
pio run --target upload
```

PlatformIO télécharge la chaîne de compilation au premier lancement, ce qui
peut prendre plusieurs minutes. L'environnement `esp32s3_n16r8` est
sélectionné automatiquement.

Si la carte n'est pas détectée, préciser le port :

```bash
pio run --target upload --upload-port COM5          # Windows
pio run --target upload --upload-port /dev/ttyUSB0  # Linux/macOS
```

Si le téléversement échoue malgré tout, maintenir le bouton **BOOT** pendant la
connexion pour entrer en mode flash, puis relancer la commande.

### 4. Suivre le démarrage

```bash
pio device monitor
```

L'ESP32 redémarre automatiquement à la fin du téléversement.

## Première configuration

Au premier démarrage, Gateway Lab crée automatiquement un point d'accès WiFi :

```text
GatewayLab-Setup
```

Connectez-vous à ce réseau depuis votre ordinateur, tablette ou smartphone.

Une page de configuration devrait s'ouvrir automatiquement.

Si ce n'est pas le cas, ouvrez :

```text
http://192.168.4.1
```

## Configuration du WiFi

Saisissez :

* le nom du réseau WiFi (SSID)
* le mot de passe

Cliquez sur :

```text
Enregistrer
```

Gateway Lab redémarre automatiquement.

## Accéder à Gateway Lab

Une fois connecté à votre réseau local :

```text
http://gatewaylab.local
```

Si votre réseau ne prend pas en charge mDNS, utilisez l'adresse IP affichée par votre routeur ou votre box Internet.

## Réseaux WiFi enregistrés

Gateway Lab peut mémoriser plusieurs réseaux WiFi.

Les réseaux enregistrés sont conservés en mémoire même après :

* un redémarrage
* une coupure de courant
* une mise à jour OTA

Ils peuvent être gérés depuis :

```text
Paramètres → Réseau WiFi
```

## Mise à jour du firmware

Les versions suivantes peuvent être installées directement depuis l'interface Web grâce à la fonction OTA.

Aucune connexion USB n'est nécessaire après l'installation initiale.

## Dépannage

### Impossible d'accéder à Gateway Lab

Vérifiez :

* que l'ESP32 est connecté au WiFi
* que votre appareil est sur le même réseau
* que mDNS est supporté par votre système

Essayez également l'adresse IP attribuée par votre routeur.

### Le portail de configuration n'apparaît pas

Connectez-vous manuellement au réseau :

```text
GatewayLab-Setup
```

Puis ouvrez :

```text
http://192.168.4.1
```

## Support

Pour signaler un bug ou proposer une amélioration :

* Ouvrez une issue GitHub
* Consultez le README et la documentation du projet
