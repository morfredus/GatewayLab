#!/data/data/com.termux/files/usr/bin/bash
# Identifie un appareil Android (TV, tablette, telephone debogable) via ADB
# et envoie le resultat a l'ESP32 (Gateway Lab) via /api/enrich.
#
# Pre-requis Termux : pkg install android-tools curl
# L'appareil cible doit avoir le debogage ADB reseau active
# (Parametres > Options dev. > Debogage ADB sans fil / sur le reseau, port 5555).
#
# Usage : ./enrich_android.sh <ip> [port]

set -euo pipefail
source "$(dirname "$0")/lib_common.sh"
require_adb

IP="${1:-}"
PORT="${2:-5555}"
if [ -z "$IP" ]; then
    echo "Usage : $0 <ip> [port]" >&2
    exit 1
fi

TARGET="$IP:$PORT"
echo "Connexion ADB a $TARGET..."
adb connect "$TARGET" >/dev/null

_prop() { adb -s "$TARGET" shell getprop "$1" 2>/dev/null | tr -d '\r'; }

MANUFACTURER="$(_prop ro.product.manufacturer)"
MODEL="$(_prop ro.product.model)"
RELEASE="$(_prop ro.build.version.release)"
MAC="$(adb -s "$TARGET" shell cat /sys/class/net/wlan0/address 2>/dev/null | tr -d '\r' | tr 'a-f' 'A-F')"

# Categorie : Android TV se distingue par sa caracteristique systeme dediee
IS_TV="$(_prop ro.build.characteristics)"
CATEGORY="Mobile"
case "$IS_TV" in
    *tv*) CATEGORY="AndroidTV" ;;
esac

OS="Android $RELEASE"

echo "Manufacturer : $MANUFACTURER"
echo "Modele       : $MODEL"
echo "OS           : $OS"
echo "MAC          : ${MAC:-inconnue}"
echo "Categorie    : $CATEGORY"

enrich_send "$IP" "$MAC" "$OS" "$MODEL" "$MANUFACTURER" "$CATEGORY"

adb disconnect "$TARGET" >/dev/null
