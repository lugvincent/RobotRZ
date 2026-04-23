#!/bin/bash
# =============================================================================
# SCRIPT  : rz_voice_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/voice/rz_voice_manager.sh
#
# OBJECTIF
# --------
# Daemon de gestion de la sortie audio du Smartphone Embarqué (SE).
# Centralise trois fonctions liées au son sortant :
#   1. TTS  (synthèse vocale)   : termux-tts-speak
#   2. Volume multimédia        : termux-volume canal music (0-100%)
#   3. Sortie audio             : détection et publication (internal|jack|bt)
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Lit en continu fifo/fifo_voice_in.
# Les trames sont routées par rz_vpiv_parser.sh depuis MQTT topic SE/commande.
# Chaque trame est analysée selon son type et dispatchée.
#
# TRAMES TRAITÉES
# ----------------
#   $A:Voice:tts:*:"texte"#         → TTS (interruptible par urgence)
#   $V:Voice:vol:*:80#              → Volume 0-100% + ACK $I:Voice:vol
#   $V:Voice:output:*:jack#         → Pub sortie active + ACK (commutation V2)
#   $E:Urg:source:*:URG_*#          → TTS urgence prioritaire
#
# GESTION VOLUME
# --------------
# termux-volume --stream music --volume X
# Plage Android (0-15 typique) détectée dynamiquement au démarrage.
# Normalisation VPIV 0-100 → plage Android native.
# Volume restauré depuis voice_config.json au démarrage.
# ⚠ INDÉPENDANT de modeSTT : actif même si STT est coupé.
#   Voice gère toute la sortie audio SE (TTS, musique, baffe jack/BT).
#
# GESTION SORTIE AUDIO
# --------------------
# Android commute automatiquement (jack prioritaire, puis BT, puis HP).
# Ce manager détecte la sortie active via termux-volume et publie l'état.
# Commutation BT forcée = V2 (Tasker ou am broadcast).
#
# TTS URGENCE
# -----------
# $E:Urg interrompt immédiatement le TTS en cours (kill PID en cours).
#
# INTERACTIONS
# ------------
#   Lit     : fifo/fifo_voice_in              (trames VPIV depuis vpiv_parser)
#   Lit     : config/sensors/voice_config.json (vol et output initiaux)
#   Écrit   : fifo/fifo_termux_in             (ACK $I:Voice:* vers state-manager)
#   Log     : logs/voice_manager.log
#   Appelle : termux-tts-speak, termux-volume (Termux API)
#
# PRÉCAUTIONS
# -----------
# - UNE SEULE instance (PID guard sur logs/voice_manager.pid).
# - termux-volume nécessite pkg install termux-api.
# - Si termux-volume absent : volume non géré, TTS reste fonctionnel.
# - Ne jamais lire fifo_termux_in ici (réservé à state-manager).
# - Démarrer après rz_vpiv_parser.sh (crée fifo_voice_in).
#
# ARBORESCENCE PROJET
# -------------------
#   scripts/sensors/voice/
#       rz_voice_manager.sh       ← ce fichier
#   config/sensors/
#       voice_config.json         ← généré par check_config.sh
#   fifo/
#       fifo_voice_in             ← créé par rz_vpiv_parser.sh (init_fifo)
#   logs/
#       voice_manager.log
#       voice_manager.pid
#
# DÉPENDANCES
# -----------
#   termux-tts-speak   (pkg install termux-api)
#   termux-volume      (pkg install termux-api)
#   jq                 (pkg install jq)
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0  (création module Voice autonome)
# DATE    : 2026-04-22
# =============================================================================

# =============================================================================
# CONFIGURATION DES CHEMINS
# Convention projet : remonte de scripts/sensors/voice/ vers racine termux/
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    # Fallback développement PC
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

VOICE_CONFIG="$BASE_DIR/config/sensors/voice_config.json"
FIFO_VOICE="$BASE_DIR/fifo/fifo_voice_in"
FIFO_TERMUX="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/voice_manager.log"
PID_FILE="$BASE_DIR/logs/voice_manager.pid"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_voice() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [VOICE] $1" >> "$LOG_FILE"
    echo "[VOICE] $1"
}

# Écriture non-bloquante dans fifo_termux_in (ACK vers state-manager)
# Même pattern que write_fifo() dans rz_vpiv_parser.sh (timeout 1s)
send_ack() {
    local trame="$1"
    if [ ! -p "$FIFO_TERMUX" ]; then
        log_voice "WARN : fifo_termux_in absent — ACK perdu : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_TERMUX" &
    local pid_w=$!
    ( sleep 1 && kill "$pid_w" 2>/dev/null ) &
    wait "$pid_w" 2>/dev/null
}

# =============================================================================
# CHARGEMENT DE LA CONFIGURATION
# voice_config.json généré par check_config.sh depuis courant_init.json
# =============================================================================

vol_courant=80
output_courant="internal"
tts_rate="1.0"

load_config() {
    if [ ! -f "$VOICE_CONFIG" ]; then
        log_voice "WARN : $VOICE_CONFIG absent — défauts : vol=80 output=internal"
        return
    fi

    local v
    v=$(jq -r '.voice.vol // 80' "$VOICE_CONFIG" 2>/dev/null)
    [[ "$v" =~ ^[0-9]+$ ]] && [ "$v" -ge 0 ] && [ "$v" -le 100 ] && vol_courant=$v

    v=$(jq -r '.voice.output // "internal"' "$VOICE_CONFIG" 2>/dev/null)
    case "$v" in internal|jack|bt) output_courant=$v ;; esac

    v=$(jq -r '.voice.ttsRate // "1.0"' "$VOICE_CONFIG" 2>/dev/null)
    [[ "$v" =~ ^[0-9]+(\.[0-9]+)?$ ]] && tts_rate=$v

    log_voice "Config chargée : vol=${vol_courant}% output=$output_courant ttsRate=$tts_rate"
}

# =============================================================================
# GESTION DU VOLUME
# Normalise 0-100 (VPIV) → plage Android native (détectée au boot).
# =============================================================================

ANDROID_VOL_MAX=15   # défaut, remplacé par detect_android_vol_max()
TERMUX_VOL_OK=false  # flag : termux-volume disponible ?

detect_android_vol_max() {
    if ! command -v termux-volume &>/dev/null; then
        log_voice "WARN : termux-volume absent — gestion volume désactivée (TTS actif)"
        TERMUX_VOL_OK=false
        return
    fi
    TERMUX_VOL_OK=true

    local info max
    info=$(termux-volume 2>/dev/null)
    if [ $? -ne 0 ] || [ -z "$info" ]; then
        log_voice "WARN : termux-volume inaccessible — plage défaut : 0-$ANDROID_VOL_MAX"
        return
    fi

    max=$(echo "$info" | jq -r '.[] | select(.stream=="music") | .maxVolume' 2>/dev/null)
    if [[ "$max" =~ ^[0-9]+$ ]] && [ "$max" -gt 0 ]; then
        ANDROID_VOL_MAX=$max
        log_voice "Plage volume Android : 0-$ANDROID_VOL_MAX (canal music)"
    else
        log_voice "WARN : maxVolume non lu — défaut : 0-$ANDROID_VOL_MAX"
    fi
}

appliquer_volume() {
    local vol_pct="$1"

    if ! [[ "$vol_pct" =~ ^[0-9]+$ ]] || [ "$vol_pct" -lt 0 ] || [ "$vol_pct" -gt 100 ]; then
        log_voice "WARN : volume '$vol_pct' invalide — ignoré"
        return 1
    fi

    if [ "$TERMUX_VOL_OK" == "false" ]; then
        log_voice "WARN : termux-volume absent — volume ignoré"
        return 1
    fi

    # Normalisation vers plage Android
    local vol_android
    vol_android=$(( vol_pct * ANDROID_VOL_MAX / 100 ))

    termux-volume --stream music --volume "$vol_android" 2>/dev/null
    if [ $? -eq 0 ]; then
        vol_courant=$vol_pct
        log_voice "Volume : ${vol_pct}% → Android $vol_android/$ANDROID_VOL_MAX ✓"
        send_ack "\$I:Voice:vol:*:${vol_pct}#"
    else
        log_voice "ERREUR : termux-volume échoué (android=$vol_android)"
        return 1
    fi
}

# =============================================================================
# DÉTECTION SORTIE AUDIO
# termux-volume JSON expose audioOutput sur certaines versions de termux-api.
# =============================================================================

detecter_sortie() {
    if [ "$TERMUX_VOL_OK" == "false" ]; then
        echo "internal"
        return
    fi

    local info ao
    info=$(termux-volume 2>/dev/null)
    [ $? -ne 0 ] && { echo "internal"; return; }

    ao=$(echo "$info" | jq -r '.[0].audioOutput // ""' 2>/dev/null)
    case "$ao" in
        "BLUETOOTH")                        echo "bt"       ;;
        "WIRED_HEADSET"|"WIRED_HEADPHONES") echo "jack"     ;;
        *)                                  echo "internal" ;;
    esac
}

publier_sortie() {
    local sortie
    sortie=$(detecter_sortie)
    if [ "$sortie" != "$output_courant" ]; then
        output_courant=$sortie
        log_voice "Changement sortie audio détecté : $sortie"
    fi
    send_ack "\$I:Voice:output:*:${output_courant}#"
}

# =============================================================================
# GESTION TTS
# =============================================================================

tts_pid=""

tts_speak() {
    local texte="$1"
    local urgence="${2:-non}"   # "oui" = interrompt le TTS en cours

    # Nettoyage guillemets VPIV
    texte=$(echo "$texte" | tr -d '"')
    [ -z "$texte" ] && { log_voice "WARN : TTS texte vide — ignoré"; return; }

    # Interruption si urgence
    if [ "$urgence" == "oui" ] && [ -n "$tts_pid" ] && kill -0 "$tts_pid" 2>/dev/null; then
        kill "$tts_pid" 2>/dev/null
        log_voice "TTS urgence : interruption PID $tts_pid"
        tts_pid=""
    fi

    log_voice "TTS${urgence:+ [URG]} : $texte"
    termux-tts-speak "$texte" &
    tts_pid=$!
}

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================

run_voice_loop() {
    log_voice "Boucle démarrée — écoute $FIFO_VOICE"

    local trame

    while true; do

        if IFS= read -r trame < "$FIFO_VOICE" 2>/dev/null; then

            # ── TTS direct ───────────────────────────────────────────────────
            if [[ "$trame" =~ ^\$A:Voice:tts:[^:]+:(.+)#$ ]]; then
                tts_speak "${BASH_REMATCH[1]}" "non"

            # ── Volume ───────────────────────────────────────────────────────
            elif [[ "$trame" =~ ^\$V:Voice:vol:[^:]+:([0-9]+)#$ ]]; then
                appliquer_volume "${BASH_REMATCH[1]}"

            # ── Sortie audio ─────────────────────────────────────────────────
            elif [[ "$trame" =~ ^\$V:Voice:output:[^:]+:([^#]+)#$ ]]; then
                local cible="${BASH_REMATCH[1]}"
                local active
                active=$(detecter_sortie)
                log_voice "Sortie demandée=$cible active=$active"
                if [ "$active" != "$cible" ]; then
                    log_voice "WARN : commutation '$cible' non dispo V1 — jack/internal=auto Android, BT=V2"
                    send_ack "\$I:COM:warn:SE:\"Voice:output $cible indisponible, actif:$active\"#"
                fi
                output_courant=$active
                send_ack "\$I:Voice:output:*:${active}#"

            # ── Urgence ──────────────────────────────────────────────────────
            elif [[ "$trame" =~ ^\$E:Urg:source:[^:]+:(URG_[^#]*)#$ ]]; then
                case "${BASH_REMATCH[1]}" in
                    "URG_LOW_BAT")     tts_speak "Batterie critique."     "oui" ;;
                    "URG_CPU_CHARGE")  tts_speak "Surcharge processeur."  "oui" ;;
                    "URG_OVERHEAT")    tts_speak "Surchauffe détectée."   "oui" ;;
                    "URG_SENSOR_FAIL") tts_speak "Capteur défaillant."    "oui" ;;
                    *)                 tts_speak "Alerte sécurité."       "oui" ;;
                esac

            else
                log_voice "WARN : trame non reconnue : $trame"
            fi
        fi

        sleep 0.05
    done
}

# =============================================================================
# MAIN
# =============================================================================

main() {
    log_voice "=========================================="
    log_voice "Démarrage rz_voice_manager.sh  v1.0"
    log_voice "=========================================="

    # ── Unicité instance ──────────────────────────────────────────────────────
    if [ -f "$PID_FILE" ]; then
        local old_pid
        old_pid=$(cat "$PID_FILE")
        if kill -0 "$old_pid" 2>/dev/null; then
            log_voice "Instance déjà en cours (PID=$old_pid) — arrêt."
            exit 1
        fi
        rm -f "$PID_FILE"
    fi
    echo $$ > "$PID_FILE"

    # ── Vérification FIFO ────────────────────────────────────────────────────
    # fifo_voice_in est normalement créé par rz_vpiv_parser.sh (init_fifo).
    local wait_fifo=0
    while [ ! -p "$FIFO_VOICE" ] && [ $wait_fifo -lt 5 ]; do
        log_voice "Attente fifo_voice_in ($wait_fifo/5)..."
        sleep 1
        wait_fifo=$(( wait_fifo + 1 ))
    done
    if [ ! -p "$FIFO_VOICE" ]; then
        log_voice "WARN : fifo_voice_in absent — création locale (parser non démarré ?)"
        mkdir -p "$(dirname "$FIFO_VOICE")"
        mkfifo "$FIFO_VOICE"
    fi

    # ── Chargement config ────────────────────────────────────────────────────
    load_config

    # ── Détection plage volume ───────────────────────────────────────────────
    detect_android_vol_max

    # ── Volume initial ───────────────────────────────────────────────────────
    log_voice "Application volume initial : ${vol_courant}%"
    appliquer_volume "$vol_courant"

    # ── Sortie audio initiale ────────────────────────────────────────────────
    publier_sortie

    # ── Boucle principale ────────────────────────────────────────────────────
    run_voice_loop
}

trap '
    log_voice "Signal reçu — arrêt propre."
    rm -f "$PID_FILE"
    [ -n "$tts_pid" ] && kill "$tts_pid" 2>/dev/null
    exit 0
' EXIT INT TERM

main
