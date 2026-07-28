# Gateway Lab

*Read in another language: **English** (this document) · [Français](README.fr.md).*

![Version](https://img.shields.io/badge/version-1.9.3-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange)
![Framework](https://img.shields.io/badge/framework-Arduino%20%2F%20PlatformIO-00979D)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-in%20development-yellow)

*Autonomous network inventory and discovery appliance powered by ESP32-S3.*

Gateway Lab is an autonomous network gateway that discovers, identifies and keeps
the history of the devices present on a home local network.

Built around an ESP32-S3, the project favours simplicity of deployment, autonomy,
low power consumption and local data retention.

---

## Key features

- Multi-protocol discovery (ARP, ICMP, mDNS, SSDP, DNS-SD, NetBIOS...)
- Persistent inventory of detected devices
- Automatic classification (manufacturer, category, type)
- History of connections, disconnections and changes
- Favourites and user notes
- Continuous network monitoring and a per-device stability score
- CSV and JSON export, backup and restore
- Responsive web interface, OTA updates
- Fully local operation, no cloud

> **Exhaustive** feature list: see **[docs/FEATURES.md](docs/FEATURES.md)**.

---

## Interface preview

### Home
Network information, connection state, system diagnostics and quick access to the main functions.

![Home](docs/pictures/Gateway_Lab_Accueil.png)

### Devices
Inventory of detected devices with filters, favourites, notes, confidence level and administration tools.

![Devices](docs/pictures/Gateway_Lab_Equipements.png)

### History
Chronological log of new devices, reconnections, disconnections and detected changes.

![History](docs/pictures/Gateway_Lab_Historique.png)

### Topology
Simplified view of the gateway, the detected access points and the attached devices.

![Topology](docs/pictures/Gateway_Lab_Topologie.png)

### System
WiFi configuration, automatic monitoring, status NeoPixel, backup/restore, OTA and system state.

![System](docs/pictures/Gateway_Lab_Systeme.png)

---

## Quick start

1. Clone the repository and build with PlatformIO (`pio run --target upload`)
2. Connect to the `GatewayLab-Setup` WiFi network
3. Configure WiFi from the browser
4. Reach Gateway Lab at `http://gatewaylab.local`

Detailed guide: [INSTALLATION.md](INSTALLATION.md) · Developer guide: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)

---

## Target hardware

**ESP32-S3 DevKitC-1 N16R8** — 16 MB Flash, 8 MB PSRAM, dual-core 240 MHz.

---

## Documentation

| File | Description |
|---|---|
| [INSTALLATION.md](INSTALLATION.md) | Installation and first configuration |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Build, flash and development |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Internal architecture, project structure, web pipeline |
| [docs/API.md](docs/API.md) | Full REST API reference |
| [docs/FEATURES.md](docs/FEATURES.md) | Exhaustive feature list |
| [docs/PROTOCOLS.md](docs/PROTOCOLS.md) | Network protocols and identification sources |
| [docs/WIFI_SETUP.md](docs/WIFI_SETUP.md) | Detailed WiFi portal operation |
| [docs/WARNINGS.md](docs/WARNINGS.md) | Architecture decisions and known limitations |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [ROADMAP.md](ROADMAP.md) | Planned evolutions |

---

## Development constraints

- `include/board_config.h` — do not modify
- `include/secrets.h` — never commit
- CSS only in `web_src/styles.css`
- HTML only in `web_src/*.html` (never an inline `<style>` or `<script>`)
- JavaScript only in `web_src/*.js`
- Versioning only in `platformio.ini` via `PROJECT_VERSION`
- After any change to `web_src/` or `data/oui.json` → re-run `python tools/minify_web.py`

Full web pipeline and project structure: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## License

Open-source personal project published for learning, experimentation and knowledge
sharing around the ESP32, networking and embedded systems.
