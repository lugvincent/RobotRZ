#!/bin/bash
# =============================================================================
# SCRIPT: rz_state-manager.sh
# CHEMIN: ~/robotRZ-smartSE/termux/scripts/core/rz_state-manager.sh
#
# OBJECTIF
# --------
# Gestionnaire centralisé des états SE du robot RobotRZ.
# Lit les trames VPIV depuis MQTT (SE/commande), met à jour global.json,
# recalcule le contexte STT et synchronise l'état vers Tasker.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
#   - Reçoit les trames VPIV via mosquitto_sub (topic SE/commande)
#   - Traite les modules : CfgS, Mtr, Gyro, MG, STT
#   - Met à jour global.json après chaque changement d'état (écriture atomique)
#   - Calcule STT.context selon modeRZ / typePtge / Robot.speed
#   - Copie global.json vers /sdcard/Tasker/RobotRZ/ après chaque écriture
#     (requis par la tâche Tasker RZ_OuvreEtat pour lire l'état SE)
#   - Envoie les ACK VPIV $I:*# sur MQTT SE/statut
#
# INTERACTIONS
# ------------
#   Lit    : config/global.json                    (état courant)
#   Écrit  : config/global.json                    (mises à jour état)
#   Copie  : /sdcard/Tasker/RobotRZ/global.json    (sync Tasker)
#   Publie : MQTT SE/statut                        (ACK transitions)
#   Lit    : config/keys.json                      (credentials MQTT)
#
# PRÉCAUTIONS
# -----------
# - Écriture atomique via fichier .tmp + mv (évite corruption JSON)
# - sync_to_tasker() silencieux si /sdcard/Tasker/RobotRZ/ absent
# - STT.modeSTT=OFF → STT.context forcé à Inactif (Python ignorera à la prochaine détection)
# - Jamais de pattern A && B || C (SC2015) — utiliser if/then/else
#
# DÉPENDANCES
# -----------
#   jq, mosquitto_sub, mosquitto_pub
#   config/keys.json, config/global.json
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.5  (ajout cas STT, sync Tasker après chaque write global.json)
# DATE    : 2026-05-20
# =============================================================================

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
GLOBAL_CONFIG="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/state_manager.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"

# Chemin Tasker — requis par RZ_OuvreEtat pour lire l'état SE
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
# Copie global.json vers /sdcard/Tasker/RobotRZ/ après chaque écriture.
# Requis par la tâche Tasker RZ_OuvreEtat pour lire l'état SE.
# Silencieux si le dossier Tasker est absent (Tasker non installé / SD absent).
# ⚠ Utilise if/then/else — jamais de pattern A && B || C (SC2015).
# =============================================================================
sync_to_tasker() {
    local tasker_dir
    tasker_dir="$(dirname "$TASKER_GLOBAL")"
    if [ -d "$tasker_dir" ]; then
        if cp "$GLOBAL_CONFIG" "$TASKER_GLOBAL" 2>/dev/null; then
            log "Sync Tasker ✓" "DEBUG"
        else
            log "Sync Tasker échouée" "WARN"
        fi
    fi
}

# =============================================================================
# FONCTION : calculate_stt_context
# =============================================================================
calculate_stt_context() {
    local modeRZ
    modeRZ=$(jq -r '.CfgS.modeRZ // 0' "$GLOBAL_CONFIG")
    local modePtge
    modePtge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_CONFIG")
    local speed
    speed=$(jq -r '.Robot.speed // 0' "$GLOBAL_CONFIG")
    local context="Inactif"

    log "Calcul contexte STT (RZ:$modeRZ, Ptge:$modePtge, Spd:$speed)"

    if [[ "$modeRZ" == "0" || "$modeRZ" == "5" ]]; then
        context="Inactif"
    elif [[ "$modePtge" != "1" && "$modePtge" != "4" ]]; then
        context="Inactif"
    elif [[ "$speed" != "0" ]]; then
        context="Cmde"
    else
        case "$modeRZ" in
            1)   context="Conv"   ;;
            4)   context="Mixte"  ;;
            2|3) context="Mixte"  ;;
            *)   context="Inactif" ;;
        esac
    fi

    jq --arg ctx "$context" '.STT.context=$ctx' "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"

    log "STT.context défini sur : $context"
    sync_to_tasker
    # Notifier Tasker du changement de contexte STT
    # → met à jour %RZsttContext via profil Fichier_ModifieParSE → RZ_Dispatch → RZ_STT_Context
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

    log "Transition demandée : $key -> $value" "UPDATE"

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
                if [[ "$PROP" == "speed" ]]; then
                    jq --arg v "$VALUE" '.Robot.speed=($v|tonumber)' "$GLOBAL_CONFIG" \
                        > "${GLOBAL_CONFIG}.tmp" \
                    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                    calculate_stt_context   # sync_to_tasker appelé en fin de calculate
                fi
                ;;
            "STT")
                # ----------------------------------------------------------------
                # Commandes SP → STT
                # modeSTT : KWS = écoute active | OFF = inactif
                #   OFF → STT.context forcé Inactif (Python ignorera à la prochaine détection)
                #   KWS → recalcul contexte selon modeRZ / typePtge
                # paraSTT : mise à jour paramètres PocketSphinx (objet JSON)
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
                            log "STT.context forcé Inactif (mode OFF)"
                            sync_to_tasker
                        else
                            calculate_stt_context   # sync_to_tasker appelé en fin
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
                        log "STT.paraSTT mis à jour" "UPDATE"
                        sync_to_tasker
                        ;;
                    *)
                        log "STT.$PROP : propriété inconnue — ignorée" "DEBUG"
                        ;;
                esac
                ;;
            "Gyro"|"MG")
                printf '%s\n' "$msg" > "$FIFO_IN"
                ;;
            *)
                log "Module inconnu ou ignoré : $MODULE" "DEBUG"
                ;;
        esac
    else
        log "Trame invalide reçue : $msg" "ERROR"
    fi
}

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================
main() {
    log "=== Démarrage du Gestionnaire d'État (SE) ==="

    if [[ ! -f "$GLOBAL_CONFIG" ]]; then
        log "Global config absente. Création d'une structure de base." "WARN"
        echo '{"CfgS":{"modeRZ":1,"typePtge":0},"Robot":{"speed":0},"STT":{"modeSTT":"OFF","context":"Inactif"},"Sys":{}}' \
            > "$GLOBAL_CONFIG"
        sync_to_tasker
    fi

    log "En attente de commandes sur SE/commande..."
    mosquitto_sub -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/commande" | while read -r msg; do
        process_mqtt_command "$msg"
    done
}
main
