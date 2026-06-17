#!/data/data/com.termux/files/usr/bin/bash
# Fonctions partagees par les scripts termux/*.sh
# A sourcer en tete de chaque script : source "$(dirname "$0")/lib_common.sh"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -f "$SCRIPT_DIR/config.sh" ]; then
    source "$SCRIPT_DIR/config.sh"
else
    ESP32_HOST="${ESP32_HOST:-gateway-lab-v1.local}"
fi

# Envoie un enrichissement a l'ESP32 (POST /api/enrich)
# Usage : enrich_send "<ip>" "<mac>" "<os>" "<model>" "<manufacturer>" "<category>"
# Les champs vides ne sont pas inclus dans le JSON envoye.
enrich_send() {
    local ip="$1" mac="$2" os="$3" model="$4" manufacturer="$5" category="$6"
    local payload="{"
    local first=1
    _add() {
        local key="$1" val="$2"
        [ -z "$val" ] && return
        [ $first -eq 0 ] && payload+=","
        payload+="\"$key\":\"$val\""
        first=0
    }
    [ -n "$ip" ]  && { payload+="\"ip\":\"$ip\""; first=0; }
    _add "mac" "$mac"
    _add "os" "$os"
    _add "model" "$model"
    _add "manufacturer" "$manufacturer"
    _add "category" "$category"
    payload+="}"

    echo "POST http://$ESP32_HOST/api/enrich  $payload"
    curl -sS -X POST "http://$ESP32_HOST/api/enrich" \
         -H "Content-Type: application/json" \
         -d "$payload"
    echo
}

# Verifie qu'adb est disponible (paquet Termux : android-tools)
require_adb() {
    if ! command -v adb >/dev/null 2>&1; then
        echo "adb introuvable — installer le paquet : pkg install android-tools" >&2
        exit 1
    fi
}
