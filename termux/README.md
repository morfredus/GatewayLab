# Termux — Pixel comme second nœud actif

Scripts à exécuter sur le Pixel 8 Pro (rooté) via Termux, pour des actions que
l'ESP32 ne peut pas faire lui-même (ADB, SSH, scans plus profonds...). Les
résultats sont renvoyés à l'ESP32 via son API HTTP (`/api/enrich`), qui les
fusionne dans l'inventaire existant.

## Installation

```sh
pkg install android-tools curl
cp config.sh.example config.sh
# éditer config.sh : ESP32_HOST=gateway-lab-v1.local (ou IP de l'ESP32)
chmod +x *.sh
```

## Scripts disponibles

- **`enrich_android.sh <ip> [port]`** — Se connecte en ADB réseau à un
  appareil Android (TV, tablette...), lit modèle/fabricant/version OS/MAC,
  puis les envoie à l'ESP32 via `POST /api/enrich`.
- **`reboot_android.sh <ip> [port]`** — Redémarre un appareil Android via ADB.
  Premier exemple d'action « spécifique à un type d'appareil » (catégorie V3
  du roadmap), complémentaire des actions génériques déjà gérées par l'ESP32
  (Wake-on-LAN, ouverture d'interface web).

Prérequis sur l'appareil cible : débogage ADB réseau activé (Paramètres >
Options pour les développeurs > Débogage sans fil / débogage réseau, port
5555 par défaut).

## API ESP32 utilisée

`POST /api/enrich` — corps JSON, ex :

```json
{"ip":"192.168.1.50","mac":"AA:BB:CC:DD:EE:FF","os":"Android 13",
 "model":"Chromecast avec Google TV","manufacturer":"Google","category":"AndroidTV"}
```

`mac` ou `ip` doit identifier un équipement déjà connu de l'ESP32 (détecté
lors d'un scan précédent). Les champs `services`/`openPorts` (chaînes
pipe-séparées) sont fusionnés sans doublon ; les autres champs remplacent la
valeur existante.
