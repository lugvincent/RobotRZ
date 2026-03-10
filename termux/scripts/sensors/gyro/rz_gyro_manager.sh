#!/bin/bash
# =============================================================================
# SCRIPT  : rz_gyro_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/gyro/rz_gyro_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire du capteur gyroscope du smartphone (SE).
# Fait le pont entre le capteur Android (via termux-sensor) et SP (via MQTT/VPIV).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Charge la configuration depuis gyro_runtime.json (généré par check_config.sh)
#    ou depuis courant_init.json en fallback.
# 2. Envoie les ACK de réception à SP pour modeGyro, paraGyro, angleVSEBase.
# 3. Selon le mode actif, publie les données via la FIFO fifo_termux_in :
#       - DATACont : flux brut à freqCont Hz  → $F:Gyro:dataContGyro:*:[x,y,z]#
#       - DATAMoy  : flux moyenne à freqMoy Hz → $F:Gyro:dataMoyGyro:*:[x,y,z]#
#       - ALERTE   : surveille les seuils      → $E:Gyro:statusGyro:*:ALERTE#
#       - OFF      : aucun flux, signale OFF
# 4. Envoie statusGyro (EVT) uniquement en cas de changement d'état ou d'alerte.
#
# FLUX DE DONNÉES
# ---------------
#   Manager → fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#   (Le manager n'appelle JAMAIS mosquitto_pub directement)
#
# TABLE A - PROPRIETES IMPLÉMENTÉES
# -----------------------------------
# SP → SE (réception depuis courant_init.json / gyro_runtime.json) :
#   modeGyro     enum  OFF|DATACont|DATAMoy|ALERTE
#   paraGyro     obj   {freqCont, freqMoy, seuilMesure{x,y,z}, seuilAlerte{x,y}, nbValPourMoy}
#   angleVSEBase num   deg×10  (angle vertical machine BASE+THB, correction tangage)
#
# SE → SP (publication MQTT) :
#   modeGyro     I   ACK réception du mode
#   paraGyro     I   ACK réception des paramètres (image réelle utilisée)
#   angleVSEBase I   ACK réception de l'angle
#   dataContGyro F   [x,y,z] rad/s×100  (FLUX, si modeGyro=DATACont)
#   dataMoyGyro  F   [x,y,z] rad/s×100  (FLUX, si modeGyro=DATAMoy)
#   statusGyro   E   OFF|OK|ALERTE|ERROR (EVT, uniquement si changement)
#
# ARTICULATION AVEC LES AUTRES SCRIPTS
# --------------------------------------
# - check_config.sh      : génère gyro_runtime.json depuis courant_init.json (à lancer avant)
# - courant_init.sh      : (dev) simule l'envoi SP, génère courant_init.json
# - rz_vpiv_parser.sh    : reçoit les commandes MQTT de SP, peut mettre à jour la config
# - rz_state-manager.sh  : gère les modes globaux (modeRZ) qui peuvent bloquer le gyro
# - error_handler.sh     : pour signaler les urgences si défaut capteur
#
# PRECAUTIONS
# -----------
# - Le capteur gyroscope n'est pas disponible sur tous les smartphones.
#   En cas d'absence, le script bascule en simulation (à désactiver en prod).
# - Les valeurs rad/s sont multipliées par 100 pour éviter les flottants (règle Table A).
# - angleVSEBase est envoyé par SP comme flux (CAT=F) : dynamique hors paraGyro.
#   Ce script le lit depuis la config au démarrage. Pas de mise à jour à chaud ici (v1).
# - Le script tourne en boucle infinie. Lancer en arrière-plan ou via un service.
#
# DEPENDANCES
# -----------
#   - jq           : manipulation JSON
#   - termux-sensor: lecture capteur Android (optionnel, fallback simulation)
#   - awk           : calcul période (flottant)
#   - fifo_termux_in: pipe nommée créée par fifo_manager.sh (obligatoire)
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (Conformité Table A - VPIV corrigés - MQTT réel)
# DATE    : 2026-03-03
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

# Détection dynamique : smartphone ou PC de développement
if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

# Fichier runtime généré par check_config.sh (source principale au démarrage)
RUNTIME_JSON="$BASE_DIR/config/gyro_runtime.json"

# Fallback si gyro_runtime.json absent (dev sans check_config.sh)
COURANT_JSON="$BASE_DIR/config/courant_init.json"

LOG_FILE="$BASE_DIR/logs/gyro.log"

# FIFO de sortie : le manager écrit ici, rz_vpiv_parser.sh lit et publie MQTT
# Architecture : Manager → fifo_termux_in → Parser → MQTT → SP
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"

# =============================================================================
# VALEURS PAR DÉFAUT (écrasées par load_config)
# =============================================================================

modeGyro="OFF"
angleVSEBase=0
statusGyro=""       # Vide au démarrage pour forcer le premier envoi statusGyro

# Paramètres paraGyro
freqCont=10         # Fréquence flux continu en Hz
freqMoy=2           # Fréquence flux moyenne en Hz
nbValPourMoy=5      # Nombre de valeurs pour calculer la moyenne
seuilMesureX=10     # Seuil filtrage bruit axe X (rad/s×100) - valeurs < ignorées
seuilMesureY=10     # Seuil filtrage bruit axe Y
seuilMesureZ=5      # Seuil filtrage bruit axe Z
seuilAlerteX=300    # Seuil déclenchement alerte axe X (roulis)
seuilAlerteY=300    # Seuil déclenchement alerte axe Y (tangage, après correction angleVSEBase)

# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

# Journalisation dans le fichier log
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [GYRO] $1" >> "$LOG_FILE"
}

# Envoi d'une trame VPIV vers le parser via la FIFO fifo_termux_in.
# Le parser (rz_vpiv_parser.sh) lit cette FIFO et publie sur MQTT.
# Architecture : Manager → FIFO → Parser → MQTT → SP
# Usage : send_vpiv "$trame_vpiv"
send_vpiv() {
    local trame="$1"

    # Vérification que la FIFO existe et est bien une pipe nommée
    if [ ! -p "$FIFO_OUT" ]; then
        log "WARN : FIFO $FIFO_OUT absente ou invalide. Trame perdue : $trame"
        return 1
    fi

    # Écriture non-bloquante dans la FIFO (timeout 2s pour éviter le blocage
    # si personne ne lit de l'autre côté)
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &   # Watchdog anti-blocage
    wait "$pid_write" 2>/dev/null

    log "FIFO → $trame"
}

# =============================================================================
# CHARGEMENT DE LA CONFIGURATION
# =============================================================================
# Lit gyro_runtime.json si présent (généré par check_config.sh),
# sinon lit courant_init.json directement (fallback développement).
# Envoie les ACK VPIV à SP après chargement.

load_config() {

    local src_file

    # --- Sélection source de config ---
    if [ -f "$RUNTIME_JSON" ]; then
        src_file="$RUNTIME_JSON"
        log "Config source : gyro_runtime.json"
    elif [ -f "$COURANT_JSON" ]; then
        src_file="$COURANT_JSON"
        log "WARN : gyro_runtime.json absent, fallback sur courant_init.json"
    else
        log "ERREUR CRITIQUE : Aucun fichier de config trouvé. Arrêt."
        send_vpiv "\$E:Gyro:statusGyro:*:ERROR#"
        exit 1
    fi

    # --- Lecture du mode ---
    modeGyro=$(jq -r '.modeGyro // .gyro.modeGyro // "OFF"' "$src_file")

    # --- Lecture paraGyro ---
    # Chemin différent selon que c'est gyro_runtime.json ou courant_init.json
    local para_prefix
    if [ "$src_file" = "$RUNTIME_JSON" ]; then
        para_prefix=""       # gyro_runtime.json : champs à la racine
    else
        para_prefix=".gyro"  # courant_init.json : champs sous .gyro
    fi

    freqCont=$(jq -r "${para_prefix}.paraGyro.freqCont // 10"        "$src_file")
    freqMoy=$(jq -r "${para_prefix}.paraGyro.freqMoy // 2"           "$src_file")
    nbValPourMoy=$(jq -r "${para_prefix}.paraGyro.nbValPourMoy // 5"  "$src_file")
    seuilMesureX=$(jq -r "${para_prefix}.paraGyro.seuilMesure.x // 10" "$src_file")
    seuilMesureY=$(jq -r "${para_prefix}.paraGyro.seuilMesure.y // 10" "$src_file")
    seuilMesureZ=$(jq -r "${para_prefix}.paraGyro.seuilMesure.z // 5"  "$src_file")
    seuilAlerteX=$(jq -r "${para_prefix}.paraGyro.seuilAlerte.x // 300" "$src_file")
    seuilAlerteY=$(jq -r "${para_prefix}.paraGyro.seuilAlerte.y // 300" "$src_file")
    angleVSEBase=$(jq -r "${para_prefix}.angleVSEBase // 0"            "$src_file")

    log "Config chargée : mode=$modeGyro freqCont=$freqCont freqMoy=$freqMoy nbVal=$nbValPourMoy angle=$angleVSEBase"
    log "  seuilMesure=[${seuilMesureX},${seuilMesureY},${seuilMesureZ}] seuilAlerte=[${seuilAlerteX},${seuilAlerteY}]"

    # --- ACK modeGyro → SP (Table A : $I:Gyro:modeGyro:*:<mode>#) ---
    send_vpiv "\$I:Gyro:modeGyro:*:${modeGyro}#"

    # --- ACK paraGyro → SP : image réelle des paramètres utilisés (Table A : $I:Gyro:paraGyro:*:{...}#) ---
    # On renvoie l'objet complet avec tous les champs pour que SP puisse vérifier
    local ack_para
    ack_para="{\"freqCont\":${freqCont},\"freqMoy\":${freqMoy},\"seuilMesure\":[${seuilMesureX},${seuilMesureY},${seuilMesureZ}],\"seuilAlerte\":[${seuilAlerteX},${seuilAlerteY}],\"nbValPourMoy\":${nbValPourMoy}}"
    send_vpiv "\$I:Gyro:paraGyro:*:${ack_para}#"

    # --- ACK angleVSEBase → SP (Table A : $I:Gyro:angleVSEBase:*:<val>#) ---
    send_vpiv "\$I:Gyro:angleVSEBase:*:${angleVSEBase}#"
}

# =============================================================================
# GESTION DU STATUS GYRO
# =============================================================================
# Envoie statusGyro UNIQUEMENT si l'état change (évite le flood MQTT).
# Table A : $E:Gyro:statusGyro:*:OFF|OK|ALERTE|ERROR#

update_status() {
    local new_status="$1"

    if [ "$new_status" != "$statusGyro" ]; then
        statusGyro="$new_status"
        send_vpiv "\$E:Gyro:statusGyro:*:${statusGyro}#"
        log "statusGyro → $statusGyro"
    fi
}

# =============================================================================
# LECTURE CAPTEUR RÉEL (termux-sensor)
# =============================================================================
# Lit le gyroscope Android et remplit les variables x, y, z en rad/s×100.
# Retourne 0 si lecture OK, 1 si capteur indisponible (bascule simulation).
# Règle Table A : pas de float → multiplication par 100, arrondi à l'entier.

read_real_sensor() {
    local raw
    # Test direct du code retour de termux-sensor (ShellCheck SC2181)
    if ! raw=$(termux-sensor -s "gyroscope" -n 1 2>/dev/null) || [ -z "$raw" ]; then
        return 1   # Capteur indisponible
    fi

    # Conversion rad/s → rad/s×100 (entier, pas de float)
    x=$(echo "$raw" | jq -r '.values[0] * 100 | round | tonumber' 2>/dev/null)
    y=$(echo "$raw" | jq -r '.values[1] * 100 | round | tonumber' 2>/dev/null)
    z=$(echo "$raw" | jq -r '.values[2] * 100 | round | tonumber' 2>/dev/null)

    # Vérification que les valeurs sont bien numériques
    if [ -z "$x" ] || [ -z "$y" ] || [ -z "$z" ]; then
        return 1
    fi

    return 0
}

# =============================================================================
# SIMULATION MESURE (fallback développement)
# =============================================================================
# Génère des valeurs aléatoires dans la plage ±300 rad/s×100.
# À DÉSACTIVER en production réelle (remplacer par read_real_sensor uniquement).

simulate_measure() {
    x=$((RANDOM % 600 - 300))
    y=$((RANDOM % 600 - 300))
    z=$((RANDOM % 600 - 300))
}

# =============================================================================
# ACQUISITION D'UNE MESURE (capteur réel ou simulation)
# =============================================================================
# Tente le capteur réel. Si indisponible, utilise la simulation.
# Remplit les variables globales x, y, z.

acquire_measure() {
    if ! read_real_sensor; then
        # Capteur indisponible : simulation (développement)
        simulate_measure
        # Déscommenter la ligne suivante pour signaler l'absence de capteur en prod :
        # update_status "ERROR" && return
    fi
}

# =============================================================================
# FILTRAGE BRUIT
# =============================================================================
# Si la valeur absolue est inférieure au seuil de mesure → mise à zéro.
# Règle Table A : moyenne calculée uniquement sur valeurs > seuilMesure.

apply_noise_filter() {
    # ${x#-} = valeur absolue de x (supprime le signe moins éventuel)
    [ "${x#-}" -lt "$seuilMesureX" ] && x=0
    [ "${y#-}" -lt "$seuilMesureY" ] && y=0
    [ "${z#-}" -lt "$seuilMesureZ" ] && z=0
}

# =============================================================================
# MODE DATACont — Flux continu haute fréquence
# =============================================================================
# Publie les valeurs brutes à freqCont Hz.
# Table A : $F:Gyro:dataContGyro:*:[x,y,z]#   (rad/s×100)
# Ici vitesses angulaires brutes, utilisées pour l'odométrie côté SP/Arduino.

mode_dataCont() {
    # Calcul de la période d'attente entre deux envois (en secondes, float)
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqCont}")

    log "Mode DATACont démarré (freqCont=${freqCont}Hz, période=${period}s)"
    update_status "OK"

    while [ "$modeGyro" = "DATACont" ]; do

        sleep "$period"

        acquire_measure
        apply_noise_filter   # Supprime le bruit, pas de seuil alerte en mode continu

        # Publication du flux vers SP (Table A : dataContGyro)
        send_vpiv "\$F:Gyro:dataContGyro:*:[${x},${y},${z}]#"

    done

    log "Mode DATACont terminé (modeGyro changé à $modeGyro)"
}

# =============================================================================
# MODE DATAMoy — Flux de valeurs moyennées
# =============================================================================
# Accumule nbValPourMoy mesures valides (> seuilMesure) puis publie la moyenne.
# Envoi à freqMoy Hz.
# Table A : $F:Gyro:dataMoyGyro:*:[x,y,z]#   (rad/s×100)

mode_dataMoy() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqMoy}")

    log "Mode DATAMoy démarré (freqMoy=${freqMoy}Hz, nbValPourMoy=${nbValPourMoy})"
    update_status "OK"

    while [ "$modeGyro" = "DATAMoy" ]; do

        sleep "$period"

        # Accumulation des mesures valides (non nulles après filtrage)
        local count=0 sumX=0 sumY=0 sumZ=0

        while [ "$count" -lt "$nbValPourMoy" ]; do

            acquire_measure
            apply_noise_filter

            # Règle Table A : n'intégrer dans la moyenne que les valeurs non nulles
            if [ "$x" -ne 0 ] || [ "$y" -ne 0 ] || [ "$z" -ne 0 ]; then
                sumX=$((sumX + x))
                sumY=$((sumY + y))
                sumZ=$((sumZ + z))
                count=$((count + 1))
            fi

        done

        # Calcul de la moyenne entière
        local avgX=$((sumX / nbValPourMoy))
        local avgY=$((sumY / nbValPourMoy))
        local avgZ=$((sumZ / nbValPourMoy))

        # Publication flux moyen vers SP (Table A : dataMoyGyro)
        send_vpiv "\$F:Gyro:dataMoyGyro:*:[${avgX},${avgY},${avgZ}]#"

    done

    log "Mode DATAMoy terminé (modeGyro changé à $modeGyro)"
}

# =============================================================================
# MODE ALERTE — Surveillance des seuils angulaires
# =============================================================================
# Surveille en continu les axes X (roulis) et Y (tangage).
# Le tangage est corrigé par angleVSEBase pour compenser l'inclinaison machine.
# Déclenche statusGyro=ALERTE si un seuil est dépassé.
# Table A : $E:Gyro:statusGyro:*:ALERTE#

mode_alerte() {
    log "Mode ALERTE démarré (seuilAlerteX=$seuilAlerteX, seuilAlerteY=$seuilAlerteY, angleVSEBase=$angleVSEBase)"
    update_status "OK"

    while [ "$modeGyro" = "ALERTE" ]; do

        acquire_measure
        apply_noise_filter

        # Correction tangage : on soustrait l'angle de référence machine (angleVSEBase)
        # pour obtenir l'écart par rapport à la verticale réelle
        local y_corr=$((y - angleVSEBase))

        # Test de dépassement de seuil (valeur absolue)
        if [ "${x#-}" -gt "$seuilAlerteX" ] || \
           [ "${y_corr#-}" -gt "$seuilAlerteY" ]; then
            update_status "ALERTE"
            log "  ALERTE : x=${x} (seuil${seuilAlerteX}) y_corr=${y_corr} (seuil${seuilAlerteY})"
        else
            update_status "OK"
        fi

        sleep 0.1   # Fréquence de scrutation : 10 Hz (fixe en mode alerte)

    done

    log "Mode ALERTE terminé (modeGyro changé à $modeGyro)"
}

# =============================================================================
# MAIN — Point d'entrée principal
# =============================================================================

main() {
    log "=========================================="
    log "Démarrage rz_gyro_manager.sh  v2.0"
    log "=========================================="

    # Vérification des dépendances
    if ! command -v jq &>/dev/null; then
        log "ERREUR CRITIQUE : jq non installé. Arrêt."
        exit 1
    fi

    # Vérification que la FIFO de sortie est accessible
    if [ ! -p "$FIFO_OUT" ]; then
        log "WARN : FIFO $FIFO_OUT absente au démarrage. Lancer fifo_manager.sh create."
        # On continue : la FIFO peut être créée entre-temps
    fi

    # Chargement de la configuration et envoi des ACK vers SP
    load_config

    # Aiguillage vers le mode actif
    log "Lancement mode : $modeGyro"
    case "$modeGyro" in

        OFF)
            update_status "OFF"
            log "Mode OFF : aucun flux. Script terminé normalement."
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

        *)
            # Mode inconnu : signaler l'erreur à SP et sortir
            log "ERREUR : modeGyro inconnu = '$modeGyro'"
            update_status "ERROR"
            exit 1
            ;;
    esac

    log "rz_gyro_manager.sh terminé."
}

# Lancement
main
