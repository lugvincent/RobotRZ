#!/bin/bash
# =============================================================================
# SCRIPT  : rz_stt_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_stt_manager.sh
#
# OBJECTIF
# --------
# Lanceur et superviseur du daemon vocal Python (rz_stt_manager.py).
# Gère également le flux TTS local :
#   - Écoute fifo_termux_in pour les trames $A:Voice:tts:*
#   - Exécute termux-tts-speak en priorité (interruptible)
#   - Vérification des prérequis avant lancement Python
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script remplace l'ancienne boucle Bash de reconnaissance vocale.
# La reconnaissance (PocketSphinx) et la logique de bascule locale/IA
# sont entièrement gérées par rz_stt_manager.py.
#
# Ce .sh assure :
#   1. Vérification prérequis (python3, pocketsphinx, modèles)
#   2. Lancement de rz_stt_manager.py en arrière-plan supervisé
#   3. Relance automatique si crash (max MAX_RESTARTS fois)
#   4. Écoute TTS côté SE : trames $A:Voice:tts ou $A:COM:*
#      Ces trames arrivent via fifo_termux_in (depuis rz_vpiv_parser.sh)
#
# FLUX TTS
# --------
# Désormais délégué à rz_voice_manager.sh via fifo_voice_in.
# SP → MQTT → rz_vpiv_parser.sh → fifo_voice_in → rz_voice_manager.sh
# rz_stt_manager.sh ne lit plus fifo_termux_in pour le TTS.
#
# INTERACTIONS
# ------------
#   Lance   : rz_stt_manager.py  (Python, PocketSphinx)
#   Lit     : fifo/fifo_termux_in  (trames TTS)
#   Lit     : config/global.json   (typePtge pour TTS conditionnel)
#   Log     : logs/stt_manager.log
#
# PRÉCAUTIONS
# -----------
# - Ne pas lancer deux instances simultanées (vérification PID).
# - En cas d'absence des modèles PocketSphinx, le Python s'arrête
#   proprement avec un message d'erreur (pas de redémarrage infini).
# - MAX_RESTARTS évite une boucle infinie en cas d'erreur persistante.
#
# DÉPENDANCES
# -----------
#   python3, pocketsphinx (pip), termux-tts-speak, jq
#   rz_stt_manager.py, lib_pocketsphinx/model-fr/, lib_pocketsphinx/fr.dict
#
# AUTEUR  : Vincent Philippe
#   VERSION : 2.1  (délégation TTS à rz_voice_manager.sh, suppression run_tts_listener)
# DATE    : 2026-04-22
# =============================================================================

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

STT_DIR="$BASE_DIR/scripts/sensors/stt"
LIB_DIR="$STT_DIR/lib_pocketsphinx"

MANAGER_PY="$STT_DIR/rz_stt_manager.py"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"
GLOBAL_JSON="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/stt_manager.log"
PID_FILE="$BASE_DIR/logs/stt_manager.pid"

# Nombre max de redémarrages automatiques après crash
MAX_RESTARTS=3

# =============================================================================
# UTILITAIRES
# =============================================================================

log_stt() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [STT-MGR] $1" >> "$LOG_FILE"
    echo "[STT] $1"
}

# Lit typePtge dans global.json (0 = pas de retour vocal SE)
get_ptge_mode() {
    jq -r '.CfgS.typePtge // 0' "$GLOBAL_JSON" 2>/dev/null || echo "0"
}

# =============================================================================
# VÉRIFICATION DES PRÉREQUIS
# =============================================================================

check_prerequisites() {
    local ok=1

    if ! command -v python3 &>/dev/null; then
        log_stt "ERREUR : python3 non installé"
        ok=0
    fi

    if ! python3 -c "import pocketsphinx" 2>/dev/null; then
        log_stt "ERREUR : pocketsphinx non installé (pip install pocketsphinx)"
        ok=0
    fi

    if [ ! -f "$MANAGER_PY" ]; then
        log_stt "ERREUR : rz_stt_manager.py introuvable ($MANAGER_PY)"
        ok=0
    fi

    if [ ! -d "$LIB_DIR/model-fr" ] || [ ! -f "$LIB_DIR/fr.dict" ]; then
        log_stt "ERREUR : modèles PocketSphinx absents dans $LIB_DIR"
        log_stt "         Vérifier : model-fr/ et fr.dict"
        ok=0
    fi

    if [ $ok -eq 0 ]; then
        log_stt "Prérequis manquants — arrêt."
        return 1
    fi

    log_stt "Prérequis OK"
    return 0
}

# =============================================================================
# SUPERVISION DU DAEMON PYTHON
# Relance automatique jusqu'à MAX_RESTARTS en cas de crash.
# Pas de relance si exit 1 (erreur de config, modèles absents).
# =============================================================================

run_python_daemon() {
    local restarts=0

    while true; do
        log_stt "Lancement rz_stt_manager.py (tentative $((restarts+1))/$MAX_RESTARTS) ..."

        python3 "$MANAGER_PY"
        local exit_code=$?

        log_stt "rz_stt_manager.py terminé (exit=$exit_code)"

        # exit 1 = erreur fatale (modèles absents, etc.) → pas de relance
        if [ $exit_code -eq 1 ]; then
            log_stt "Erreur fatale Python — pas de relance."
            break
        fi

        # exit 0 = arrêt propre (Ctrl+C) → pas de relance
        if [ $exit_code -eq 0 ]; then
            log_stt "Arrêt propre Python — fin du superviseur."
            break
        fi

        # Crash (exit != 0 et != 1) → relance avec compteur
        restarts=$((restarts + 1))
        if [ $restarts -ge $MAX_RESTARTS ]; then
            log_stt "MAX_RESTARTS ($MAX_RESTARTS) atteint — arrêt superviseur."
            break
        fi

        log_stt "Crash détecté — relance dans 3s ..."
        sleep 3
    done
}

# =============================================================================
# BOUCLE TTS LOCAL
# Écoute les trames TTS sur fifo_termux_in et les exécute.
# Tourne en arrière-plan pendant que le Python gère la reconnaissance.
# =============================================================================



# =============================================================================
# MAIN
# =============================================================================

main() {
    log_stt "=========================================="
    log_stt "Démarrage rz_stt_manager.sh  v2.0"
    log_stt "=========================================="

    # Vérification instance déjà en cours
    if [ -f "$PID_FILE" ]; then
        local old_pid
        old_pid=$(cat "$PID_FILE")
        if kill -0 "$old_pid" 2>/dev/null; then
            log_stt "Instance déjà en cours (PID=$old_pid) — arrêt."
            exit 1
        fi
    fi
    echo $$ > "$PID_FILE"

    # Vérification prérequis
    if ! check_prerequisites; then
        exit 1
    fi

    # Vérification FIFO
    if [ ! -p "$FIFO_IN" ]; then
        log_stt "WARN : $FIFO_IN absent — TTS listener désactivé"
    fi

    # Lancement superviseur Python (bloquant jusqu'à MAX_RESTARTS)
    run_python_daemon

    # Nettoyage
    rm -f "$PID_FILE"
    log_stt "Module STT arrêté."
}

# Nettoyage propre sur Ctrl+C
trap 'log_stt "Signal reçu — arrêt."; rm -f "$PID_FILE"; exit 0' EXIT INT TERM

main