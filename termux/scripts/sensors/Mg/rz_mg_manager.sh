#!/bin/bash
# =============================================================================
# SCRIPT  : rz_mg_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/Mg/rz_mg_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire du capteur magnétomètre du smartphone (SE).
# Fait le pont entre le capteur Android (via termux-sensor) et SP (via FIFO/MQTT/VPIV).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Charge la configuration depuis mag_runtime.json (généré par check_config.sh)
#    ou depuis courant_init.json en fallback.
# 2. Envoie les ACK de réception à SP pour modeMg et paraMg.
# 3. Selon le mode actif, publie les données via fifo_termux_in :
#       - DATABrutCont : flux brut continu à freqCont Hz
#                        → $F:Mag:dataContBrut:*:[x,y,z]#  (µT×100)
#       - DATAGeoCont  : cap géographique continu à freqCont Hz
#                        → $F:Mag:DataContGeo:*:<val>#  (degrés×10)
#       - DATAGeoMoy   : cap géographique moyenné à freqMoy Hz
#                        → $F:Mag:DataMoyGeo:*:<val>#  (degrés×10)
#       - DATAMgCont   : cap magnétique continu à freqCont Hz
#                        → $F:Mag:DataContMg:*:<val>#  (degrés×10)
#       - DATAMgMoy    : cap magnétique moyenné à freqMoy Hz
#                        → $F:Mag:DataMoyMg:*:<val>#  (degrés×10)
#       - OFF          : aucun flux
#
# FLUX DE DONNÉES
# ---------------
#   Manager → fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#   (Le manager n'appelle JAMAIS mosquitto_pub directement)
#
# TABLE A - PROPRIÉTÉS IMPLÉMENTÉES
# -----------------------------------
# SP → SE (réception depuis mag_runtime.json) :
#   modeMg    enum  OFF|DATABrutCont|DATAGeoCont|DATAGeoMoy|DATAMgCont|DATAMgMoy
#   paraMg    obj   {freqCont, freqMoy, seuilMesure{x,y,z}, nbValPourMoy}
#
# SE → SP (publication via FIFO) :
#   modeMg        I   ACK réception du mode
#   paraMg        I   ACK réception des paramètres
#   dataContBrut  F   [x,y,z] µT×100            (si DATABrutCont)
#   DataContGeo   F   cap géo degrés×10          (si DATAGeoCont)
#   DataMoyGeo    F   cap géo moyenné degrés×10  (si DATAGeoMoy)
#   DataContMg    F   cap mag degrés×10           (si DATAMgCont)
#   DataMoyMg     F   cap mag moyenné degrés×10   (si DATAMgMoy)
#
# CALCUL DU CAP
# -------------
# Le cap est calculé via atan2(y, x) sur les axes du magnétomètre.
# Résultat en radians converti en degrés×10 (0–3600).
# Pas de correction de déclinaison magnétique dans cette version (v1).
# X = gauche/droite, Y = avant/arrière, Z = inclinaison (même orientation que Gyro).
#
# ARTICULATION AVEC LES AUTRES SCRIPTS
# --------------------------------------
#   check_config.sh     : génère mag_runtime.json avant le démarrage
#   courant_init.sh     : (dev) génère courant_init.json
#   rz_vpiv_parser.sh   : lit fifo_termux_in et publie sur MQTT
#   fifo_manager.sh     : crée les pipes nommées (à lancer avant ce script)
#
# PRÉCAUTIONS
# -----------
# - Le calcul de cap nécessite awk (atan2 non disponible en bash pur).
# - Les valeurs µT sont multipliées par 100 pour éviter les flottants (règle Table A).
# - Le capteur "MagneticField" Android peut varier selon le modèle de smartphone.
#   Tester avec : termux-sensor -s "MagneticField" -n 1
# - En mode Moy, la moyenne est calculée sur nbValPourMoy valeurs valides
#   (valeur absolue > seuilMesure), pas sur un nombre fixe de lectures brutes.
#
# DÉPENDANCES
# -----------
#   - jq            : manipulation JSON
#   - awk           : calcul atan2 pour le cap, calcul période (flottant)
#   - termux-sensor : lecture capteur Android (optionnel, fallback simulation)
#   - fifo_termux_in: pipe nommée créée par fifo_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (Conformité Table A complète - FIFO - 6 modes)
# DATE    : 2026-03-03
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

# Détection dynamique : smartphone Termux ou PC de développement
if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

# Fichier runtime généré par check_config.sh (source principale)
RUNTIME_JSON="$BASE_DIR/config/mag_runtime.json"

# Fallback si mag_runtime.json absent (dev sans check_config.sh)
COURANT_JSON="$BASE_DIR/config/courant_init.json"

LOG_FILE="$BASE_DIR/logs/mag.log"

# FIFO de sortie vers le parser (qui publie sur MQTT)
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"

# =============================================================================
# VALEURS PAR DÉFAUT (écrasées par load_config)
# =============================================================================

modeMg="OFF"

# Paramètres paraMg
freqCont=5          # Fréquence flux continu en Hz
freqMoy=1           # Fréquence flux moyenné en Hz
nbValPourMoy=5      # Nombre de valeurs valides pour la moyenne
seuilMesureX=50     # Seuil filtrage bruit axe X (µT×100)
seuilMesureY=50     # Seuil filtrage bruit axe Y (µT×100)
seuilMesureZ=50     # Seuil filtrage bruit axe Z (µT×100)

# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

# Journalisation horodatée
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [MAG] $1" >> "$LOG_FILE"
}

# Envoi d'une trame VPIV dans la FIFO → parser → MQTT → SP
# Écriture non-bloquante avec watchdog 2s (évite le blocage si parser absent)
send_vpiv() {
    local trame="$1"

    if [ ! -p "$FIFO_OUT" ]; then
        log "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi

    # Écriture en arrière-plan + watchdog anti-blocage
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null

    log "FIFO → $trame"
}

# =============================================================================
# CHARGEMENT DE LA CONFIGURATION
# =============================================================================

load_config() {
    local src_file

    # Sélection source : runtime en priorité, courant_init en fallback
    if [ -f "$RUNTIME_JSON" ]; then
        src_file="$RUNTIME_JSON"
        log "Config source : mag_runtime.json"
    elif [ -f "$COURANT_JSON" ]; then
        src_file="$COURANT_JSON"
        log "WARN : mag_runtime.json absent, fallback sur courant_init.json"
    else
        log "ERREUR CRITIQUE : Aucun fichier de config trouvé. Arrêt."
        exit 1
    fi

    # Préfixe de lecture selon la source
    # mag_runtime.json : champs à la racine sous .mag
    # courant_init.json : champs sous .mag
    modeMg=$(jq -r '.mag.modeMg // "OFF"' "$src_file")

    freqCont=$(jq -r    '.mag.paraMg.freqCont // 5'          "$src_file")
    freqMoy=$(jq -r     '.mag.paraMg.freqMoy // 1'           "$src_file")
    nbValPourMoy=$(jq -r '.mag.paraMg.nbValPourMoy // 5'     "$src_file")
    seuilMesureX=$(jq -r '.mag.paraMg.seuilMesure.x // 50'   "$src_file")
    seuilMesureY=$(jq -r '.mag.paraMg.seuilMesure.y // 50'   "$src_file")
    seuilMesureZ=$(jq -r '.mag.paraMg.seuilMesure.z // 50'   "$src_file")

    log "Config chargée : mode=$modeMg freqCont=$freqCont freqMoy=$freqMoy nbVal=$nbValPourMoy"
    log "  seuilMesure=[${seuilMesureX},${seuilMesureY},${seuilMesureZ}]"

    # --- ACK modeMg → SP ---
    send_vpiv "\$I:Mag:modeMg:*:${modeMg}#"

    # --- ACK paraMg → SP : image réelle des paramètres utilisés ---
    local ack_para
    ack_para="{\"freqCont\":${freqCont},\"freqMoy\":${freqMoy},\"seuilMesure\":[${seuilMesureX},${seuilMesureY},${seuilMesureZ}],\"nbValPourMoy\":${nbValPourMoy}}"
    send_vpiv "\$I:Mag:paraMg:*:${ack_para}#"
}

# =============================================================================
# LECTURE CAPTEUR RÉEL (termux-sensor)
# =============================================================================
# Remplit les variables globales x, y, z en µT×100 (entiers, pas de float).
# Retourne 0 si OK, 1 si capteur indisponible.

read_real_sensor() {
    local raw
    if ! raw=$(termux-sensor -s "MagneticField" -n 1 2>/dev/null) || [ -z "$raw" ]; then
        return 1
    fi

    # Conversion µT → µT×100 (entier) — règle Table A : pas de float
    x=$(echo "$raw" | jq -r '.values[0] * 100 | round | tonumber' 2>/dev/null)
    y=$(echo "$raw" | jq -r '.values[1] * 100 | round | tonumber' 2>/dev/null)
    z=$(echo "$raw" | jq -r '.values[2] * 100 | round | tonumber' 2>/dev/null)

    [ -z "$x" ] || [ -z "$y" ] || [ -z "$z" ] && return 1
    return 0
}

# =============================================================================
# SIMULATION MESURE (fallback développement)
# =============================================================================
# Champ magnétique terrestre typique : ~200–600 µT×100

simulate_measure() {
    x=$((RANDOM % 400 + 200))    # +200 à +600 µT×100 (axe X)
    y=$((RANDOM % 400 - 200))    # -200 à +200 µT×100 (axe Y)
    z=$((RANDOM % 600 - 300))    # ±300 µT×100 (axe Z)
}

# =============================================================================
# ACQUISITION (capteur réel ou simulation)
# =============================================================================

acquire_measure() {
    if ! read_real_sensor; then
        simulate_measure   # Fallback développement
    fi
}

# =============================================================================
# FILTRAGE BRUIT
# =============================================================================
# Valeurs absolues < seuil → mises à zéro (non intégrées dans la moyenne).

apply_noise_filter() {
    [ "${x#-}" -lt "$seuilMesureX" ] && x=0
    [ "${y#-}" -lt "$seuilMesureY" ] && y=0
    [ "${z#-}" -lt "$seuilMesureZ" ] && z=0
}

# =============================================================================
# CALCUL DU CAP (atan2 via awk)
# =============================================================================
# Calcule le cap en degrés×10 (0–3600) à partir des axes X et Y du magnétomètre.
# Formule : cap = atan2(y, x) converti en degrés, normalisé 0–360, ×10.
# Pas de correction de déclinaison magnétique (v1).
# Résultat dans la variable globale : cap_deg10

compute_cap() {
    cap_deg10=$(awk -v x="$x" -v y="$y" '
    BEGIN {
        PI = 3.14159265358979
        # atan2 en radians, converti en degrés
        cap_rad = atan2(y, x)
        cap_deg = cap_rad * 180 / PI
        # Normalisation 0–360°
        if (cap_deg < 0) cap_deg += 360
        # Multiplication ×10 pour éviter les flottants (règle Table A)
        printf "%d", cap_deg * 10
    }')
}

# =============================================================================
# MODE DATABrutCont — Flux brut continu
# =============================================================================
# Publie [x,y,z] µT×100 à freqCont Hz sans filtrage.
# Table A : $F:Mag:dataContBrut:*:[x,y,z]#

mode_dataBrutCont() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqCont}")
    log "Mode DATABrutCont démarré (freqCont=${freqCont}Hz)"

    while [ "$modeMg" = "DATABrutCont" ]; do
        sleep "$period"
        acquire_measure
        # Pas de filtrage en mode brut : on envoie les valeurs telles quelles
        send_vpiv "\$F:Mag:dataContBrut:*:[${x},${y},${z}]#"
    done

    log "Mode DATABrutCont terminé"
}

# =============================================================================
# MODE DATAGeoCont — Cap géographique continu
# =============================================================================
# Calcule et publie le cap géo à freqCont Hz.
# Note v1 : cap géo = cap mag (pas de correction déclinaison magnétique).
# Table A : $F:Mag:DataContGeo:*:<val>#  (degrés×10)

mode_dataGeoCont() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqCont}")
    log "Mode DATAGeoCont démarré (freqCont=${freqCont}Hz)"

    while [ "$modeMg" = "DATAGeoCont" ]; do
        sleep "$period"
        acquire_measure
        apply_noise_filter
        compute_cap
        send_vpiv "\$F:Mag:DataContGeo:*:${cap_deg10}#"
    done

    log "Mode DATAGeoCont terminé"
}

# =============================================================================
# MODE DATAGeoMoy — Cap géographique moyenné
# =============================================================================
# Accumule nbValPourMoy mesures valides puis publie la moyenne du cap à freqMoy Hz.
# Table A : $F:Mag:DataMoyGeo:*:<val>#  (degrés×10)

mode_dataGeoMoy() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqMoy}")
    log "Mode DATAGeoMoy démarré (freqMoy=${freqMoy}Hz, nbVal=${nbValPourMoy})"

    while [ "$modeMg" = "DATAGeoMoy" ]; do
        sleep "$period"

        local count=0 sumCap=0

        while [ "$count" -lt "$nbValPourMoy" ]; do
            acquire_measure
            apply_noise_filter

            # N'intégrer que les mesures non nulles (au moins un axe non nul)
            if [ "$x" -ne 0 ] || [ "$y" -ne 0 ]; then
                compute_cap
                sumCap=$((sumCap + cap_deg10))
                count=$((count + 1))
            fi
        done

        local avgCap=$((sumCap / nbValPourMoy))
        send_vpiv "\$F:Mag:DataMoyGeo:*:${avgCap}#"
    done

    log "Mode DATAGeoMoy terminé"
}

# =============================================================================
# MODE DATAMgCont — Cap magnétique continu
# =============================================================================
# Calcule et publie le cap magnétique brut à freqCont Hz.
# Table A : $F:Mag:DataContMg:*:<val>#  (degrés×10)

mode_dataMgCont() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqCont}")
    log "Mode DATAMgCont démarré (freqCont=${freqCont}Hz)"

    while [ "$modeMg" = "DATAMgCont" ]; do
        sleep "$period"
        acquire_measure
        apply_noise_filter
        compute_cap
        send_vpiv "\$F:Mag:DataContMg:*:${cap_deg10}#"
    done

    log "Mode DATAMgCont terminé"
}

# =============================================================================
# MODE DATAMgMoy — Cap magnétique moyenné
# =============================================================================
# Accumule nbValPourMoy mesures valides puis publie la moyenne du cap à freqMoy Hz.
# Table A : $F:Mag:DataMoyMg:*:<val>#  (degrés×10)

mode_dataMgMoy() {
    local period
    period=$(awk "BEGIN {printf \"%.4f\", 1/$freqMoy}")
    log "Mode DATAMgMoy démarré (freqMoy=${freqMoy}Hz, nbVal=${nbValPourMoy})"

    while [ "$modeMg" = "DATAMgMoy" ]; do
        sleep "$period"

        local count=0 sumCap=0

        while [ "$count" -lt "$nbValPourMoy" ]; do
            acquire_measure
            apply_noise_filter

            if [ "$x" -ne 0 ] || [ "$y" -ne 0 ]; then
                compute_cap
                sumCap=$((sumCap + cap_deg10))
                count=$((count + 1))
            fi
        done

        local avgCap=$((sumCap / nbValPourMoy))
        send_vpiv "\$F:Mag:DataMoyMg:*:${avgCap}#"
    done

    log "Mode DATAMgMoy terminé"
}

# =============================================================================
# MAIN
# =============================================================================

main() {
    log "=========================================="
    log "Démarrage rz_mg_manager.sh  v2.0"
    log "=========================================="

    # Vérification dépendances
    if ! command -v jq &>/dev/null; then
        log "ERREUR CRITIQUE : jq non installé. Arrêt."
        exit 1
    fi
    if ! command -v awk &>/dev/null; then
        log "ERREUR CRITIQUE : awk non installé. Arrêt."
        exit 1
    fi

    # Vérification FIFO
    if [ ! -p "$FIFO_OUT" ]; then
        log "WARN : FIFO $FIFO_OUT absente. Lancer fifo_manager.sh create."
    fi

    # Chargement config et envoi ACK
    load_config

    # Aiguillage vers le mode actif
    log "Lancement mode : $modeMg"
    case "$modeMg" in
        OFF)
            log "Mode OFF : aucun flux. Script terminé normalement."
            ;;
        DATABrutCont)
            mode_dataBrutCont
            ;;
        DATAGeoCont)
            mode_dataGeoCont
            ;;
        DATAGeoMoy)
            mode_dataGeoMoy
            ;;
        DATAMgCont)
            mode_dataMgCont
            ;;
        DATAMgMoy)
            mode_dataMgMoy
            ;;
        *)
            log "ERREUR : modeMg inconnu = '$modeMg'"
            exit 1
            ;;
    esac

    log "rz_mg_manager.sh terminé."
}

main