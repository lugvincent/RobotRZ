#!/bin/bash
# =============================================================================
# SCRIPT: rz_camera_manager.sh
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/sensors/cam/rz_camera_manager.sh
# VERSION: 1.4 (Intégration Snapshot + ACK synchronisés + éviter latence)
# DESCRIPTION: Pilotage hybride de la caméra via IP Webcam (Intents Android)
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_CAM="$BASE_DIR/config/sensors/cam_config.json"
GLOBAL_JSON="$BASE_DIR/config/global.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/camera.log"

log_cam() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] [VISION] $1" >> "$LOG_FILE"; }

notify_system_stop() {
    echo "\$I:Com:info:SE:\"CAMERA_START_PRIORITY\"#" > "$FIFO_OUT"
}

get_optimal_params() {
    local cpu_load
    cpu_load=$(jq -r '.Sys.cpu_load // 0' "$GLOBAL_JSON" | cut -d. -f1)
    [[ "$cpu_load" -gt 70 ]] && echo "optimized" || echo "normal"
}

start_webcam() {
    local side
    local profile
    local res
    local fps
    local cam_id
    local ip_addr

    side="$1"
    profile=$(get_optimal_params)
    res=$(jq -r ".profiles.$profile.res" "$CONFIG_CAM")
    fps=$(jq -r ".profiles.$profile.fps" "$CONFIG_CAM")

    notify_system_stop
    sleep 1

    cam_id=0
    [[ "$side" == "front" ]] && cam_id=1

    log_cam "Start IP Webcam: $side ($res @ $fps FPS)"
    
    am start -n com.pas.webcam/.Rolling \
        --ei "com.pas.webcam.EXTRA_CAMERA" "$cam_id" \
        --es "com.pas.webcam.EXTRA_RESOLUTION" "$res" \
        --ei "com.pas.webcam.EXTRA_FPS_LIMIT" "$fps"

    ip_addr=$(ifconfig wlan0 | grep 'inet ' | awk '{print $2}')
    
    # ACKS : Flux et État
    echo "\$F:CamSE:streamURL:$side:{http://$ip_addr:8080/video}#" > "$FIFO_OUT"
    echo "\$I:CamSE:active:$side:On#" > "$FIFO_OUT"
    
    jq --arg side "$side" '.status.active = "On" | .status.current_side = $side' "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"
}

stop_webcam() {
    log_cam "Arrêt de la vision."
    am force-stop com.pas.webcam
    echo "\$I:CamSE:active:*:Off#" > "$FIFO_OUT"
    jq '.status.active = "Off"' "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"
}

take_snapshot() {
    local ip_addr
    local snap_url
    ip_addr=$(ifconfig wlan0 | grep 'inet ' | awk '{print $2}')
    snap_url="http://$ip_addr:8080/shot.jpg"
    
    log_cam "Snapshot: $snap_url"
    echo "\$I:CamSE:snapURL:*:{$snap_url}#" > "$FIFO_OUT"
}

# --- D. LOGIQUE D'AIGUILLAGE CORRIGÉE ---
case "$1" in
    "active")
        if [[ "$2" == "On" ]]; then
            start_webcam "rear" & 
        else
            stop_webcam
        fi
        ;;
    "mode")
        if [[ "$2" == "front" || "$2" == "rear" ]]; then
            start_webcam "$2" &
        else
            # On récupère l'état actuel pour basculer
            current_side=$(jq -r '.status.current_side // "rear"' "$CONFIG_CAM")
            if [[ "$current_side" == "rear" ]]; then
                start_webcam "front" &
            else
                start_webcam "rear" &
            fi
        fi
        ;;
    "(snap)")
        take_snapshot
        ;;
esac