#!/data/data/com.termux/files/usr/bin/bash
# Redemarre un appareil Android (TV, tablette...) via ADB.
# Action que l'ESP32 ne peut pas faire lui-meme (pas d'acces ADB natif) —
# c'est le premier exemple d'"action specifique a un device type" (V3 du roadmap).
#
# Pre-requis Termux : pkg install android-tools
# L'appareil cible doit avoir le debogage ADB reseau active.
#
# Usage : ./reboot_android.sh <ip> [port]

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

echo "Redemarrage de $TARGET..."
adb -s "$TARGET" reboot

echo "OK — l'appareil redemarre."
