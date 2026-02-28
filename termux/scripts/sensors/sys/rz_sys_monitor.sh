#!/bin/bash
# =============================================================================
# SCRIPT: rz_sys_monitor.sh
# CHEMIN: termux/scripts/sensors/sys/rz_sys_monitor.sh
# OBJECTIFS: 
#    - Surveillance matérielle : CPU, Température, Stockage, Uptime.
#    - Mise à jour du "Pulse" (global.json) pour synchronisation avec le Superviseur (SP).
#    - Déclenchement du Réflexe d'Urgence (CAT=E) vers le SP via VPIV.
# INTERACTIONS:
#    - Lit les seuils d'alerte dans config/sensors/sys_config.json.
#    - Écrit les états dynamiques dans config/global.json.
#    - Envoie les trames d'urgence au FIFO_IN ($E:Urg:source:SE:...).
# =============================================================================

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_FILE="$BASE_DIR/config/sensors/sys_config.json"
GLOBAL_FILE="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/sys_monitor.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"


# --- IMPORT DES BIBLIOTHÈQUES ---
# On utilise BASE_DIR pour éviter les "Not following" de Shellcheck
# shellcheck source=/dev/null
source "$BASE_DIR/scripts/sensors/sys/lib/urgence.sh"
# shellcheck source=/dev/null
source "$BASE_DIR/scripts/sensors/sys/lib/alerts_sys.sh"
# shellcheck source=/dev/null
source "$BASE_DIR/scripts/utils/logger.sh"

# --- INITIALISATION ---
# Création forcée du dossier log s'il n'existe pas
mkdir -p "$(dirname "$LOG_FILE")"
[ ! -f "$LOG_FILE" ] && touch "$LOG_FILE"

# Redéfinition locale de la fonction log pour s'assurer qu'elle écrit dans LOG_FILE
# Si logger.sh définit déjà une fonction log, celle-ci prendra le dessus ou complètera.
log_sys() {
    local level="INFO"
    local message="$1"
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" >> "$LOG_FILE"
    # Optionnel: on l'affiche aussi sur la sortie standard pour le debug
    echo "[$level] $message"
}

# --- FONCTIONS D'ACQUISITION ---
get_cpu_usage() { top -n 1 | awk '/Cpu/ {print $2}' | cut -d. -f1; }
get_thermal()   { termux-sensor -s "Temperature" -n 1 | jq -r '.temperature' 2>/dev/null || echo 0; }
get_storage()   { df -h /sdcard | awk 'NR==2 {print $5}' | tr -d '%'; }
get_uptime()    { awk '{print $1}' /proc/uptime; }

# --- MISE À JOUR GLOBAL.JSON ---
update_global_state() {
    local cpu=$1
    local temp=$2
    local store=$3
    local up=$4
    local date_now
    date_now=$(date '+%Y-%m-%d %H:%M:%S')

    jq --arg cpu "$cpu" \
       --arg temp "$temp" \
       --arg store "$store" \
       --arg up "$up" \
       --arg date "$date_now" \
       '.Sys.cpu_load=($cpu|tonumber) | .Sys.temp=($temp|tonumber) | .Sys.storage=($store|tonumber) | .Sys.uptime=($up|tonumber) | .Sys.last_update=$date' \
       "$GLOBAL_FILE" > "${GLOBAL_FILE}.tmp" && mv "${GLOBAL_FILE}.tmp" "$GLOBAL_FILE"
}

# --- BOUCLE PRINCIPALE ---
main() {
    log "Démarrage du moniteur système"

    while true; do
        # 1. Chargement dynamique de la config (fréquence et seuils)
        local config
        config=$(cat "$CONFIG_FILE")
        local frequence
        frequence=$(jq -r '.frequence // 10' <<< "$config")
        local cpu_threshold
        cpu_threshold=$(jq -r '.thresholds.cpu.alert // 90' <<< "$config")

        # 2. Acquisition des données (CORRIGÉ : assignation complète)
        local cpu_val
        cpu_val=$(get_cpu_usage)
        local thm_val
        thm_val=$(get_thermal)
        local sto_val
        sto_val=$(get_storage)
        local upt_val
        upt_val=$(get_uptime)

        # 3. Mise à jour du Pulse (global.json)
        update_global_state "$cpu_val" "$thm_val" "$sto_val" "$upt_val"

        # 4. LOGIQUE RÉFLEXE : Signalement Urgence si seuil CPU franchi
        if [ "$cpu_val" -gt "$cpu_threshold" ]; then
            # Envoi Événement (E) pour déclencher la sécurité côté SP
            echo "\$E:Urg:source:SE:URG_CPU_CHARGE#" > "$FIFO_IN"
            # Info via COM (I) pour affichage interface
            echo "\$I:COM:warn:SE:\"Surcharge CPU détectée : $cpu_val%\"#" > "$FIFO_IN"
        fi

        # 5. Vérification thermique (Optionnel si tu as une lib externe)
        # check_thermal_alert "$thm_val" 

        sleep "$frequence"
    done
}

main