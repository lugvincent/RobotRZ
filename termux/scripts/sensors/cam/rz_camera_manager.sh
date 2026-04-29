#!/bin/bash
# =============================================================================
# SCRIPT  : rz_camera_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/cam/rz_camera_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire de la caméra du smartphone embarqué (SE).
# Pilotage hybride : Tasker lance/arrête IP Webcam Pro,
# ce script gère les paramètres, URLs et remontées VPIV vers SP.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script est appelé à la demande par rz_vpiv_parser.sh (one-shot).
# Arguments : $1 = propriété VPIV (modeCam | paraCam | snap)
#             $2 = valeur
#             $3 = instance (front | rear)  [défaut : rear]
#
# ARCHITECTURE (v3.0)
# -------------------
# Le lancement et l'arrêt d'IP Webcam Pro sont délégués à Tasker :
#   modeCam:stream|snapshot → fifo_tasker_in → Tasker RZ_CamStart
#   modeCam:off             → fifo_tasker_in → Tasker RZ_CamStop
#
# Ce script intervient APRÈS que Tasker a lancé IP Webcam :
#   1. Attend que le serveur HTTP soit disponible (port 8080)
#   2. Récupère l'IP WiFi locale
#   3. Construit et publie l'URL stream/snapshot vers SP
#   4. Met à jour cam_config.json
#
# MODES CAMÉRA (Table A)
# ----------------------
#   stream   : flux vidéo continu   → URL : http://<ip>:8080/video
#   snapshot : photo unique         → URL : http://<ip>:8080/shot.jpg
#   off      : arrêt IP Webcam Pro  → via Tasker RZ_CamStop
#   error    : état interne sur erreur non critique
#
# LOGIQUE DE PROFIL CPU
# ---------------------
# get_optimal_params() adapte res/fps si CPU > 70% :
#   normal    : paramètres issus de paraCam (valeurs SP)
#   optimized : res "low", fps 10 (profil dégradé cam_config.json)
#
# GESTION ERREURS (Table A)
# -------------------------
#   CAM_START_FAIL  : serveur IP Webcam ne répond pas après timeout
#   CAM_IP_FAIL     : IP wlan0 non lisible
#   CAM_CONFIG_FAIL : cam_config.json illisible ou absent
# Selon CfgS.typePtge :
#   typePtge 0 ou 2 → attente urgDelay s → URG_SENSOR_FAIL
#   autres          → COM:error uniquement
#
# TABLE A — VPIV PRODUITS
# -----------------------
# SP → SE :
#   $V:CamSE:modeCam:<side>:stream#
#   $V:CamSE:modeCam:<side>:snapshot#
#   $V:CamSE:modeCam:<side>:off#
#   $V:CamSE:paraCam:<side>:{"res":"720p","fps":30,"quality":80,"urgDelay":5}#
#   $V:CamSE:snap:<side>:1#
#
# SE → SP :
#   $I:CamSE:modeCam:<side>:stream#       ACK mode appliqué
#   $I:CamSE:modeCam:<side>:off#          ACK arrêt
#   $I:CamSE:modeCam:<side>:error#        Mode erreur positionné
#   $I:CamSE:paraCam:<side>:{...}#        ACK paramètres appliqués
#   $I:CamSE:StreamURL:<side>:<url>#      URL stream ou snapshot
#   $E:CamSE:error:<side>:<CODE>#         Événement erreur
#   $I:COM:error:SE:"CamSE <side> : ..."# Erreur non critique
#   $E:Urg:source:SE:URG_SENSOR_FAIL#     Urgence (typePtge 0 ou 2)
#
# INTERACTIONS
# ------------
#   Appelé par  : rz_vpiv_parser.sh
#   Coordonne   : Tasker (RZ_CamStart / RZ_CamStop) via fifo_tasker_in
#   Lit         : config/sensors/cam_config.json
#   Lit         : config/global.json (.Sys.cpu, .CfgS.typePtge)
#   Écrit dans  : config/sensors/cam_config.json
#   Écrit dans  : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# - IP Webcam Pro (com.pas.webcam.pro) doit être installée sur SE.
# - Le lancement est délégué à Tasker — ce script n'appelle pas am/termux-am.
# - Timeout d'attente serveur : 15s (TIMEOUT_SERVER_S).
# - urgDelay est lu dans cam_config.json par instance.
# - ⚠️ Les trames VPIV commencent par \$ (dollar échappé).
#
# DÉPENDANCES
# -----------
#   jq     : lecture/écriture JSON
#   curl   : vérification disponibilité serveur HTTP
#   ifconfig/ip : lecture IP locale
#   fifo_termux_in : créé par fifo_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (délégation lancement/arrêt à Tasker)
# DATE    : 2026-04-28
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

CONFIG_CAM="$BASE_DIR/config/sensors/cam_config.json"
GLOBAL_JSON="$BASE_DIR/config/global.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/camera.log"

# Port IP Webcam Pro (défaut 8080)
CAM_PORT=8080

# Timeout attente démarrage serveur (secondes)
TIMEOUT_SERVER_S=15

# =============================================================================
# UTILITAIRES
# =============================================================================

log_cam() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [VISION] $1" >> "$LOG_FILE"
}

# Envoi VPIV via FIFO (non-bloquant, watchdog 2s)
send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_OUT" ]; then
        log_cam "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null
}

# Lecture IP locale WiFi (ifconfig puis fallback ip addr)
get_ip() {
    local ip
    ip=$(ifconfig wlan0 2>/dev/null | awk '/inet /{print $2}')
    if [ -z "$ip" ]; then
        ip=$(ip addr show wlan0 2>/dev/null | \
             awk '/inet /{gsub(/\/.*/, "", $2); print $2}')
    fi
    echo "$ip"
}

# Attente que le serveur HTTP soit disponible
# $1 = IP, $2 = timeout (s)
# Retourne 0 si disponible, 1 si timeout
wait_server() {
    local ip="$1"
    local timeout="$2"
    local elapsed=0

    log_cam "Attente serveur http://${ip}:${CAM_PORT}/ (timeout ${timeout}s)..."

    while [ $elapsed -lt $timeout ]; do
        if curl -s --max-time 2 "http://${ip}:${CAM_PORT}/" > /dev/null 2>&1; then
            log_cam "Serveur disponible après ${elapsed}s"
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    log_cam "ERREUR : serveur non disponible après ${timeout}s"
    return 1
}

# =============================================================================
# GESTION ERREURS
# $1 = instance (front|rear)
# $2 = code erreur
# =============================================================================

handle_error() {
    local side="$1"
    local code="$2"
    local type_ptge urg_delay

    log_cam "ERREUR [$side] : $code"

    send_vpiv "\$E:CamSE:error:${side}:${code}#"
    send_mode_state "$side" "error"

    type_ptge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_JSON" 2>/dev/null || echo "0")

    if [[ "$type_ptge" == "0" || "$type_ptge" == "2" ]]; then
        urg_delay=$(jq -r ".${side}.urgDelay // 5" "$CONFIG_CAM" 2>/dev/null || echo "5")
        log_cam "typePtge=$type_ptge : URG_SENSOR_FAIL dans ${urg_delay}s"
        sleep "$urg_delay"
        send_vpiv "\$E:Urg:source:SE:URG_SENSOR_FAIL#"
    else
        send_vpiv "\$I:COM:error:SE:\"CamSE ${side} : ${code}\"#"
    fi
}

# =============================================================================
# MISE À JOUR MODE DANS cam_config.json
# $1 = instance, $2 = mode
# =============================================================================

send_mode_state() {
    local side="$1"
    local mode_val="$2"

    jq --arg side "$side" --arg mode "$mode_val" \
       '.[$side].mode = $mode' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    send_vpiv "\$I:CamSE:modeCam:${side}:${mode_val}#"
    log_cam "modeCam[$side] → $mode_val"
}

# =============================================================================
# FONCTION : notify_system_stop
# Signal priorité caméra aux autres modules SE
# =============================================================================

notify_system_stop() {
    send_vpiv "\$I:COM:info:SE:\"CAMERA_START_PRIORITY\"#"
}

# =============================================================================
# FONCTION : get_optimal_params
# Adapte res/fps selon charge CPU
# =============================================================================

get_optimal_params() {
    local side="$1"
    local cpu_load
    cpu_load=$(jq -r '.Sys.cpu // 0' "$GLOBAL_JSON" 2>/dev/null | cut -d. -f1)

    if [[ "$cpu_load" -gt 70 ]]; then
        log_cam "CPU ${cpu_load}% > 70% : profil optimized"
        echo "optimized"
    else
        echo "normal"
    fi
}

# =============================================================================
# FONCTION : setup_stream
# Appelée APRÈS que Tasker a lancé IP Webcam.
# Attend que le serveur soit prêt, construit et publie l'URL.
# $1 = instance (front|rear)
# $2 = mode (stream|snapshot)
# =============================================================================

setup_stream() {
    local side="$1"
    local target_mode="$2"
    local ip_addr

    # Vérification cam_config.json
    if [ ! -f "$CONFIG_CAM" ]; then
        handle_error "$side" "CAM_CONFIG_FAIL"
        return 1
    fi

    # Récupération IP locale
    ip_addr=$(get_ip)
    if [ -z "$ip_addr" ]; then
        handle_error "$side" "CAM_IP_FAIL"
        return 1
    fi

    # Signal priorité aux autres modules
    notify_system_stop

    # Attente que le serveur IP Webcam soit prêt
    # (Tasker l'a lancé juste avant via fifo_tasker_in)
    if ! wait_server "$ip_addr" "$TIMEOUT_SERVER_S"; then
        handle_error "$side" "CAM_START_FAIL"
        return 1
    fi

    # Mise à jour last_ip et current_side
    jq --arg ip "$ip_addr" '.status.last_ip = $ip' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    jq --arg side "$side" '.status.current_side = $side' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # Construction URL selon mode
    local stream_url
    if [ "$target_mode" == "snapshot" ]; then
        stream_url="http://${ip_addr}:${CAM_PORT}/shot.jpg"
    else
        stream_url="http://${ip_addr}:${CAM_PORT}/video"
    fi

    # Mise à jour streamURL dans l'instance
    jq --arg side "$side" --arg url "$stream_url" \
       '.[$side].streamURL = $url' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # ACK StreamURL → SP
    send_vpiv "\$I:CamSE:StreamURL:${side}:${stream_url}#"

    # ACK mode → SP
    send_mode_state "$side" "$target_mode"

    log_cam "[$side] $target_mode prêt → $stream_url"
}

# =============================================================================
# FONCTION : stop_cam
# Mise à jour état après arrêt par Tasker
# $1 = instance (front|rear|*)
# =============================================================================

stop_cam() {
    local side="$1"

    log_cam "Arrêt caméra [$side] — IP Webcam arrêtée par Tasker"

    if [ "$side" == "*" ]; then
        jq '.front.mode = "off" | .rear.mode = "off"' \
           "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
        && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"
        send_vpiv "\$I:CamSE:modeCam:front:off#"
        send_vpiv "\$I:CamSE:modeCam:rear:off#"
    else
        send_mode_state "$side" "off"
    fi
}

# =============================================================================
# FONCTION : take_snapshot
# Génère l'URL snapshot depuis IP Webcam en cours
# $1 = instance (front|rear)
# =============================================================================

take_snapshot() {
    local side="$1"
    local ip_addr

    ip_addr=$(get_ip)
    if [ -z "$ip_addr" ]; then
        handle_error "$side" "CAM_IP_FAIL"
        return 1
    fi

    # Vérifier que le serveur tourne
    if ! curl -s --max-time 2 "http://${ip_addr}:${CAM_PORT}/" > /dev/null 2>&1; then
        log_cam "WARN : serveur non disponible pour snapshot [$side]"
        handle_error "$side" "CAM_START_FAIL"
        return 1
    fi

    local snap_url="http://${ip_addr}:${CAM_PORT}/shot.jpg"
    log_cam "Snapshot [$side] : $snap_url"

    jq --arg side "$side" --arg url "$snap_url" \
       '.[$side].streamURL = $url' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    send_vpiv "\$I:CamSE:StreamURL:${side}:${snap_url}#"
}

# =============================================================================
# FONCTION : apply_para_cam
# Applique les paramètres paraCam reçus de SP
# $1 = instance, $2 = JSON paramètres
# =============================================================================

apply_para_cam() {
    local side="$1"
    local params="$2"

    if ! echo "$params" | jq '.' > /dev/null 2>&1; then
        log_cam "ERREUR : paraCam JSON invalide pour $side : $params"
        send_vpiv "\$I:COM:error:SE:\"CamSE ${side} : paraCam JSON invalide\"#"
        return 1
    fi

    local res fps quality urg_delay
    res=$(      echo "$params" | jq -r '.res      // empty')
    fps=$(      echo "$params" | jq -r '.fps      // empty')
    quality=$(  echo "$params" | jq -r '.quality  // empty')
    urg_delay=$(echo "$params" | jq -r '.urgDelay // empty')

    local tmp
    tmp=$(jq --arg side      "$side" \
             --arg res       "${res:-null}" \
             --arg fps       "${fps:-null}" \
             --arg quality   "${quality:-null}" \
             --arg urgDelay  "${urg_delay:-null}" \
          '
          if $res      != "null" then .[$side].res      = ($res|tonumber? // $res) else . end |
          if $fps      != "null" then .[$side].fps      = ($fps|tonumber)           else . end |
          if $quality  != "null" then .[$side].quality  = ($quality|tonumber)       else . end |
          if $urgDelay != "null" then .[$side].urgDelay = ($urgDelay|tonumber)      else . end
          ' "$CONFIG_CAM")

    echo "$tmp" > "$CONFIG_CAM"

    local ack
    ack=$(jq -c ".${side} | {res,fps,quality,urgDelay}" "$CONFIG_CAM")
    send_vpiv "\$I:CamSE:paraCam:${side}:${ack}#"
    log_cam "paraCam[$side] appliqué : $ack"
}

# =============================================================================
# AIGUILLAGE PROPRIÉTÉS VPIV
# =============================================================================

PROP="$1"
VAL="$2"
SIDE="${3:-rear}"

# Validation instance
if [[ "$SIDE" != "front" && "$SIDE" != "rear" && "$SIDE" != "*" ]]; then
    log_cam "ERREUR : instance inconnue '$SIDE'"
    exit 1
fi

case "$PROP" in

    "modeCam")
        case "$VAL" in
            "off")
                # Tasker a déjà arrêté IP Webcam via fifo_tasker_in
                # Ce script met juste à jour l'état
                stop_cam "$SIDE"
                ;;
            "stream"|"snapshot")
                # Tasker a lancé IP Webcam via fifo_tasker_in
                # Ce script attend que le serveur soit prêt et publie l'URL
                setup_stream "$SIDE" "$VAL" &
                ;;
            *)
                log_cam "ERREUR : modeCam '$VAL' inconnu"
                send_vpiv "\$I:COM:error:SE:\"CamSE ${SIDE} : modeCam inconnu '${VAL}'\"#"
                exit 1
                ;;
        esac
        ;;

    "paraCam")
        apply_para_cam "$SIDE" "$VAL"
        ;;

    "snap")
        take_snapshot "$SIDE"
        ;;

    *)
        log_cam "Propriété inconnue : '$PROP'"
        send_vpiv "\$I:COM:error:SE:\"CamSE : propriété inconnue '${PROP}'\"#"
        exit 1
        ;;
esac
