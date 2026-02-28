#!/bin/bash
# =============================================================================
# SCRIPT: rz_torch_manager.sh
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/sensors/torch/rz_torch_manager.sh
# DESCRIPTION: Gestionnaire indépendant de la torche (Flash)
#              Bascule automatique entre mode natif et mode caméra (IP Webcam).
# VERSION: 1.2 (Séparation déclarations/instanciations + ACK)
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_CAM="$BASE_DIR/config/sensors/cam_config.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/torch.log"

log_torch() { 
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [TORCH] $1" >> "$LOG_FILE" 
}

set_torch() {
    local state
    local cam_active
    local ip_local
    local cmd

    state="$1" 
    # Lecture de l'état actuel de la caméra dans le JSON centralisé
    cam_active=$(jq -r '.status.active // "Off"' "$CONFIG_CAM")

    if [ "$cam_active" == "On" ]; then
        log_torch "Mode Caméra détecté : Commande via API HTTP."
        ip_local=$(ifconfig wlan0 | grep 'inet ' | awk '{print $2}')
        
        cmd="disablels"
        [[ "$state" == "On" ]] && cmd="enablels"
        
        # Appel asynchrone pour ne pas bloquer le manager
        curl -s "http://$ip_local:8080/$cmd" > /dev/null &
    else
        log_torch "Mode Autonome : Commande via Termux-Torch."
        if [ "$state" == "On" ]; then
            termux-torch on
        else
            termux-torch off
        fi
    fi

    # --- ACK VPIV DE SYNCHRONISATION ---
    echo "\$I:TorchSE:active:*:$state#" > "$FIFO_OUT"
}

# --- LOGIQUE D'AIGUILLAGE ---
case "$1" in
    "active")
        set_torch "$2"
        ;;
    *)
        log_torch "Propriété inconnue : $1"
        ;;
esac