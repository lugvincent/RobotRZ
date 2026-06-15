#!/bin/bash
# =============================================================================
# SCRIPT  : rz_tasker_out.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/core/rz_tasker_out.sh
#
# OBJECTIF
# --------
# Surveille vpiv_out.txt (écrit par Tasker) et publie son contenu
# vers SP via MQTT sur le topic SE/statut.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# - Utilise inotifywait pour détecter les modifications de vpiv_out.txt
# - Filtre les trames vides et les doublons
# - Publie chaque nouvelle trame vers SE/statut
#
# PIPELINE
# --------
#   Tasker (RZ_MenuCmd, RZ_AppStop...) → vpiv_out.txt
#   → inotifywait détecte la modification
#   → mosquitto_pub → SE/statut → Node-RED SP
#   → parseVPIV : CAT=V → Gtion_EtatGlobal (validation intent)
#                 CAT=I → Gtion_EtatGlobal (ACK)
#
# FORMATS ATTENDUS dans vpiv_out.txt
# -----------------------------------
#   $V:CfgS:modeRZ:*:1#   ← commande mode (depuis menu Tasker)
#   $I:Appli:Zoom:*:Off#  ← ACK fermeture appli
#   $I:Appli:Maps:*:On#   ← ACK ouverture appli
#
# PRECAUTIONS
# -----------
# - Une seule instance doit tourner (lancée par rz_start.sh)
# - Ne publie pas si la trame est vide ou identique à la précédente
# - Crée vpiv_out.txt s'il n'existe pas (évite erreur inotifywait)
#
# ARTICULATION
# ------------
#   Lancé par : rz_start.sh (start_core)
#   Arrêté par : rz_start.sh --stop (kill_process "rz_tasker_out")
#   Lit        : /sdcard/Tasker/RobotRZ/vpiv_out.txt
#   Publie sur : SE/statut (MQTT)
#
# DEPENDANCES
# -----------
#   inotifywait (inotify-tools), mosquitto_pub, keys.json
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0 - 2026-05-19
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
KEYS_FILE="$BASE_DIR/config/keys.json"
LOG_FILE="$BASE_DIR/logs/tasker_out.log"
VPIV_OUT="/sdcard/Tasker/RobotRZ/vpiv_out.txt"

# --- Credentials MQTT ---
MQTT_USER=$(jq -r '.MQTT_USER // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_PASS=$(jq -r '.MQTT_PASS // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST=$(jq -r '.MQTT_HOST // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST="${MQTT_HOST:-192.168.1.40}"

# =============================================================================
# UTILITAIRES
# =============================================================================

mkdir -p "$(dirname "$LOG_FILE")"

log() {
    local level="${2:-INFO}"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [TASKER_OUT] [$level] $1" >> "$LOG_FILE"
}

# =============================================================================
# INITIALISATION
# =============================================================================

# Créer vpiv_out.txt s'il n'existe pas (inotifywait exige un fichier existant)
if [ ! -f "$VPIV_OUT" ]; then
    touch "$VPIV_OUT"
    log "Création de vpiv_out.txt"
fi

log "=== Démarrage surveillance vpiv_out.txt ==="
log "Fichier surveillé : $VPIV_OUT"
log "Broker MQTT       : $MQTT_HOST"

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================

last_trame=""  # Mémorise la dernière trame publiée (anti-doublon)

while inotifywait -e modify "$VPIV_OUT" 2>/dev/null; do

    # Lire le contenu du fichier
    trame=$(cat "$VPIV_OUT" 2>/dev/null | tr -d '\n')

    # --- Filtre trame vide ---
    if [ -z "$trame" ]; then
        log "Trame vide ignorée" "DEBUG"
        continue
    fi

    # --- Filtre doublon ---
    if [ "$trame" = "$last_trame" ]; then
        log "Doublon ignoré : $trame" "DEBUG"
        continue
    fi

    # --- Validation format VPIV minimal ($..#) ---
    if [[ ! "$trame" =~ ^\$.+#$ ]]; then
        log "Format invalide ignoré : $trame" "WARN"
        continue
    fi

    # --- Publication MQTT ---
    log "Publication : $trame"
    if mosquitto_pub \
        -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/statut" \
        -m "$trame" \
        -q 1 2>/dev/null; then
        log "OK publié : $trame"
        last_trame="$trame"
    else
        log "Échec MQTT : $trame" "ERR"
    fi

done

log "Surveillance terminée (inotifywait a quitté)" "WARN"