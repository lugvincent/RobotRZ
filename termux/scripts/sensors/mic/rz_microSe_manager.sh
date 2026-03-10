#!/bin/bash
# =============================================================================
# SCRIPT  : rz_microSe_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/mic/rz_microSe_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire principal du microphone SE (smartphone embarqué).
# Mesure le niveau sonore RMS via ffmpeg, applique un filtre fenêtre glissante,
# gère les seuils d'intensité et pilote le balayage d'orientation sonore.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Script daemon (boucle permanente). Lit paraMicro depuis mic_config.json
# (généré par check_config.sh) et adapte son comportement selon modeMicro.
#
# MODES (modeMicro)
# -----------------
#   off         : arrêt enregistrement, aucun flux VPIV
#   normal      : dataContRMS publié à chaque mesure (freqCont ms)
#   intensite   : dataContRMS si delta > seuilDelta + (alerte) si seuil franchi
#                 pendant holdTime ms (amortisseur bruit de fond)
#   orientation : balayage servo TGD → direction (angle max RMS)
#   surveillance: intensite + balayage auto si RMS > seuilMoyen → direction
#
# MESURE RMS
# ----------
# termux-microphone-record enregistre winSize ms dans un fichier WAV temporaire.
# ffmpeg calcule le niveau RMS via astats.
# Valeur retournée : RMS×10 (entier 0–1000) pour éviter les flottants.
#
# FENÊTRE GLISSANTE & FILTRE DELTA
# ----------------------------------
# Tableau circulaire de N dernières mesures (N = winSize/freqCont).
# Moyenne glissante = lissage bruit de fond.
# Envoi dataContRMS uniquement si |moyenne_actuelle - dernière_envoyée| > seuilDelta.
#
# SYSTÈME D'ALERTE (holdTime)
# ----------------------------
# Un seuil est considéré "franchi" uniquement s'il est maintenu >= holdTime ms.
# Évite les faux déclenchements sur pics courts (choc, bruit ponctuel).
# (alerte) envoyé à chaque CHANGEMENT de niveau (montée ET retour silence).
#
# TABLE A — VPIV PRODUITS
# -----------------------
# SP → SE :
#   $V:MicroSE:modeMicro:*:intensite#
#   $V:MicroSE:paraMicro:*:{...}#
#
# SE → SP :
#   $I:MicroSE:modeMicro:*:intensite#       ACK mode
#   $I:MicroSE:paraMicro:*:{...}#           ACK paramètres
#   $F:MicroSE:dataContRMS:*:342#           Niveau RMS (0–1000)
#   $E:MicroSE:(alerte):*:fort#             Seuil franchi (faible|moyen|fort|silence)
#   $F:MicroSE:direction:*:35#              Angle orientation (orientation/surveillance)
#   $I:COM:error:SE:"MicroSE : ..."#        Erreur non critique
#
# INTERACTIONS
# ------------
#   Appelé par  : bootstrap SE au démarrage (daemon)
#   Lit         : config/sensors/mic_config.json
#   Lit         : config/global.json (.CfgS.modeRZ, .CfgS.typePtge)
#   Lance       : rz_balayage_sonore.sh (modes orientation/surveillance)
#   Écrit dans  : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# - termux-microphone-record + ffmpeg requis (pkg install ffmpeg termux-api).
# - Un seul enregistrement actif à la fois (LOCK_FILE).
# - winSize >= freqCont (sinon fenêtre glissante sans sens).
# - Le balayage est lancé en sous-shell non bloquant (&).
#   Un seul balayage à la fois (vérifié via PID_BALAYAGE).
# - ⚠️ WAV_TEMP partagé avec rz_balayage_sonore.sh : ne pas changer le chemin.
#
# DÉPENDANCES
# -----------
#   jq, ffmpeg, termux-microphone-record (termux-api), bc
#   fifo_termux_in : créé par fifo_manager.sh
#   rz_balayage_sonore.sh : dans le même répertoire
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (Table A complète, fenêtre glissante, holdTime, delta filtre,
#                  5 modes, balayage via VPIV Srv:TGD, sans mosquitto_pub)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

MIC_CONFIG="$BASE_DIR/config/sensors/mic_config.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/microse_manager.log"
LOCK_FILE="$BASE_DIR/logs/microse.lock"
WAV_TEMP="/tmp/microse_measure.wav"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# =============================================================================
# VALEURS PAR DÉFAUT (écrasées par load_config)
# =============================================================================

modeMicro="off"
freqCont=300        # ms — période mesure RMS
winSize=300         # ms — durée enregistrement WAV par mesure
seuilDelta=30       # RMS×10 — delta min pour envoi dataContRMS
holdTime=500        # ms — durée maintien seuil avant (alerte)
seuilFaible=100     # RMS×10
seuilMoyen=400      # RMS×10
seuilFort=700       # RMS×10
pasBalayage=10      # degrés
timeoutOrientation=15  # secondes

# =============================================================================
# ÉTAT INTERNE (variables de suivi entre cycles)
# =============================================================================

derniere_rms_envoyee=0          # Dernière valeur dataContRMS envoyée (filtre delta)
niveau_actuel="silence"         # Niveau courant : silence|faible|moyen|fort
niveau_precedent="silence"      # Pour détecter les changements
timestamp_seuil=0               # Timestamp (ms) du début du dépassement de seuil
PID_BALAYAGE=0                  # PID du balayage en cours (0 = aucun)

# Tableau fenêtre glissante (valeurs RMS récentes)
declare -a fenetre_rms=()
NB_FENETRE=5    # Recalculé après load_config

# =============================================================================
# UTILITAIRES
# =============================================================================

log_mic() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [MICRO] $1" >> "$LOG_FILE"
}

# Timestamp en millisecondes
now_ms() {
    date +%s%3N
}

# Envoi VPIV via FIFO (non-bloquant, watchdog 2s)
send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_OUT" ]; then
        log_mic "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null
}

# =============================================================================
# CHARGEMENT CONFIGURATION depuis mic_config.json
# =============================================================================

load_config() {
    if [ ! -f "$MIC_CONFIG" ]; then
        log_mic "WARN : mic_config.json absent. Valeurs par défaut utilisées."
        return 1
    fi

    modeMicro=$(   jq -r '.mic.modeMicro          // "off"' "$MIC_CONFIG")
    freqCont=$(    jq -r '.mic.paraMicro.freqCont  // 300'  "$MIC_CONFIG")
    winSize=$(     jq -r '.mic.paraMicro.winSize   // 300'  "$MIC_CONFIG")
    seuilDelta=$(  jq -r '.mic.paraMicro.seuilDelta // 30'  "$MIC_CONFIG")
    holdTime=$(    jq -r '.mic.paraMicro.holdTime  // 500'  "$MIC_CONFIG")
    seuilFaible=$( jq -r '.mic.paraMicro.seuilFaible // 100' "$MIC_CONFIG")
    seuilMoyen=$(  jq -r '.mic.paraMicro.seuilMoyen  // 400' "$MIC_CONFIG")
    seuilFort=$(   jq -r '.mic.paraMicro.seuilFort   // 700' "$MIC_CONFIG")
    pasBalayage=$( jq -r '.mic.paraMicro.pasBalayage // 10'  "$MIC_CONFIG")
    timeoutOrientation=$(jq -r '.mic.paraMicro.timeoutOrientation // 15' "$MIC_CONFIG")

    # Taille fenêtre glissante : winSize / freqCont (min 2)
    NB_FENETRE=$(( winSize / freqCont ))
    (( NB_FENETRE < 2 )) && NB_FENETRE=2

    log_mic "Config chargée : mode=$modeMicro freqCont=${freqCont}ms winSize=${winSize}ms"
    log_mic "  seuilDelta=$seuilDelta holdTime=${holdTime}ms"
    log_mic "  seuils : faible=$seuilFaible moyen=$seuilMoyen fort=$seuilFort"
    log_mic "  balayage : pas=${pasBalayage}° timeout=${timeoutOrientation}s NB_FENETRE=$NB_FENETRE"

    # ACK modeMicro → SP
    send_vpiv "\$I:MicroSE:modeMicro:*:${modeMicro}#"

    # ACK paraMicro → SP
    local ack
    ack="{\"freqCont\":${freqCont},\"winSize\":${winSize},\"seuilDelta\":${seuilDelta},"
    ack+="\"holdTime\":${holdTime},\"seuilFaible\":${seuilFaible},"
    ack+="\"seuilMoyen\":${seuilMoyen},\"seuilFort\":${seuilFort},"
    ack+="\"pasBalayage\":${pasBalayage},\"timeoutOrientation\":${timeoutOrientation}}"
    send_vpiv "\$I:MicroSE:paraMicro:*:${ack}#"
}

# =============================================================================
# MESURE RMS
# Enregistre winSize ms via termux-microphone-record puis analyse via ffmpeg.
# Retourne RMS×10 (entier 0–1000).
# =============================================================================

read_rms() {
    local duration_s
    # Conversion winSize ms → secondes avec 2 décimales pour termux-microphone-record
    duration_s=$(echo "scale=2; $winSize / 1000" | bc)

    # Enregistrement WAV (supprime le fichier précédent)
    rm -f "$WAV_TEMP"
    termux-microphone-record -d "$duration_s" -f "$WAV_TEMP" 2>/dev/null

    if [ ! -f "$WAV_TEMP" ]; then
        log_mic "WARN : enregistrement WAV échoué"
        echo "0"
        return
    fi

    # Analyse RMS via ffmpeg astats (retourne valeur en dBFS, on extrait la puissance)
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

    # Conversion dBFS → valeur linéaire ×10 (0–1000)
    # RMS en dBFS est négatif : -60dBFS = silence, 0dBFS = max
    # Formule : val = (100 + rms_db) * 10  → clampé 0–1000
    local rms_int
    rms_int=$(echo "$rms_raw" | awk '{
        val = (100 + $1) * 10
        if (val < 0)    val = 0
        if (val > 1000) val = 1000
        printf "%d", val
    }')
    echo "${rms_int:-0}"
}

# =============================================================================
# FENÊTRE GLISSANTE
# Ajoute une mesure, retourne la moyenne des NB_FENETRE dernières valeurs.
# =============================================================================

update_fenetre() {
    local nouvelle_val="$1"

    # Ajout en fin de tableau
    fenetre_rms+=("$nouvelle_val")

    # Suppression des valeurs les plus anciennes si dépassement taille
    while (( ${#fenetre_rms[@]} > NB_FENETRE )); do
        fenetre_rms=("${fenetre_rms[@]:1}")
    done

    # Calcul moyenne
    local somme=0
    for val in "${fenetre_rms[@]}"; do
        somme=$(( somme + val ))
    done
    echo $(( somme / ${#fenetre_rms[@]} ))
}

# =============================================================================
# CLASSIFICATION NIVEAU SONORE
# $1 = valeur RMS×10
# Retourne : silence | faible | moyen | fort
# =============================================================================

get_niveau() {
    local rms="$1"
    if   (( rms >= seuilFort   )); then echo "fort"
    elif (( rms >= seuilMoyen  )); then echo "moyen"
    elif (( rms >= seuilFaible )); then echo "faible"
    else                                echo "silence"
    fi
}

# =============================================================================
# GESTION ALERTE (holdTime)
# Vérifie si le niveau est maintenu depuis holdTime ms avant d'envoyer (alerte).
# $1 = nouveau niveau calculé
# =============================================================================

check_alerte() {
    local nouveau_niveau="$1"
    local now
    now=$(now_ms)

    if [ "$nouveau_niveau" != "$niveau_actuel" ]; then
        # Début d'un nouveau niveau → démarrage chrono holdTime
        niveau_actuel="$nouveau_niveau"
        timestamp_seuil=$now
        return
    fi

    # Niveau stable : vérifier si holdTime écoulé
    local duree=$(( now - timestamp_seuil ))
    if (( duree >= holdTime )) && [ "$niveau_actuel" != "$niveau_precedent" ]; then
        # Seuil maintenu suffisamment longtemps → envoi (alerte)
        send_vpiv "\$E:MicroSE:(alerte):*:${niveau_actuel}#"
        log_mic "(alerte) → ${niveau_actuel} (maintenu ${duree}ms)"
        niveau_precedent="$niveau_actuel"
    fi
}

# =============================================================================
# LANCEMENT BALAYAGE SONORE (non bloquant)
# Vérifie qu'aucun balayage n'est déjà en cours avant de lancer.
# =============================================================================

lancer_balayage() {
    # Vérifier si balayage déjà en cours
    if (( PID_BALAYAGE > 0 )) && kill -0 "$PID_BALAYAGE" 2>/dev/null; then
        log_mic "Balayage déjà en cours (PID=$PID_BALAYAGE), ignoré."
        return
    fi

    local script_balayage="$SCRIPT_DIR/rz_balayage_sonore.sh"
    if [ ! -f "$script_balayage" ]; then
        log_mic "ERREUR : rz_balayage_sonore.sh introuvable."
        send_vpiv "\$I:COM:error:SE:\"MicroSE : rz_balayage_sonore.sh introuvable\"#"
        return
    fi

    log_mic "Lancement balayage sonore..."
    bash "$script_balayage" \
         "$pasBalayage" "$timeoutOrientation" "$winSize" \
         "$FIFO_OUT" "$LOG_FILE" &
    PID_BALAYAGE=$!
    log_mic "Balayage lancé (PID=$PID_BALAYAGE)"
}

# =============================================================================
# UN CYCLE COMPLET DE MESURE
# Appelé à chaque itération de la boucle principale.
# =============================================================================

run_cycle() {
    # 1. Lecture RMS brut
    local rms_brut
    rms_brut=$(read_rms)

    # 2. Lissage fenêtre glissante
    local rms_lisse
    rms_lisse=$(update_fenetre "$rms_brut")

    # 3. Aiguillage selon mode
    case "$modeMicro" in

        # -------------------------------------------------------------------
        # MODE NORMAL : envoi dataContRMS à chaque mesure (sans filtre delta)
        # -------------------------------------------------------------------
        "normal")
            send_vpiv "\$F:MicroSE:dataContRMS:*:${rms_lisse}#"
            ;;

        # -------------------------------------------------------------------
        # MODE INTENSITE : filtre delta + alerte holdTime
        # -------------------------------------------------------------------
        "intensite")
            # Envoi dataContRMS uniquement si delta > seuilDelta
            local delta=$(( rms_lisse - derniere_rms_envoyee ))
            (( delta < 0 )) && delta=$(( -delta ))   # Valeur absolue

            if (( delta > seuilDelta )); then
                send_vpiv "\$F:MicroSE:dataContRMS:*:${rms_lisse}#"
                derniere_rms_envoyee=$rms_lisse
            fi

            # Vérification seuils + holdTime
            local niveau
            niveau=$(get_niveau "$rms_lisse")
            check_alerte "$niveau"
            ;;

        # -------------------------------------------------------------------
        # MODE ORIENTATION : balayage uniquement (pas de flux RMS)
        # -------------------------------------------------------------------
        "orientation")
            lancer_balayage
            # Pause pendant la durée estimée du balayage pour ne pas relancer
            sleep "$timeoutOrientation"
            ;;

        # -------------------------------------------------------------------
        # MODE SURVEILLANCE : intensite + balayage auto si RMS > seuilMoyen
        # -------------------------------------------------------------------
        "surveillance")
            # Filtre delta
            local delta=$(( rms_lisse - derniere_rms_envoyee ))
            (( delta < 0 )) && delta=$(( -delta ))

            if (( delta > seuilDelta )); then
                send_vpiv "\$F:MicroSE:dataContRMS:*:${rms_lisse}#"
                derniere_rms_envoyee=$rms_lisse
            fi

            # Alertes
            local niveau
            niveau=$(get_niveau "$rms_lisse")
            check_alerte "$niveau"

            # Balayage automatique si niveau >= moyen
            if [[ "$niveau" == "moyen" || "$niveau" == "fort" ]]; then
                lancer_balayage
            fi
            ;;

        # -------------------------------------------------------------------
        # MODE OFF : ne rien faire (géré dans la boucle principale)
        # -------------------------------------------------------------------
        "off")
            ;;
    esac
}

# =============================================================================
# MAIN — Boucle principale
# =============================================================================

main() {
    log_mic "=========================================="
    log_mic "Démarrage rz_microSe_manager.sh  v2.0"
    log_mic "=========================================="

    # Vérifications dépendances
    for tool in jq ffmpeg bc; do
        if ! command -v "$tool" &>/dev/null; then
            log_mic "ERREUR CRITIQUE : $tool non installé. Arrêt."
            exit 1
        fi
    done
    if ! command -v termux-microphone-record &>/dev/null; then
        log_mic "ERREUR CRITIQUE : termux-microphone-record indisponible (pkg install termux-api)"
        send_vpiv "\$I:COM:error:SE:\"MicroSE : termux-microphone-record indisponible\"#"
        exit 1
    fi

    if [ ! -p "$FIFO_OUT" ]; then
        log_mic "WARN : FIFO $FIFO_OUT absente. Lancer fifo_manager.sh create."
    fi

    # Chargement config
    load_config

    # Boucle principale
    while true; do

        # Rechargement dynamique du mode (SP peut changer modeMicro en cours)
        if [ -f "$MIC_CONFIG" ]; then
            local new_mode
            new_mode=$(jq -r '.mic.modeMicro // "off"' "$MIC_CONFIG")
            if [ "$new_mode" != "$modeMicro" ]; then
                log_mic "Changement de mode : $modeMicro → $new_mode"
                modeMicro="$new_mode"
                # Réinitialisation état interne sur changement de mode
                fenetre_rms=()
                derniere_rms_envoyee=0
                niveau_actuel="silence"
                niveau_precedent="silence"
                timestamp_seuil=0
                # ACK nouveau mode
                send_vpiv "\$I:MicroSE:modeMicro:*:${modeMicro}#"
            fi
        fi

        if [ "$modeMicro" == "off" ]; then
            sleep 1
            continue
        fi

        # Cycle de mesure
        run_cycle

        # Pause inter-cycles (freqCont ms → secondes)
        local sleep_s
        sleep_s=$(echo "scale=3; $freqCont / 1000" | bc)
        sleep "$sleep_s"
    done
}

# Nettoyage sur sortie
trap 'rm -f "$LOCK_FILE" "$WAV_TEMP"; log_mic "Arrêt du manager."' EXIT

main