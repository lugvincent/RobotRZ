#!/bin/bash
# =============================================================================
# SCRIPT  : urgence.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/sys/lib/urgence.sh
#
# OBJECTIF
# --------
# Bibliothèque de déclenchement d'urgences système.
# Fournit trigger_urgence() utilisée par alerts_sys.sh.
#
# UTILISATION
# -----------
# Ce fichier est SOURCÉ (source urgence.sh), pas exécuté directement.
# Il hérite de l'environnement du script parent :
#   - FIFO_IN  : chemin du pipe nommé (défini dans rz_sys_monitor.sh)
#   - LOG_FILE : chemin du log       (défini dans rz_sys_monitor.sh)
#
# FLUX VPIV PRODUITS
# ------------------
#   $E:Urg:source:SE:URG_CPU_CHARGE#    → CPU surchargé
#   $E:Urg:source:SE:URG_OVERHEAT#      → Température critique
#   $E:Urg:source:SE:URG_SENSOR_FAIL#   → Stockage ou mémoire critique
#   $E:Urg:source:SE:URG_UNKNOWN#       → Code non reconnu (fallback)
#
# CODES URG VALIDES (Table A)
# ---------------------------
#   URG_OVERHEAT          Surchauffe détectée
#   URG_LOW_BAT           Batterie faible
#   URG_MOTOR_STALL       Moteur bloqué
#   URG_SENSOR_FAIL       Défaillance capteur
#   URG_BUFFER_OVERFLOW   Dépassement mémoire
#   URG_PARSING_ERROR     Erreur de parsing
#   URG_CPU_CHARGE        Charge CPU excessive
#   URG_APPLI_FAIL        Échec application
#   URG_LOOP_TOO_SLOW_SE     Boucle trop lente coté SE
#   URG_LOOP_TOO_SLOW_A     Boucle trop lente coté A
#   URG_UNKNOWN           Urgence inconnue (fallback)
#
# PRÉCAUTIONS
# -----------
# - Ne jamais appeler mosquitto_pub ici.
# - La FIFO doit exister (créée par fifo_manager.sh).
# - Écriture non-bloquante (watchdog 2s) pour ne pas figer le monitor.
# - Ce fichier doit être sourcé AVANT alerts_sys.sh.
#
# ARTICULATION
# ------------
#   rz_sys_monitor.sh → source urgence.sh
#   alerts_sys.sh     → appelle trigger_urgence()
#   trigger_urgence() → FIFO_IN → rz_vpiv_parser.sh → MQTT → SP
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.1  (pas de changement fonctionnel vs 2.0)
# DATE    : 2026-03-04
# =============================================================================

# Codes d'urgence valides — utilisés pour validation avant envoi
readonly -a URG_CODES_VALIDES=(
    "URG_OVERHEAT"
    "URG_LOW_BAT"
    "URG_MOTOR_STALL"
    "URG_SENSOR_FAIL"
    "URG_BUFFER_OVERFLOW"
    "URG_PARSING_ERROR"
    "URG_CPU_CHARGE"
    "URG_APPLI_FAIL"
    "URG_LOOP_TOO_SLOW_SE"
    "URG_UNKNOWN"
)

# =============================================================================
# FONCTION : trigger_urgence
# ARGS     : $1 = code source (ex: URG_CPU_CHARGE)
# EFFET    : Écrit $E:Urg:source:SE:<code># dans FIFO_IN
# =============================================================================
trigger_urgence() {
    local source="$1"

    # Validation du code — fallback URG_UNKNOWN si inconnu
    local code_valide=false
    for code in "${URG_CODES_VALIDES[@]}"; do
        [[ "$source" == "$code" ]] && code_valide=true && break
    done
    if [[ "$code_valide" == false ]]; then
        echo "[URG] WARN : code inconnu '$source' → remplacé par URG_UNKNOWN" >> "${LOG_FILE:-/dev/null}"
        source="URG_UNKNOWN"
    fi

    # Vérification FIFO disponible
    if [[ -z "$FIFO_IN" ]]; then
        echo "[URG] ERREUR : FIFO_IN non défini dans le script parent" >> "${LOG_FILE:-/dev/null}"
        return 1
    fi
    if [[ ! -p "$FIFO_IN" ]]; then
        echo "[URG] WARN : FIFO $FIFO_IN absente. Urgence $source perdue." >> "${LOG_FILE:-/dev/null}"
        return 1
    fi

    # Envoi non-bloquant (watchdog 2s)
    echo "\$E:Urg:source:SE:${source}#" > "$FIFO_IN" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null

    echo "[URG] Urgence émise : $source" >> "${LOG_FILE:-/dev/null}"
    return 0
}