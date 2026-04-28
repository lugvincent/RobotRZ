#!/bin/bash
# =============================================================================
# SCRIPT  : error_handler.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/utils/error_handler.sh
#
# OBJECTIF
# --------
# Gestion centralisée des erreurs et urgences selon la Table A.
# Fournit deux fonctions utilitaires appelables depuis tout script SE :
#   handle_urgence  → émet un $E:Urg vers SP
#   handle_message  → émet un $I:COM vers SP
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Toutes les trames VPIV transitent par fifo_termux_in → rz_vpiv_parser.sh
# → MQTT → SP. Jamais de mosquitto_pub direct (règle architecture SE).
#
# TABLE A — VPIV ÉMIS
# --------------------
#   $E:Urg:source:SE:<URG_CODE>#   urgence vers SP
#   $I:Urg:statut:*:active#        statut urgence active
#   $I:Urg:niveau:SE:<niveau>#     niveau warn|danger
#   $I:COM:<type>:SE:<message>#    message informatif
#
# UTILISATION
# -----------
#   source error_handler.sh
#   handle_urgence "URG_OVERHEAT" "danger" "Température CPU: 85°C"
#   handle_message "warn" "Batterie faible (20%)"
#
# ARTICULATION
# ------------
#   Appelé par : tous les managers SE (gyro, sys, mag, cam...)
#   Écrit dans : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# ⚠️ Ne jamais appeler mosquitto_pub directement — tout passe par la FIFO.
# ⚠️ fifo_termux_in doit exister (créé par fifo_manager.sh create).
# - Écriture non-bloquante (timeout 1s) pour ne pas bloquer l'appelant.
#
# DÉPENDANCES
# -----------
#   jq : lecture keys.json (non utilisé ici — FIFO uniquement)
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (architecture FIFO — suppression mosquitto_pub direct)
# DATE    : 2026-04-27
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
fi

FIFO_TERMUX="$BASE_DIR/fifo/fifo_termux_in"

# =============================================================================
# UTILITAIRE — Écriture non-bloquante dans la FIFO
# Timeout 1s pour ne pas bloquer l'appelant si le parser est absent.
# =============================================================================

_write_fifo_eh() {
    local trame="$1"
    if [ ! -p "$FIFO_TERMUX" ]; then
        echo "[error_handler] WARN : FIFO absente — trame perdue : $trame" >&2
        return 1
    fi
    echo "$trame" > "$FIFO_TERMUX" &
    local pid_w=$!
    ( sleep 1 && kill "$pid_w" 2>/dev/null ) &
    wait "$pid_w" 2>/dev/null
}

# =============================================================================
# TYPES D'URGENCES VALIDES (Table A — VAR=Urg, PROP=source)
# =============================================================================

declare -A URG_TYPES=(
    ["URG_CPU_CHARGE"]="Charge CPU excessive"
    ["URG_OVERHEAT"]="Surchauffe détectée"
    ["URG_LOW_BAT"]="Niveau batterie faible"
    ["URG_MOTOR_STALL"]="Moteur bloqué"
    ["URG_SENSOR_FAIL"]="Défaillance capteur"
    ["URG_BUFFER_OVERFLOW"]="Dépassement mémoire"
    ["URG_PARSING_ERROR"]="Erreur de parsing"
    ["URG_APPLI_FAIL"]="Échec application"
    ["URG_LOOP_TOO_SLOW_SE"]="Boucle trop lente"
    ["URG_UNKNOWN"]="Urgence inconnue"
)

# =============================================================================
# TYPES DE MESSAGES COM VALIDES (Table A — VAR=COM)
# =============================================================================

declare -A MSG_TYPES=(
    ["debug"]="DEBUG"
    ["error"]="ERREUR"
    ["info"]="INFO"
    ["warn"]="AVERTISSEMENT"
)

# =============================================================================
# FONCTION : handle_urgence
# =============================================================================
# Émet les trames d'urgence vers SP via fifo_termux_in.
#
# Usage   : handle_urgence <source> <niveau> <raison>
# Exemple : handle_urgence "URG_OVERHEAT" "danger" "Température: 86°C"
#
# Paramètres :
#   source : code urgence (voir URG_TYPES) — fallback URG_UNKNOWN si inconnu
#   niveau : warn|danger
#   raison : texte libre pour COM:error (facultatif)
# =============================================================================

handle_urgence() {
    local source="$1"
    local niveau="$2"
    local raison="$3"

    # Validation source — fallback URG_UNKNOWN
    if [[ -z "${URG_TYPES[$source]}" ]]; then
        source="URG_UNKNOWN"
    fi

    # Validation niveau — fallback warn
    if [[ "$niveau" != "danger" && "$niveau" != "warn" ]]; then
        niveau="warn"
    fi

    # Table A : $E:Urg:source:SE:<code>#
    _write_fifo_eh "\$E:Urg:source:SE:${source}#"

    # Table A : $I:Urg:statut:*:active#
    _write_fifo_eh "\$I:Urg:statut:*:active#"

    # Table A : $I:Urg:niveau:SE:<niveau>#
    _write_fifo_eh "\$I:Urg:niveau:SE:${niveau}#"

    # Table A : $I:COM:error:SE:<raison># (si raison fournie)
    if [[ -n "$raison" ]]; then
        _write_fifo_eh "\$I:COM:error:SE:\"${raison}\"#"
    fi
}

# =============================================================================
# FONCTION : handle_message
# =============================================================================
# Émet un message COM informatif vers SP via fifo_termux_in.
#
# Usage   : handle_message <type> <message>
# Exemple : handle_message "warn" "Batterie faible (20%)"
#
# Paramètres :
#   type    : debug|error|info|warn — fallback error si inconnu
#   message : texte libre
# =============================================================================

handle_message() {
    local type="$1"
    local message="$2"

    # Validation type — fallback error
    if [[ -z "${MSG_TYPES[$type]}" ]]; then
        type="error"
    fi

    # Table A : $I:COM:<type>:SE:<message>#
    _write_fifo_eh "\$I:COM:${type}:SE:${message}#"
}