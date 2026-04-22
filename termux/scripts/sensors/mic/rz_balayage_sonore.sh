#!/bin/bash
# =============================================================================
# SCRIPT  : rz_balayage_sonore.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/mic/rz_balayage_sonore.sh
#
# OBJECTIF
# --------
# Balayage sonore pour localisation de la source audio.
# Pilote le servo TGD (tête gauche-droite) via VPIV $V:Srv:TGD:*:<angle>#,
# mesure le RMS à chaque position, retourne l'angle du niveau max.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Lancé en sous-shell non-bloquant par rz_microSe_manager.sh.
# Parcourt -90° → +90° par pas de pasBalayage degrés.
# À chaque position : commande servo TGD via FIFO puis mesure RMS.
# À la fin : envoie direction (angle max RMS) + repositionne servo à 0°.
# Arduino (3 micros) peut confirmer indépendamment l'angle via VPIV A→SP.
#
# ARGUMENTS
# ---------
#   $1 = pasBalayage       (degrés, ex: 10)
#   $2 = timeoutOrientation (secondes max, ex: 15)
#   $3 = winSize           (ms enregistrement par position, ex: 300)
#   $4 = FIFO_OUT          (chemin fifo_termux_in)
#   $5 = LOG_FILE          (chemin log partagé avec manager)
#
# VPIV PRODUITS
# -------------
#   $V:Srv:TGD:*:<angle>#       Commande servo TGD à chaque position
#   $F:MicroSE:direction:*:<angle>#  Angle direction max RMS (résultat final)
#   $V:Srv:TGD:*:0#             Retour position centrale en fin de balayage
#   $I:COM:error:SE:"..."#      Erreur si balayage échoue
#
# PRÉCAUTIONS
# -----------
# - Attend 200ms après commande servo avant mesure (stabilisation mécanique).
# - Timeout global : si balayage dépasse timeoutOrientation s → arrêt immédiat.
# - WAV_TEMP : même chemin que dans le manager (ne pas modifier).
# - Si toutes les mesures RMS = 0 → pas d'envoi direction (pas de source détectée).
#
# DÉPENDANCES
# -----------
#   ffmpeg, termux-microphone-record, bc
#   fifo_termux_in (argument $4)
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (VPIV Srv:TGD via FIFO, sans mosquitto_pub, timeout global,
#                  retour position 0°, gestion RMS=0)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# ARGUMENTS
# =============================================================================

PAS_BALAYAGE="${1:-10}"
TIMEOUT_S="${2:-15}"
WIN_SIZE_MS="${3:-300}"
FIFO_OUT="${4:-/tmp/fifo_termux_in}"
LOG_FILE="${5:-/tmp/balayage.log}"

WAV_TEMP="/tmp/microse_measure.wav"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_bal() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [BALAYAGE] $1" >> "$LOG_FILE"
}

# Envoi VPIV via FIFO (non-bloquant, watchdog 2s)
send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_OUT" ]; then
        log_bal "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null
}

# Commande servo TGD via VPIV → FIFO
# $1 = angle (-90 à +90)
set_servo_tgd() {
    local angle="$1"
    # Conversion angle -90/+90 vers valeur servo 0–180 (centre = 90)
    local servo_val=$(( angle + 90 ))
    send_vpiv "\$V:Srv:TGD:*:${servo_val}#"
    log_bal "Servo TGD → ${angle}° (servo_val=${servo_val})"
    # Délai stabilisation mécanique servo (200ms)
    sleep 0.2
}

# Mesure RMS à la position courante
# Retourne RMS×10 (entier 0–1000)
read_rms_position() {
    local duration_s
    duration_s=$(echo "scale=2; $WIN_SIZE_MS / 1000" | bc)

    rm -f "$WAV_TEMP"
    termux-microphone-record -d "$duration_s" -f "$WAV_TEMP" 2>/dev/null

    if [ ! -f "$WAV_TEMP" ]; then
        echo "0"
        return
    fi

    local rms_raw
    rms_raw=$(ffmpeg -i "$WAV_TEMP" \
                     -af "astats=measure_perchannel=0:measure_overall=RMS_level" \
                     -f null - 2>&1 \
              | grep "RMS level" | tail -1 \
              | awk '{print $NF}')

    if [ -z "$rms_raw" ]; then
        echo "0"
        return
    fi

    # Conversion dBFS → 0–1000
    echo "$rms_raw" | awk '{
        val = (100 + $1) * 10
        if (val < 0)    val = 0
        if (val > 1000) val = 1000
        printf "%d", val
    }'
}

# =============================================================================
# MAIN — Balayage -90° → +90°
# =============================================================================

main() {
    log_bal "=== Début du balayage sonore ==="
    log_bal "Paramètres : pas=${PAS_BALAYAGE}° timeout=${TIMEOUT_S}s winSize=${WIN_SIZE_MS}ms"

    local max_rms=0
    local max_angle=0
    local nb_mesures=0
    local ts_debut
    ts_debut=$(date +%s)

    # Séquence d'angles : -90 → +90 par pasBalayage
    local angle=-90
    while (( angle <= 90 )); do

        # Vérification timeout global
        local ts_now
        ts_now=$(date +%s)
        if (( ts_now - ts_debut >= TIMEOUT_S )); then
            log_bal "TIMEOUT atteint (${TIMEOUT_S}s). Balayage interrompu à ${angle}°."
            send_vpiv "\$I:COM:error:SE:\"MicroSE : balayage interrompu (timeout ${TIMEOUT_S}s)\"#"
            break
        fi

        # Positionnement servo
        set_servo_tgd "$angle"

        # Mesure RMS à cette position
        local rms
        rms=$(read_rms_position)
        log_bal "  ${angle}° → RMS=${rms}"
        nb_mesures=$(( nb_mesures + 1 ))

        # Mise à jour maximum
        if (( rms > max_rms )); then
            max_rms=$rms
            max_angle=$angle
        fi

        angle=$(( angle + PAS_BALAYAGE ))
    done

    log_bal "Balayage terminé : $nb_mesures mesures. Max RMS=$max_rms à ${max_angle}°."

    # Envoi résultat uniquement si au moins un niveau significatif détecté
if (( max_rms > 0 )); then
    # Conversion en degrés * 10 pour conformité Table A
    local max_angle_vpiv=$(( max_angle * 10 ))
    send_vpiv "\$F:MicroSE:direction:*:${max_angle_vpiv}#"
    log_bal "direction envoyée : ${max_angle_vpiv} (soit ${max_angle}°)"
    else
        log_bal "Aucune source sonore détectée. direction non envoyée."
    fi

    # Retour position centrale (0° = servo_val 90)
    set_servo_tgd 0
    log_bal "Servo TGD repositionné à 0°."
    log_bal "=== Fin du balayage sonore ==="
}

main