#!/bin/bash
# =============================================================================
# SCRIPT: rz_gyro_manager.sh
# MODULE: Gyro (SE)
#
# DESCRIPTION GENERALE
# --------------------
# Implémente strictement les propriétés définies dans la Table A :
#
# PROPRIETES SP → SE :
#   - modeGyro        (enum OFF|DATACont|DATAMoy|ALERTE)
#   - paraGyro        (object)
#   - angleVSEBase    (number deg×10)
#
# PROPRIETES SE → SP :
#   - ACK modeGyro
#   - ACK paraGyro
#   - ACK angleVSEBase
#   - dataContGyro    (flux si modeGyro=DATACont)
#   - dataMoyGyro     (flux si modeGyro=DATAMoy)
#   - statusGyro      (événement)
#
# REMARQUES IMPORTANTES
# ---------------------
# - AUCUN renommage par rapport à la Table A
# - Les flux respectent les VPIV définis
# - Code volontairement verbeux
# - statusGyro envoyé uniquement si changement ou alerte
# 
#Version 2024-06 : Ajout de commentaires détaillés et structuration du code pour une meilleure lisibilité.
# auteur: philippeVincent
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_FILE="$BASE_DIR/config/courant_init.json"
LOG_FILE="$BASE_DIR/logs/gyro.log"

# ===========================
# VARIABLES INTERNES
# ===========================

modeGyro="OFF"
angleVSEBase=0
statusGyro="OFF"

# Paramètres paraGyro
freqCont=10
freqMoy=2
nbValPourMoy=5
seuilMesureX=0
seuilMesureY=0
seuilMesureZ=0
seuilAlerteX=300
seuilAlerteY=300

# ===========================
# LOG
# ===========================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [GYRO] $1" >> "$LOG_FILE"
}

# ===========================
# CHARGEMENT CONFIG
# ===========================

load_config() {

    modeGyro=$(jq -r '.gyro.modeGyro' "$CONFIG_FILE")

    freqCont=$(jq -r '.gyro.paraGyro.freqCont' "$CONFIG_FILE")
    freqMoy=$(jq -r '.gyro.paraGyro.freqMoy' "$CONFIG_FILE")
    nbValPourMoy=$(jq -r '.gyro.paraGyro.nbValPourMoy' "$CONFIG_FILE")

    seuilMesureX=$(jq -r '.gyro.paraGyro.seuilMesure.x' "$CONFIG_FILE")
    seuilMesureY=$(jq -r '.gyro.paraGyro.seuilMesure.y' "$CONFIG_FILE")
    seuilMesureZ=$(jq -r '.gyro.paraGyro.seuilMesure.z' "$CONFIG_FILE")

    seuilAlerteX=$(jq -r '.gyro.paraGyro.seuilAlerte.x' "$CONFIG_FILE")
    seuilAlerteY=$(jq -r '.gyro.paraGyro.seuilAlerte.y' "$CONFIG_FILE")

    angleVSEBase=$(jq -r '.gyro.angleVSEBase' "$CONFIG_FILE")

    # ACK vers SP
    echo "\$I:Gyro:modeGyro:*:$modeGyro#"
    echo "\$I:Gyro:paraGyro:*:{freqCont:$freqCont,freqMoy:$freqMoy}#"
    echo "\$I:Gyro:angleVSEBase:*:$angleVSEBase#"
}

# ===========================
# STATUS MANAGEMENT
# ===========================

update_status() {

    new_status="$1"

    if [ "$new_status" != "$statusGyro" ]; then
        statusGyro="$new_status"
        echo "\$E:Gyro:statusGyro:*:$statusGyro#"
        log "statusGyro -> $statusGyro"
    fi
}

# ===========================
# SIMULATION MESURE
# ===========================

simulate_measure() {

    x=$((RANDOM % 600 - 300))
    y=$((RANDOM % 600 - 300))
    z=$((RANDOM % 600 - 300))
}

# ===========================
# FILTRAGE BRUIT
# ===========================

apply_noise_filter() {

    [ "${x#-}" -lt "$seuilMesureX" ] && x=0
    [ "${y#-}" -lt "$seuilMesureY" ] && y=0
    [ "${z#-}" -lt "$seuilMesureZ" ] && z=0
}

# ===========================
# MODE DATACont
# ===========================

mode_dataCont() {

    period=$(awk "BEGIN {print 1/$freqCont}")

    update_status "OK"

    while [ "$modeGyro" = "DATACont" ]; do

        sleep "$period"

        simulate_measure
        apply_noise_filter

        echo "\$F:Gyro:dataContinueGyro:*:[$x,$y,$z]#"
    done
}

# ===========================
# MODE DATAMoy
# ===========================

mode_dataMoy() {

    period=$(awk "BEGIN {print 1/$freqMoy}")

    update_status "OK"

    while [ "$modeGyro" = "DATAMoy" ]; do

        sleep "$period"

        count=0
        sumX=0; sumY=0; sumZ=0

        while [ "$count" -lt "$nbValPourMoy" ]; do

            simulate_measure
            apply_noise_filter

            if [ "$x" -ne 0 ] || [ "$y" -ne 0 ] || [ "$z" -ne 0 ]; then
                sumX=$((sumX + x))
                sumY=$((sumY + y))
                sumZ=$((sumZ + z))
                count=$((count + 1))
            fi
        done

        avgX=$((sumX / nbValPourMoy))
        avgY=$((sumY / nbValPourMoy))
        avgZ=$((sumZ / nbValPourMoy))

        echo "\$F:Gyro:dataMoyenneGyro:*:[$avgX,$avgY,$avgZ]#"
    done
}

# ===========================
# MODE ALERTE
# ===========================

mode_alerte() {

    update_status "OK"

    while [ "$modeGyro" = "ALERTE" ]; do

        simulate_measure
        apply_noise_filter

        # Correction tangage via angleVSEBase
        y_corr=$((y - angleVSEBase))

        if [ "${x#-}" -gt "$seuilAlerteX" ] || \
           [ "${y_corr#-}" -gt "$seuilAlerteY" ]; then

            update_status "ALERTE"

        else
            update_status "OK"
        fi

        sleep 0.1
    done
}

# ===========================
# MAIN
# ===========================

load_config

case "$modeGyro" in
    OFF)
        update_status "OFF"
        ;;
    DATACont)
        mode_dataCont
        ;;
    DATAMoy)
        mode_dataMoy
        ;;
    ALERTE)
        mode_alerte
        ;;
esac