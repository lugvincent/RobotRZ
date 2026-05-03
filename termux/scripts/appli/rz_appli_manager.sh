#!/bin/bash
# =============================================================================
# SCRIPT: rz_state-manager.sh
# CHEMIN: ~/robotRZ-smartSE/termux/scripts/core/rz_state-manager.sh
# DESCRIPTION:
#   - Gestion centralisée des états du système
#   - Intercepte les commandes de pilotage (MQTT).
#   - Calcule le contexte STT (Commande / Conversation / Mixte).
#   - Maintient la cohérence de global.json.
#   - Assure la passerelle vers les FIFO locaux.
#
# DEPENDANCES:
#   - mosquitto_pub, mosquitto_sub (pour MQTT)
#   - jq (pour JSON)
#   - keys.json (credentials MQTT)
#   - typeReseau (global.json doit contenir un champ typeReseau pour différencier les réseaux)
#
# AUTEUR: Vincent Philippe
# VERSION: 1.4  (ajout credentials MQTT depuis keys.json et de typeReseau)
# DATE: 2026-04-27
# =============================================================================

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
GLOBAL_CONFIG="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/state_manager.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"

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
                [[ "$PROP" == "modeRZ" || "$PROP" == "typePtge" || "$PROP" == "typeReseau" ]] \
                    && handle_state_transition "$PROP" "$VALUE"
                ;;
            "Mtr")
                if [[ "$PROP" == "speed" ]]; then
                    jq --arg v "$VALUE" '.Robot.speed=($v|tonumber)' "$GLOBAL_CONFIG" \
                        > "${GLOBAL_CONFIG}.tmp" \
                    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                    calculate_stt_context
                fi
                ;;
            "Gyro"|"MG")
                echo "$msg" > "$FIFO_IN"
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
        echo '{"CfgS":{"modeRZ":1,"typePtge":0},"Robot":{"speed":0},"STT":{"context":"Inactif"},"Sys":{}}' \
            > "$GLOBAL_CONFIG"
    fi

    log "En attente de commandes sur SE/commande..."
    mosquitto_sub -h "$MQTT_HOST" -p 1883 \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "SE/commande" | while read -r msg; do
        process_mqtt_command "$msg"
    done
}
main