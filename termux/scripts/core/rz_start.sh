#!/bin/bash
# =============================================================================
# SCRIPT  : rz_start.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/core/rz_start.sh
#
# OBJECTIF
# --------
# Script de démarrage automatique du système SE RobotRZ.
# Lance dans le bon ordre tous les daemons et managers selon la
# configuration courante (courant_init.json).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Vérification des dépendances (jq, mosquitto)
# 2. Test connexion MQTT broker (avec retry)
# 3. Vérification/création des FIFOs
# 4. Lancement core :
#    - rz_vpiv_parser.sh  (daemon MQTT ↔ FIFO)
#    - rz_state-manager.sh (état global)
# 5. Lancement managers capteurs selon courant_init.json :
#    - rz_sys_monitor.sh   si modeSys != OFF  (toujours en pratique)
#    - rz_gyro_manager.sh  si modeGyro != OFF
#    - rz_mg_manager.sh    si modeMg != OFF
#    - rz_microSe_manager.sh si modeMicro != off
#    - rz_camera_manager.sh  si cam.rear.mode ou cam.front.mode != off
#    - rz_stt_manager.sh     si modeSTT != OFF
# 6. Envoi trame d'annonce vers SP : $I:COM:info:SE:SE_READY#
# 7. Attente infinie (les daemons tournent en arrière-plan)
#
# UTILISATION
# -----------
#   bash rz_start.sh          Démarrage normal
#   bash rz_start.sh --stop   Arrêt propre de tous les managers
#   bash rz_start.sh --status Affiche l'état des processus
#
# ARTICULATION
# ------------
#   Lit    : termux/config/courant_init.json (modes des managers)
#   Lit    : termux/config/keys.json (credentials MQTT)
#   Lance  : rz_vpiv_parser.sh, rz_state-manager.sh
#   Lance  : managers capteurs selon config
#   Publie : $I:COM:info:SE:SE_READY# sur SE/statut au démarrage
#
# PRÉCAUTIONS
# -----------
# - Tuer les processus existants avant de relancer (évite les doublons)
# - Attendre que le parser soit prêt avant de lancer les managers
# - Les managers écrivent dans fifo_termux_in → ne pas lancer avant le parser
# - check_config.sh doit avoir été exécuté au moins une fois
#   pour que les *_runtime.json existent
#
# DÉPENDANCES
# -----------
#   jq, mosquitto_sub, mosquitto_pub
#   rz_vpiv_parser.sh, rz_state-manager.sh
#   rz_sys_monitor.sh, rz_gyro_manager.sh, rz_mg_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0 - 2026-04-28
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
fi

SCRIPTS_DIR="$BASE_DIR/scripts"
CONFIG_DIR="$BASE_DIR/config"
LOG_DIR="$BASE_DIR/logs"
FIFO_DIR="$BASE_DIR/fifo"

COURANT_INIT="$CONFIG_DIR/courant_init.json"
KEYS_FILE="$CONFIG_DIR/keys.json"
LOG_FILE="$LOG_DIR/rz_start.log"

# =============================================================================
# CREDENTIALS MQTT
# =============================================================================

MQTT_USER=$(jq -r '.MQTT_USER // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_PASS=$(jq -r '.MQTT_PASS // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST=$(jq -r '.MQTT_HOST // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST="${MQTT_HOST:-192.168.1.40}"

# =============================================================================
# UTILITAIRES
# =============================================================================

mkdir -p "$LOG_DIR"

log() {
    local msg="$1"
    local level="${2:-INFO}"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [START] [$level] $msg" | tee -a "$LOG_FILE"
}

ok()   { log "✓ $1" "OK";   }
warn() { log "⚠ $1" "WARN"; }
err()  { log "✗ $1" "ERR";  }

# Lecture d'une valeur dans courant_init.json
cfg() {
    jq -r "$1 // empty" "$COURANT_INIT" 2>/dev/null
}

# Tuer proprement un processus par pattern
kill_process() {
    local pattern="$1"
    local pids
    pids=$(pgrep -f "$pattern" 2>/dev/null)
    if [ -n "$pids" ]; then
        kill $pids 2>/dev/null
        sleep 0.5
        ok "Processus '$pattern' arrêté (PID $pids)"
    fi
}

# Lancer un script en arrière-plan avec log
start_bg() {
    local name="$1"
    local script="$2"
    shift 2
    if [ ! -f "$script" ]; then
        warn "$name : script introuvable ($script)"
        return 1
    fi
    bash "$script" "$@" >> "$LOG_DIR/${name}.log" 2>&1 &
    local pid=$!
    sleep 1
    if kill -0 "$pid" 2>/dev/null; then
        ok "$name démarré (PID $pid)"
        return 0
    else
        err "$name : échec au démarrage — voir $LOG_DIR/${name}.log"
        return 1
    fi
}

# =============================================================================
# COMMANDE --stop
# =============================================================================

cmd_stop() {
    log "Arrêt de tous les managers SE..."
    kill_process "rz_vpiv_parser"
    kill_process "rz_state-manager"
    kill_process "rz_sys_monitor"
    kill_process "rz_gyro_manager"
    kill_process "rz_mg_manager"
    kill_process "rz_microSe_manager"
    kill_process "rz_camera_manager"
    kill_process "rz_stt_manager"
    kill_process "rz_voice_manager"
    log "Arrêt terminé."
}

# =============================================================================
# COMMANDE --status
# =============================================================================

cmd_status() {
    log "=== État des processus SE ==="
    local procs=(
        "rz_vpiv_parser"
        "rz_state-manager"
        "rz_sys_monitor"
        "rz_gyro_manager"
        "rz_mg_manager"
        "rz_microSe_manager"
        "rz_camera_manager"
        "rz_stt_manager"
    )
    for proc in "${procs[@]}"; do
        local pid
        pid=$(pgrep -f "$proc" 2>/dev/null)
        if [ -n "$pid" ]; then
            ok "$proc (PID $pid)"
        else
            warn "$proc : non démarré"
        fi
    done
}

# =============================================================================
# ÉTAPE 1 — Vérification des dépendances
# =============================================================================

check_deps() {
    log "--- Vérification dépendances ---"
    local ok=true
    for dep in jq mosquitto_pub mosquitto_sub; do
        if command -v "$dep" &>/dev/null; then
            ok "$dep disponible"
        else
            err "$dep manquant — installez-le avec : pkg install $dep"
            ok=false
        fi
    done
    if [ ! -f "$COURANT_INIT" ]; then
        err "courant_init.json absent ($COURANT_INIT)"
        err "Exécutez d'abord : bash $CONFIG_DIR/check_config.sh"
        ok=false
    fi
    $ok || exit 1
}

# =============================================================================
# ÉTAPE 2 — Test connexion MQTT (avec retry)
# =============================================================================

check_mqtt() {
    log "--- Test connexion MQTT ($MQTT_HOST) ---"
    local max_retry=5
    local retry=0
    while [ $retry -lt $max_retry ]; do
        if mosquitto_pub -h "$MQTT_HOST" -p 1883 \
            -u "$MQTT_USER" -P "$MQTT_PASS" \
            -t "SE/statut" \
            -m "\$I:COM:info:SE:SE_CONNECTING#" \
            -q 0 >/dev/null 2>&1; then
            ok "Broker MQTT accessible ($MQTT_HOST)"
            return 0
        fi
        retry=$((retry + 1))
        warn "Broker inaccessible — tentative $retry/$max_retry (attente 5s...)"
        sleep 5
    done
    err "Broker MQTT inaccessible après $max_retry tentatives"
    err "Vérifiez la connexion réseau et l'adresse $MQTT_HOST"
    exit 1
}

# =============================================================================
# ÉTAPE 3 — Vérification/création FIFOs
# =============================================================================

check_fifos() {
    log "--- Vérification FIFOs ---"
    mkdir -p "$FIFO_DIR"
    local fifos=(
        "fifo_termux_in"
        "fifo_tasker_in"
        "fifo_appli_in"
        "fifo_voice_in"
        "fifo_return"
    )
    for fifo in "${fifos[@]}"; do
        local path="$FIFO_DIR/$fifo"
        if [ ! -p "$path" ]; then
            mkfifo "$path"
            ok "FIFO créée : $fifo"
        else
            ok "FIFO OK : $fifo"
        fi
    done
}

# =============================================================================
# ÉTAPE 4 — Lancement core (parser + state-manager)
# =============================================================================

start_core() {
    log "--- Lancement core ---"

    # Tuer les anciens processus s'ils tournent
    kill_process "rz_vpiv_parser"
    kill_process "rz_state-manager"
    sleep 1

    # Parser VPIV — daemon central MQTT ↔ FIFO
    start_bg "vpiv_parser" "$SCRIPTS_DIR/core/rz_vpiv_parser.sh"
    sleep 2  # Laisser le parser s'initialiser avant de lancer les managers

    # State-manager — gestion état global (global.json)
    start_bg "state_manager" "$SCRIPTS_DIR/core/rz_state-manager.sh"
    sleep 1
}

# =============================================================================
# ÉTAPE 5 — Lancement managers capteurs selon courant_init.json
# =============================================================================

start_sensors() {
    log "--- Lancement managers capteurs ---"

    # --- Sys monitor ---
    local mode_sys
    mode_sys=$(cfg '.sys.modeSys')
    if [ -n "$mode_sys" ] && [ "$mode_sys" != "OFF" ]; then
        kill_process "rz_sys_monitor"
        start_bg "sys_monitor" "$SCRIPTS_DIR/sensors/sys/rz_sys_monitor.sh"
    else
        warn "Sys monitor : mode=$mode_sys — non démarré"
    fi

    # --- Gyro ---
    local mode_gyro
    mode_gyro=$(cfg '.gyro.modeGyro')
    if [ -n "$mode_gyro" ] && [ "$mode_gyro" != "OFF" ]; then
        kill_process "rz_gyro_manager"
        start_bg "gyro_manager" "$SCRIPTS_DIR/sensors/gyro/rz_gyro_manager.sh"
    else
        warn "Gyro manager : mode=$mode_gyro — non démarré"
    fi

    # --- Magnétomètre ---
    local mode_mg
    mode_mg=$(cfg '.mag.modeMg')
    if [ -n "$mode_mg" ] && [ "$mode_mg" != "OFF" ]; then
        kill_process "rz_mg_manager"
        start_bg "mg_manager" "$SCRIPTS_DIR/sensors/Mg/rz_mg_manager.sh"
    else
        warn "Mag manager : mode=$mode_mg — non démarré"
    fi

    # --- Micro SE ---
    local mode_micro
    mode_micro=$(cfg '.mic.modeMicro')
    if [ -n "$mode_micro" ] && [ "$mode_micro" != "off" ] && [ "$mode_micro" != "OFF" ]; then
        kill_process "rz_microSe_manager"
        start_bg "micro_manager" "$SCRIPTS_DIR/sensors/mic/rz_microSe_manager.sh"
    else
        warn "Micro manager : mode=$mode_micro — non démarré"
    fi

    # --- Caméra ---
    local mode_cam_rear mode_cam_front
    mode_cam_rear=$(cfg '.cam.rear.mode')
    mode_cam_front=$(cfg '.cam.front.mode')
    if [ "$mode_cam_rear" != "off" ] || [ "$mode_cam_front" != "off" ]; then
        kill_process "rz_camera_manager"
        start_bg "camera_manager" "$SCRIPTS_DIR/sensors/cam/rz_camera_manager.sh"
    else
        warn "Camera manager : mode rear=$mode_cam_rear front=$mode_cam_front — non démarré"
    fi

    # --- STT ---
    local mode_stt
    mode_stt=$(cfg '.stt.modeSTT')
    if [ -n "$mode_stt" ] && [ "$mode_stt" != "OFF" ]; then
        kill_process "rz_stt_manager"
        start_bg "stt_manager" "$SCRIPTS_DIR/sensors/stt/rz_stt_manager.sh"
    else
        warn "STT manager : mode=$mode_stt — non démarré"
    fi
}

# =============================================================================
# ÉTAPE 6 — Annonce SE_READY vers SP
# =============================================================================

announce_ready() {
    sleep 2
    mosquitto_pub -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/statut" \
        -m "\$I:COM:info:SE:SE_READY#" \
        -q 1 >/dev/null 2>&1
    ok "Annonce SE_READY envoyée vers SP"
}

# =============================================================================
# MAIN
# =============================================================================

main() {
    log "=========================================="
    log "Démarrage rz_start.sh v1.0"
    log "=========================================="

    case "${1:-}" in
        --stop)
            cmd_stop
            exit 0
            ;;
        --status)
            cmd_status
            exit 0
            ;;
    esac

    check_deps
    check_mqtt
    check_fifos
    start_core
    start_sensors
    announce_ready

    log "=========================================="
    log "SE opérationnel — tous les managers actifs"
    log "  bash rz_start.sh --status  pour vérifier"
    log "  bash rz_start.sh --stop    pour arrêter"
    log "=========================================="
}

main "$@"
