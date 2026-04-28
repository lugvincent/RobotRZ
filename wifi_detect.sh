#!/data/data/com.termux/files/usr/bin/bash
# ============================================================
# Script : wifi_detect.sh
# Objectif : détecter les réseaux Wi-Fi disponibles et se
#            connecter automatiquement selon priorité.
# Auteur : RZ + GPT
# ============================================================

# === Chargement des variables d'environnement ===
source ~/.env

# === Fichiers de sortie ===
CONTEXT_FILE="$HOME/Scripts_RZ_Local/network_context.txt"
LOG_DIR="$HOME/logs"
LOG_FILE="$LOG_DIR/wifi_detect.log"
mkdir -p "$LOG_DIR"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

write_context() {
    echo "CONTEXT=$1" > "$CONTEXT_FILE"
    echo "SSID=$2" >> "$CONTEXT_FILE"
    echo "STATUS=$3" >> "$CONTEXT_FILE"
}

log "===== 🔍 DÉMARRAGE wifi_detect.sh ====="

# --- Activation du Wi-Fi ---
termux-wifi-enable true
sleep 4

# --- Scan des réseaux ---
log "Scan des réseaux Wi-Fi disponibles..."
SCAN_OUTPUT=$(termux-wifi-scaninfo 2>/dev/null)
if [ -z "$SCAN_OUTPUT" ]; then
    log "❌ Impossible de scanner les réseaux Wi-Fi (permissions ou Wi-Fi désactivé)."
    write_context "aucun_wifi" "none" "scan_failed"
    exit 1
fi

# --- Extraction des SSID détectés ---
SSID_LIST=$(echo "$SCAN_OUTPUT" | grep '"ssid"' | cut -d '"' -f4)
log "SSID détectés : $SSID_LIST"

# --- Vérification des priorités ---
TARGET_SSID=""
TARGET_PWD=""

if echo "$SSID_LIST" | grep -q "$HOME_SSID"; then
    TARGET_SSID="$HOME_SSID"
    TARGET_PWD="$HOME_PASSWORD"
    CONTEXT="maison"
elif echo "$SSID_LIST" | grep -q "$XIAOMI_SSID"; then
    TARGET_SSID="$XIAOMI_SSID"
    TARGET_PWD="$XIAOMI_PASSWORD"
    CONTEXT="hotspot_SP"
else
    # Aucun réseau prioritaire trouvé
    log "⚠️ Aucun réseau prioritaire trouvé (Livebox ou SP non détecté)."
    write_context "aucun_wifi" "none" "no_match"
    exit 1
fi

log "📡 Réseau prioritaire trouvé : $TARGET_SSID ($CONTEXT)"
log "Tentative de connexion..."

# --- Connexion au réseau ---
termux-wifi-connectioninfo | grep -q "$TARGET_SSID"
if [ $? -ne 0 ]; then
    # Connexion via wpa_cli
    WPA_CONF="/data/data/com.termux/files/usr/etc/wpa_supplicant/wpa_supplicant.conf"

    if ! grep -q "$TARGET_SSID" "$WPA_CONF"; then
        log "Ajout de $TARGET_SSID dans la configuration Wi-Fi..."
        {
            echo "network={"
            echo "    ssid=\"$TARGET_SSID\""
            echo "    psk=\"$TARGET_PWD\""
            echo "}"
        } >> "$WPA_CONF"
    fi

    # Tentative de connexion via wpa_cli
    wpa_cli -i wlan0 disconnect >/dev/null
    wpa_cli -i wlan0 select_network 0 >/dev/null
    wpa_cli -i wlan0 reconnect >/dev/null
    sleep 6
fi

# --- Vérification IP ---
IP_ADDR=$(ip addr show wlan0 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
if [ -n "$IP_ADDR" ]; then
    log "✅ Connecté à $TARGET_SSID - IP locale : $IP_ADDR"
    write_context "$CONTEXT" "$TARGET_SSID" "connected"
else
    log "❌ Échec de la connexion à $TARGET_SSID"
    write_context "$CONTEXT" "$TARGET_SSID" "connection_failed"
    exit 1
fi

log "===== ✅ Fin du script wifi_detect.sh ====="
exit 0
