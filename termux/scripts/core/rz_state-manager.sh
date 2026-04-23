#!/bin/bash
# =============================================================================
# SCRIPT: rz_state-manager.sh
# CHEMIN: PROJETRZ/termux/scripts/core/rz_state-manager.sh
# DESCRIPTION:
#   - Gestion centralisée des états du système
#   - Intercepte les commandes de pilotage (MQTT).
#   - Calcule le contexte STT (Commande / Conversation / Mixte).
#   - Maintient la cohérence de global.json.
#   - Assure la passerelle vers les FIFO locaux.
#
# DEPENDANCES:
#   - mosquitto_pub (pour MQTT)
#   - jq (pour JSON)
#
# AUTEUR: Vincent Philippe
# VERSION: 1.3  (correction chemin FIFO_IN : fifo_termux_in → fifo/fifo_termux_in)
# DATE: 2026-03-12
# =============================================================================
# --- CONFIGURATION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
GLOBAL_CONFIG="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/state_manager.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"
MQTT_HOST="robotz-vincent.duckdns.org"

# --- INITIALISATION ---
mkdir -p "$(dirname "$LOG_FILE")"

log() {
    local level="INFO"
    [[ -n "$2" ]] && level="$2"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [$level] $1" >> "$LOG_FILE"
    # Optionnel : echo "[STATE] $1" (pour debug console)
}

# =============================================================================
# FONCTION : calculate_stt_context : appelée à chaque changement de modeRZ, typePtge ou speed
# LOGIQUE  : Applique la matrice de décision basée sur modeRZ, modePtge et Speed
# =============================================================================
calculate_stt_context() {
    # Extraction des valeurs actuelles
    local modeRZ
    modeRZ=$(jq -r '.CfgS.modeRZ // 0' "$GLOBAL_CONFIG")
    local modePtge
    modePtge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_CONFIG")
    local speed
    speed=$(jq -r '.Robot.speed // 0' "$GLOBAL_CONFIG")
    local context="Inactif"

    log "Calcul contexte STT (RZ:$modeRZ, Ptge:$modePtge, Spd:$speed)"

    # 1. Filtre de sécurité : Si mode ARRET (0), ERREUR (5) ou pilotage non-vocal
    if [[ "$modeRZ" == "0" || "$modeRZ" == "5" ]]; then
        context="Inactif"
    elif [[ "$modePtge" != "1" && "$modePtge" != "4" ]]; then
        context="Inactif"
    
    # 2. Sécurité Mouvement : Si le robot bouge, on ne discute pas (Sphinx uniquement)
    elif [[ "$speed" != "0" ]]; then
        context="Cmde"
    
    # 3. Arbitrage à l'arrêt
    else
        case "$modeRZ" in
            1) context="Conv"   ;; # VEILLE : Conversation IA prioritaire
            4) context="Mixte"  ;; # AUTONOME : Sphinx puis IA si échec
            2|3) context="Mixte" ;; # FIXE / DEPLACEMENT (stoppé) : Mixte
            *) context="Inactif" ;;
        esac
    fi

    # Mise à jour atomique du fichier global
    jq --arg ctx "$context" '.STT.context=$ctx' "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
    
    log "STT.context défini sur : $context"
}

# =============================================================================
# FONCTION : handle_state_transition (gestion des transitions d'état)
# ROLE     : Met à jour les modes et notifie le reste du système (MQTT)
# =============================================================================
handle_state_transition() {
    local key="$1"   # Ex: modeRZ ou typePtge
    local value="$2"

    log "Transition demandée : $key -> $value" "UPDATE"

    # Mise à jour JSON
    jq --arg val "$value" ".CfgS.$key=(\$val|tonumber)" "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"

    # Notification MQTT pour synchronisation SP (Feedback)
    mosquitto_pub -h "$MQTT_HOST" -t "SE/statut" -m "\$I:CfgS:$key:*:$value#"
    
    # Recalculer l'impact sur le STT
    calculate_stt_context
}

# =============================================================================
# FONCTION : process_mqtt_command (Traitement des commandes MQTT)
# ROLE     : Analyse la trame VPIV entrante
# =============================================================================
process_mqtt_command() {
    local msg="$1"

    # Regex pour parser $TYPE:MODULE:PROP:INST:VALUE#
    if [[ $msg =~ ^\$(.):([^:]+):([^:]+):([^:]+):(.*)#$ ]]; then
        local TYPE="${BASH_REMATCH[1]}"
        local MODULE="${BASH_REMATCH[2]}"
        local PROP="${BASH_REMATCH[3]}"
        local VALUE="${BASH_REMATCH[5]}"

        log "Parsing MQTT: [$TYPE] $MODULE.$PROP = $VALUE"

        case "$MODULE" in
            "CfgS")
                # Changement de configuration système
                [[ "$PROP" == "modeRZ" || "$PROP" == "typePtge" ]] && handle_state_transition "$PROP" "$VALUE"
                ;;
            
            "Mtr")
                # Mise à jour de la vitesse (crucial pour le contexte STT)
                if [[ "$PROP" == "speed" ]]; then
                    jq --arg v "$VALUE" '.Robot.speed=($v|tonumber)' "$GLOBAL_CONFIG" > "${GLOBAL_CONFIG}.tmp" \
                    && mv "${GLOBAL_CONFIG}.tmp" "$GLOBAL_CONFIG"
                    calculate_stt_context
                fi
                ;;

            "Gyro"|"MG")
                # Redirection vers le FIFO pour traitement par les scripts sensors
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
# BOUCLE PRINCIPALE (MAIN)
# =============================================================================
main() {
    log "=== Démarrage du Gestionnaire d'État (SE) ==="
    
    # Vérification présence fichier global
    if [[ ! -f "$GLOBAL_CONFIG" ]]; then
        log "Global config absente. Création d'une structure de base." "WARN"
        echo '{"CfgS":{"modeRZ":1,"typePtge":0},"Robot":{"speed":0},"STT":{"context":"Inactif"},"Sys":{}}' > "$GLOBAL_CONFIG"
    fi

    # Écoute continue sans reconnexion (plus réactif)
    log "En attente de commandes sur SE/commande..."
    mosquitto_sub -h "$MQTT_HOST" -t "SE/commande" | while read -r msg; do
        process_mqtt_command "$msg"
    done
}

# Lancement
main