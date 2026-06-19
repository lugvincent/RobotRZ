#!/bin/bash
# =============================================================================
# SCRIPT: rz_state-manager.sh
# CHEMIN: ~/robotRZ-smartSE/termux/scripts/core/rz_state-manager.sh
#
# OBJECTIF
# --------
# Gestionnaire centralise des etats SE du robot RobotRZ.
# Lit les trames VPIV depuis MQTT (SE/commande), met a jour global.json,
# recalcule le contexte STT et synchronise l'etat vers Tasker.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
#   - Recoit les trames VPIV via mosquitto_sub (topic SE/commande)
#   - Traite les modules : CfgS, Mtr, Gyro, MG, STT
#   - Met a jour global.json apres chaque changement d'etat (ecriture atomique)
#   - Calcule STT.context selon modeRZ / typePtge / mouvement moteur (Mtr.state)
#   - Copie global.json vers /sdcard/Tasker/RobotRZ/ apres chaque ecriture
#     (requis par la tache Tasker RZ_OuvreEtat pour lire l'etat SE)
#   - Envoie les ACK VPIV $I:*# sur MQTT SE/statut
#
# LOGIQUE STT.context (revisee juin 2026)
# -----------------------------------------
# Table modeRZ -> STT.context (base) :
#   0 (ARRET)        -> Inactif
#   1 (VEILLE)        -> Mixte  (permissif par conception ; SEUL risque reel :
#                                 deplacement initie alors que modeRZ reste VEILLE)
#   2 (FIXE)          -> Mixte  (deja securise : moteurs deplacement bloques
#                                 par typePtge/Table A, pas besoin de restreindre STT)
#   3 (DEPLACEMENT)   -> Cmde   (uniquement -- securite + rapidite pendant deplacement)
#   4 (AUTONOME)      -> Mixte
#   5 (ERREUR)        -> Inactif
#
# typePtge doit etre Vocal(1) ou Laisse+Vocal(4), sinon -> Inactif (pas de pilotage
# vocal active du tout).
#
# RAFFINEMENT SECURITE -- detection mouvement via Mtr.state :
#   Mtr.state (Table A, CAT=I, format "L,R,A", direction A->SP) est relaye par SP
#   vers SE sur SE/commande : $I:Mtr:state:*:L,R,A#
#   Si modeRZ=1 (VEILLE) ET mouvement detecte (L!=0 ou R!=0) -> Cmde force
#   (au lieu de Mixte), pour eviter qu'une conversation Gemini ne distraie
#   pendant un deplacement initie en mode VEILLE.
#   ATTENTION : ce cablage SP->SE (relai Mtr.state) n'est peut-etre pas encore
#   actif cote Node-RED. En son absence, L et R restent a leur derniere valeur
#   connue (0 par defaut) -- comportement sans danger, simplement pas de
#   detection mouvement tant que le cablage SP n'est pas fait.
#
# INTERACTIONS
# ------------
#   Lit    : config/global.json                    (etat courant)
#   Ecrit  : config/global.json                    (mises a jour etat)
#   Copie  : /sdcard/Tasker/RobotRZ/global.json    (sync Tasker)
#   Publie : MQTT SE/statut                        (ACK transitions)
#   Lit    : config/keys.json                      (credentials MQTT)
#
# PRECAUTIONS
# -----------
# - Ecriture atomique via fichier .tmp + mv (evite corruption JSON)
# - sync_to_tasker() silencieux si /sdcard/Tasker/RobotRZ/ absent
# - STT.modeSTT=OFF -> STT.context force a Inactif (Python ignorera a la prochaine detection)
# - Jamais de pattern A && B || C (SC2015) -- utiliser if/then/else
# - Robot.speed N'EXISTE PAS dans Table A -- ne jamais l'utiliser (corrige juin 2026,
#   remplace par Mtr.state qui est la variable VPIV reelle et documentee)
#
# DEPENDANCES
# -----------
#   jq, mosquitto_sub, mosquitto_pub
#   config/keys.json, config/global.json
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.6  (revision table modeRZ/STTcontext, detection mouvement via
#                  Mtr.state au lieu de Robot.speed inexistant dans Table A)
# DATE    : 2026-06-19
# =============================================================================

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
GLOBAL_CONFIG="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/state_manager.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"

# Chemin Tasker -- requis par RZ_OuvreEtat pour lire l'etat SE
TASKER_GLOBAL="/sdcard/Tasker/RobotRZ/global.json"

# --- CREDENTIALS MQTT depuis keys.json ---
KEYS_FILE="$BASE_DIR/config/keys.json"
MQTT_USER=$(jq -r '.MQTT_USER // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_PASS=$(jq -r '.MQTT_PASS // empty' "$KEYS_FILE" 2>/dev/null)
MQTT_HOST=$(jq -r '.MQTT_HOST // empty' "$KEYS_FILE" 2>/dev/null)
# Fallback si keys.json absent
MQTT_HOST="${MQTT_HOST:-robotz-vincent.duckdns.org}"

# --- INITIALISATION ---
mkdir -p "$(dirname "$LOG_FILE")"

log() {
    local level="INFO"
    [[ -n "$2" ]] && level="$2"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [$level] $1" >> "$LOG_FILE"
}

# =============================================================================
# FONCTION : sync_to_tasker
# Copie global.json vers /sdcard/Tasker/RobotRZ/ apres chaque ecriture.
# Requis par la tache Tasker RZ_OuvreEtat pour lire l'etat SE.
# Silencieux si le dossier Tasker est absent (Tasker non installe / SD absent).
# Utilise if/then/else -- jamais de pattern A && B || C (SC2015).
# =============================================================================
sync_to_tasker() {
    local tasker_dir
    tasker_dir="$(dirname "$TASKER_GLOBAL")"
    if [ -d "$tasker_dir" ]; then
        if cp "$GLOBAL_CONFIG" "$TASKER_GLOBAL" 2>/dev/null; then
            log "Sync Tasker OK" "DEBUG"
        else
            log "Sync Tasker echouee" "WARN"
        fi
    fi
}

# =============================================================================
# FONCTION : calculate_stt_context
#
# Logique (revisee juin 2026) :
#   1. modeRZ=0|5            -> Inactif
#   2. typePtge != 1 et != 4 -> Inactif (pilotage vocal pas actif)
#   3. modeRZ=1 (VEILLE) ET mouvement detecte (Mtr.state L!=0 ou R!=0)
#                             -> Cmde force (protection deplacement en VEILLE)
#   4. Sinon, table de base :
#        1 (VEILLE)      -> Mixte
#        2 (FIXE)        -> Mixte
#        3 (DEPLACEMENT) -> Cmde
#        4 (AUTONOME)    -> Mixte
# =============================================================================
calculate_stt_context() {
    local modeRZ
    modeRZ=$(jq -r '.CfgS.modeRZ // 0' "$GLOBAL_CONFIG")
    local modePtge
    modePtge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_CONFIG")
    local mtr_l
    mtr_l=$(jq -r '.Mtr.state.L // 0' "$GLOBAL_CONFIG")
    local mtr_r
    mtr_r=$(jq -r '.Mtr.state.R // 0' "$GLOBAL_CONFIG")
    local context="Inactif"
    local en_mouvement=false

    if [[ "$mtr_l" != "0" || "$mtr_r" != "0" ]]; then
        en_mouvement=true
    fi

    log "Calcul contexte STT (RZ:$modeRZ, Ptge:$modePtge, L:$mtr_l, R:$mtr_r, mvt:$en_mouvement)"

    if [[ "$modeRZ" == "0" || "$modeRZ" == "5" ]]; then
        context="Inactif"
    elif [[ "$modePtge" != "1" && "$modePtge" != "4" ]]; then
        context="Inactif"
    elif [[ "$modeRZ" == "1" && "$en_mouvement" == "true" ]]; then
        # VEILLE + mouvement detecte : seul cas a risque -- force Cmde
        context="Cmde"
        log "VEILLE + mouvement detecte -- STT.context force a Cmde (securite)" "WARN"
    else
        case "$modeRZ" in
            1)   context="Mixte"  ;;
            2)   context="Mixte"  ;;
            3)   context="Cmde"   ;;
            4)   context="Mixte"  ;;
            *)   context="Inactif" ;;
        esac
    fi

    jq --arg ctx "$context" '.STT.context=$ctx' "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"

    log "STT.context defini sur : $context"
    sync_to_tasker
    # Notifier Tasker du changement de contexte STT
    # -> met a jour %RZsttContext via profil Fichier_ModifieParSE -> RZ_Dispatch -> RZ_STT_Context
    local trigger_file="/sdcard/Tasker/RobotRZ/trigger.txt"
    if [ -d "$(dirname "$trigger_file")" ]; then
       printf '{"task":"RZ_STT_Context","param":"%s","ts":"%s"}' \
              "$context" "$(date +%s)" \
            > "$trigger_file"
    fi
}

# =============================================================================
# FONCTION : handle_state_transition
# =============================================================================
handle_state_transition() {
    local key="$1"
    local value="$2"

    log "Transition demandee : $key -> $value" "UPDATE"

    jq --arg val "$value" ".CfgS.$key=(\$val|tonumber)" "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"

    mosquitto_pub -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/statut" -m "\$I:CfgS:$key:*:$value#"

    calculate_stt_context
}

# =============================================================================
# FONCTION : process_mqtt_command
# =============================================================================
process_mqtt_command() {
    local msg="$1"

    if [[ $msg =~ ^\$(.):([^:]+):([^:]+):([^:]+):(.*)#$ ]]; then
        local TYPE="${BASH_REMATCH[1]}"
        local MODULE="${BASH_REMATCH[2]}"
        local PROP="${BASH_REMATCH[3]}"
        local VALUE="${BASH_REMATCH[5]}"

        log "Parsing MQTT: [$TYPE] $MODULE.$PROP = $VALUE"

        case "$MODULE" in
            "CfgS")
                [[ "$PROP" == "modeRZ" || "$PROP" == "typePtge" ]] \
                    && handle_state_transition "$PROP" "$VALUE"
                ;;
            "Mtr")
                # ----------------------------------------------------------------
                # Mtr.state : releve par SP depuis Arduino, relaye vers SE.
                # Format VALUE : "L,R,A" (Table A : Mtr.state, CAT=I, A->SP)
                # Utilise par calculate_stt_context pour detecter un mouvement
                # en cours (protection deplacement initie en modeRZ=1 VEILLE).
                # ----------------------------------------------------------------
                if [[ "$PROP" == "state" ]]; then
                    local mtr_l mtr_r mtr_a
                    IFS=',' read -r mtr_l mtr_r mtr_a <<< "$VALUE"
                    jq --arg l "${mtr_l:-0}" --arg r "${mtr_r:-0}" --arg a "${mtr_a:-0}" \
                        '.Mtr.state.L=($l|tonumber) | .Mtr.state.R=($r|tonumber) | .Mtr.state.A=($a|tonumber)' \
                        "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
                    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                    log "Mtr.state mis a jour : L=$mtr_l R=$mtr_r A=$mtr_a"
                    calculate_stt_context   # sync_to_tasker appele en fin de calculate
                fi
                ;;
            "STT")
                # ----------------------------------------------------------------
                # Commandes SP -> STT
                # modeSTT : KWS = ecoute active | OFF = inactif
                #   OFF -> STT.context force Inactif (Python ignorera a la prochaine detection)
                #   KWS -> recalcul contexte selon modeRZ / typePtge / Mtr.state
                # paraSTT : mise a jour parametres PocketSphinx (objet JSON)
                # ----------------------------------------------------------------
                case "$PROP" in
                    "modeSTT")
                        jq --arg v "$VALUE" '.STT.modeSTT=$v' "$GLOBAL_CONFIG" \
                            > "${GLOBAL_CONFIG}.tmp" \
                        && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                        log "STT.modeSTT=$VALUE" "UPDATE"
                        if [[ "$VALUE" == "OFF" ]]; then
                            jq '.STT.context="Inactif"' "$GLOBAL_CONFIG" \
                                > "${GLOBAL_CONFIG}.tmp" \
                            && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                            log "STT.context force Inactif (mode OFF)"
                            sync_to_tasker
                        else
                            calculate_stt_context   # sync_to_tasker appele en fin
                        fi
                        mosquitto_pub -h "$MQTT_HOST" -p 1883 \
                            -u "$MQTT_USER" -P "$MQTT_PASS" \
                            -t "SE/statut" -m "\$I:STT:modeSTT:SE:${VALUE}#"
                        ;;
                    "paraSTT")
                        # VALUE est un objet JSON : {"threshold":"1e-20","keyphrase":"rz",...}
                        jq --argjson v "$VALUE" '.STT.paraSTT=$v' "$GLOBAL_CONFIG" \
                            > "${GLOBAL_CONFIG}.tmp" \
                        && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                        log "STT.paraSTT mis a jour" "UPDATE"
                        sync_to_tasker
                        ;;
                    *)
                        log "STT.$PROP : propriete inconnue -- ignoree" "DEBUG"
                        ;;
                esac
                ;;
            "Gyro"|"MG")
                printf '%s\n' "$msg" > "$FIFO_IN"
                ;;
            *)
                log "Module inconnu ou ignore : $MODULE" "DEBUG"
                ;;
        esac
    else
        log "Trame invalide recue : $msg" "ERROR"
    fi
}

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================
main() {
    log "=== Demarrage du Gestionnaire d'Etat (SE) ==="

    if [[ ! -f "$GLOBAL_CONFIG" ]]; then
        log "Global config absente. Creation d'une structure de base." "WARN"
        echo '{"CfgS":{"modeRZ":1,"typePtge":0},"Mtr":{"state":{"L":0,"R":0,"A":0}},"STT":{"modeSTT":"OFF","context":"Inactif"},"Sys":{}}' \
            > "$GLOBAL_CONFIG"
        sync_to_tasker
    fi

    # Garantit un STT.context coherent des le demarrage, meme si global.json
    # existait deja sans bloc STT complet (ex: genere par original_init.sh)
    log "Calcul initial du contexte STT au demarrage"
    calculate_stt_context

    log "En attente de commandes sur SE/commande..."
    mosquitto_sub -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/commande" | while read -r msg; do
        process_mqtt_command "$msg"
    done
}
main
