#!/bin/bash
# =============================================================================
# SCRIPT  : rz_vpiv_parser.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/core/rz_vpiv_parser.sh
#
# OBJECTIF
# --------
# Daemon central d'aiguillage des trames VPIV reçues via MQTT (topic SE/commande).
# Décode chaque trame, gère les urgences (Point C), les retours vocaux,
# puis dispatche vers le bon FIFO ou script selon la CAT et le MODULE.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Écoute continue sur MQTT (SE/commande).
# 2. Validation format VPIV : $CAT:MODULE:PROP:INST:VALUE#
# 3. POINT C — urgence : si CAT=E + MODULE=Urg + PROP=etat + VALUE=danger
#              → lancement immédiat de safety_stop.sh
# 4. Retour vocal (si typePtge != 0) :
#    - COM:error → TTS "Erreur : ..."
#    - COM:info  → TTS "..."
#    - Urg:etat  → TTS selon code urgence
# 5. Aiguillage par CAT :
#    - CAT=A (Applications)  → fifo_appli_in   (rz_appli_manager.sh)
#    - CAT=V/F (Consignes)   → selon MODULE :
#        CamSE               → rz_camera_manager.sh
#        Appli/Zoom/TTS      → fifo_tasker_in
#        CfgS                → fifo_termux_in (state-manager)
#        autres              → fifo_termux_in
#    - CAT=A (Voice intent)  → fifo_tasker_in si MODULE=Voice
#    - CAT=I/E               → fifo_termux_in
# 6. ACK VPIV (COM:info) envoyé sur SE/statut pour chaque trame traitée.
#
# TABLE A — VPIV TRAITÉS (CAT entrants)
# --------------------------------------
#   CAT=V/F : Consignes SP→SE  (moteurs, caméra, config, ...)
#   CAT=A   : Applications SP→SE  (Appli.Baby, Appli.zoom, ExprTasker, ...)
#   CAT=E   : Événements SP→SE  (urgences, états)
#   CAT=I   : Informations SP→SE  (retours, ACK)
#
# INTERACTIONS
# ------------
#   Écoute  : MQTT topic SE/commande  (mosquitto_sub)
#   Publie  : MQTT topic SE/statut    (ACK, erreurs)
#   Écrit   : fifo/fifo_termux_in     (state-manager, managers capteurs)
#   Écrit   : fifo/fifo_tasker_in     (Tasker bridge)
#   Écrit   : fifo/fifo_appli_in      (rz_appli_manager.sh — CAT=A)
#   Écrit   : fifo/fifo_voice_in    (rz_voice_manager.sh — TTS, vol, output)
#   Lit     : config/global.json      (typePtge, pour retour vocal)
#   Appelle : scripts/core/safety_stop.sh  (Point C urgence)
#   Appelle : scripts/sensors/cam/rz_camera_manager.sh
#
# PRÉCAUTIONS
# -----------
# - fifo_appli_in doit exister (créé par fifo_manager.sh create).
# - Les trames CAT=A Appli sont transmises telles quelles à fifo_appli_in,
#   sans modification ni validation du contenu (délégué à rz_appli_manager.sh).
# - En cas de FIFO plein ou bloqué : l'écriture est non-bloquante (timeout 1s).
# - La boucle MQTT se relance automatiquement après chaque déconnexion (sleep 0.5s).
#
# DÉPENDANCES
# -----------
#   mosquitto_sub, mosquitto_pub, jq, termux-tts-speak
#   safety_stop.sh, rz_camera_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (ajout routage CAT=A → fifo_appli_in)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
fi

export CONFIG_DIR="$BASE_DIR/config"
FIFO_DIR="$BASE_DIR/fifo"
LOG_DIR="$BASE_DIR/logs"

FIFO_TASKER="$FIFO_DIR/fifo_tasker_in"
FIFO_TERMUX="$FIFO_DIR/fifo_termux_in"
FIFO_APPLI="$FIFO_DIR/fifo_appli_in"      # ← Ajout : CAT=A Applications
FIFO_VOICE="$FIFO_DIR/fifo_voice_in"   # module Voice (TTS, vol, output)
FIFO_RETURN="$FIFO_DIR/fifo_return"
LOG_FILE="$LOG_DIR/vpiv_parser.log"

MQTT_BROKER="robotz-vincent.duckdns.org"
MQTT_TOPIC="SE/commande"

# Lecture credentials MQTT depuis keys.json
KEYS_FILE="$BASE_DIR/config/keys.json"
MQTT_USER=$(jq -r '.MQTT_USER // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_PASS=$(jq -r '.MQTT_PASS // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST=$(jq -r '.MQTT_HOST // empty' "$KEYS_FILE" 2>/dev/null)
# Fallback si keys.json absent
MQTT_BROKER="${MQTT_HOST:-robotz-vincent.duckdns.org}"

# =============================================================================
# UTILITAIRES
# =============================================================================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [VPIV] $1" >> "$LOG_FILE"
    echo "$1"
}

# Écriture non-bloquante dans un FIFO (timeout 1s, évite blocage si lecteur absent)
write_fifo() {
    local fifo="$1"
    local trame="$2"
    if [ ! -p "$fifo" ]; then
        log "WARN : FIFO absent ($fifo) — trame perdue : $trame"
        return 1
    fi
    # Écriture avec timeout 1s pour ne pas bloquer le parser
    echo "$trame" > "$fifo" &
    local pid_w=$!
    ( sleep 1 && kill "$pid_w" 2>/dev/null ) &
    wait "$pid_w" 2>/dev/null
    return $?
}

# =============================================================================
# INITIALISATION DES FIFOS
# =============================================================================

init_fifo() {
    mkdir -p "$FIFO_DIR"
    [ -p "$FIFO_TASKER"  ] || mkfifo "$FIFO_TASKER"
    [ -p "$FIFO_TERMUX"  ] || mkfifo "$FIFO_TERMUX"
    [ -p "$FIFO_APPLI"   ] || mkfifo "$FIFO_APPLI"
    [ -p "$FIFO_VOICE"  ] || mkfifo "$FIFO_VOICE"
    [ -p "$FIFO_RETURN"  ] || mkfifo "$FIFO_RETURN"
    log "FIFOs initialisés"
}

# =============================================================================
# VALIDATION FORMAT VPIV
# Format : $CAT:MODULE:PROP:INST:VALUE#
# CAT    : un caractère (V F A I E)
# =============================================================================

validate_vpiv() {
    [[ "$1" =~ ^\$(.):([^:]+):([^:]+):([^:]+):(.*)#$ ]]
}

# =============================================================================
# PROCESS_COMMAND — Traitement d'une trame VPIV validée
# Les groupes BASH_REMATCH sont remplis par validate_vpiv() en amont.
# =============================================================================

process_command() {
    local msg="$1"
    local CAT="${BASH_REMATCH[1]}"
    local MODULE="${BASH_REMATCH[2]}"
    local PROP="${BASH_REMATCH[3]}"
    local INST="${BASH_REMATCH[4]}"
    local VAL="${BASH_REMATCH[5]}"

    log "Trame : CAT=$CAT MODULE=$MODULE PROP=$PROP INST=$INST VAL=$VAL"

    # =========================================================================
    # POINT C — URGENCE DANGER : réaction immédiate avant tout autre traitement
    # Déclenchement si SP confirme un état de danger (CAT=E, Urg.etat=danger)
    # =========================================================================
    if [[ "$CAT" == "E" && "$MODULE" == "Urg" && "$PROP" == "etat" && "$VAL" == "danger" ]]; then
        log "!!! ALERTE DANGER — Lancement safety_stop.sh !!!"
        bash "$BASE_DIR/scripts/core/safety_stop.sh"
        # On continue malgré tout pour envoyer l'ACK
    fi

    # =========================================================================
    # RETOUR VOCAL (si typePtge != 0 — pilotage vocal actif)
    # =========================================================================
    local ptge_mode
    ptge_mode=$(jq -r '.CfgS.typePtge // 0' "$CONFIG_DIR/global.json" 2>/dev/null)

    if [ "$ptge_mode" -ne 0 ]; then

        # --- Feedback COM vocal ---
        if [[ "$MODULE" == "COM" ]]; then
            local clean_msg
            clean_msg=$(echo "$VAL" | tr -d '"')
            [[ "$PROP" == "error" ]] && termux-tts-speak "Erreur : $clean_msg"
            [[ "$PROP" == "info"  ]] && termux-tts-speak "$clean_msg"
        fi

        # --- Feedback Urg vocal ---
        if [[ "$CAT" == "E" && "$MODULE" == "Urg" ]]; then
            case "$VAL" in
                "URG_LOW_BAT")     termux-tts-speak "Batterie critique." ;;
                "URG_CPU_CHARGE")  termux-tts-speak "Surcharge processeur." ;;
                "URG_OVERHEAT")    termux-tts-speak "Surchauffe détectée." ;;
                "danger")          termux-tts-speak "Alerte danger. Sécurité activée." ;;
                *)                 termux-tts-speak "Alerte sécurité." ;;
            esac
        fi
    fi

    # =========================================================================
    # AIGUILLAGE PAR CAT
    # =========================================================================

    case "$CAT" in

        # ---------------------------------------------------------------------
        # CAT = A — Applications Android (Appli.*, ExprTasker, Voice)
        # Deux cas :
        #   1. MODULE=Appli → rz_appli_manager.sh via fifo_appli_in
        #   2. MODULE=Voice ou autre CAT=A → fifo_tasker_in (Tasker)
        # ---------------------------------------------------------------------
        "A")
            if [[ "$MODULE" == "Appli" ]]; then
                # Transmission directe à rz_appli_manager.sh (délégation complète)
                log "CAT=A Appli → fifo_appli_in : $msg"
                write_fifo "$FIFO_APPLI" "$msg"
            elif [[ "$MODULE" == "Voice" ]]; then
                # Route toutes les trames Voice vers rz_voice_manager.sh
                # (TTS, vol, output) — supprime le conflit fifo_termux_in/stt_manager
                log "CAT=A Voice → fifo_voice_in : $msg"
                write_fifo "$FIFO_VOICE" "$msg"
            else
                # Autres CAT=A non-Appli → Tasker
                write_fifo "$FIFO_TASKER" "$msg"
            fi
            ;;

        # ---------------------------------------------------------------------
        # CAT = V (Consigne) ou F (Flux demandé)
        # Aiguillage par MODULE
        # ---------------------------------------------------------------------
        "V"|"F")
            case "$MODULE" in

                # Caméra → gestionnaire dédié
                "CamSE")
                    bash "$BASE_DIR/scripts/sensors/cam/rz_camera_manager.sh" \
                         "$PROP" "$VAL"
                    ;;

                # Apps Android UI (Zoom dans mode non-Appli, TTS Tasker)
                "Zoom"|"TTS")
                    write_fifo "$FIFO_TASKER" "$msg"
                    ;;

                # Configuration système → state-manager via fifo_termux_in
                "CfgS")
                    write_fifo "$FIFO_TERMUX" "$msg"
                    ;;

                # Torche → gestionnaire dédié
                "TorchSE")
                    bash "$BASE_DIR/scripts/sensors/torch/rz_torch_manager.sh" \
                         "$PROP" "$VAL"
                    ;;

                # Voice (vol, output) → manager Voice dédié
                "Voice")
                    write_fifo "$FIFO_VOICE" "$msg"
                    ;;

                
                # Tous les autres modules → fifo_termux_in (managers capteurs)
                *)
                    write_fifo "$FIFO_TERMUX" "$msg"
                    ;;
            esac
            ;;

        # ---------------------------------------------------------------------
        # CAT = I (Information) ou E (Événement)
        # → fifo_termux_in pour traitement state-manager
        # Exception : Urg → ACK systématique en plus
        # ---------------------------------------------------------------------
        "I"|"E")
            # ACK urgence systématique
            if [[ "$MODULE" == "Urg" ]]; then
                write_fifo "$FIFO_TERMUX" "\$I:Urg:${PROP}:SE:${VAL}#"
            else
                write_fifo "$FIFO_TERMUX" "$msg"
            fi
            # Propagation urgences vers Voice (TTS urgence interruptif)
            if [[ "$CAT" == "E" && "$MODULE" == "Urg" ]]; then
                write_fifo "$FIFO_VOICE" "$msg"
            fi
            ;;

        # ---------------------------------------------------------------------
        # CAT inconnue
        # ---------------------------------------------------------------------
        *)
            log "WARN : CAT inconnue '$CAT' — trame ignorée : $msg"
            mosquitto_pub -h "$MQTT_BROKER" -p 1883 \
                -u "$MQTT_USER" -P "$MQTT_PASS" \
                -t "SE/statut" \
                -m "\$I:COM:warn:SE:\"VPIV: CAT inconnue '${CAT}'\"#" \
                -q 0 >/dev/null 2>&1
            return
            ;;
    esac

    # =========================================================================
    # ACK MQTT — Confirmation de réception (optionnel, utile pour debug SP)
    # Format allégé : 4 premiers champs de la trame
    # =========================================================================
    local short_msg
    short_msg=$(echo "$msg" | cut -d':' -f1-4)
    mosquitto_pub -h "$MQTT_BROKER" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/statut" \
        -m "\$I:COM:info:SE:ACK_${short_msg}#" \
        -q 0 >/dev/null 2>&1
}

# =============================================================================
# BOUCLE PRINCIPALE — Daemon MQTT
# Reconnexion automatique après chaque interruption mosquitto_sub.
# =============================================================================

main() {
    log "=========================================="
    log "Démarrage rz_vpiv_parser.sh  v2.0"
    log "=========================================="

    # Vérification dépendances
    if ! command -v jq &>/dev/null; then
        log "ERREUR CRITIQUE : jq non installé. Arrêt."
        exit 1
    fi
    if ! command -v mosquitto_sub &>/dev/null; then
        log "ERREUR CRITIQUE : mosquitto_sub non installé. Arrêt."
        exit 1
    fi

    init_fifo

    log "Écoute MQTT sur $MQTT_BROKER/$MQTT_TOPIC ..."

    # Boucle de reconnexion : mosquitto_sub -C 1 lit un message puis sort,
    # ce qui permet de relancer proprement en cas de coupure réseau.
    while true; do
        mosquitto_sub -h "$MQTT_BROKER" -p 1883 \
            -u "$MQTT_USER" -P "$MQTT_PASS" \
            -t "$MQTT_TOPIC" -C 1 \
        | while read -r msg; do
            if validate_vpiv "$msg"; then
                process_command "$msg"
            else
                log "Trame invalide : '$msg'"
                mosquitto_pub -h "$MQTT_BROKER" -p 1883 \
                    -u "$MQTT_USER" -P "$MQTT_PASS" \
                    -t "SE/statut" \
                    -m "\$E:COM:error:SE:Trame_Invalide#" \
                    -q 0 >/dev/null 2>&1
            fi
        done
        # Pause courte avant reconnexion (évite boucle CPU si réseau absent)
        sleep 0.5
    done
}

# Nettoyage propre sur Ctrl+C ou kill
trap 'log "Arrêt du parser VPIV."' EXIT

main