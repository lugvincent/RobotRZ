#!/bin/bash
# =============================================================================
# SCRIPT  : rz_camera_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/cam/rz_camera_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire de la caméra du smartphone embarqué (SE).
# Pilotage via Tasker (rz_tasker_bridge.sh → RZ_CamStart/RZ_CamStop).
# Gère deux instances : front (caméra avant) et rear (caméra arrière).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script est appelé à la demande par rz_vpiv_parser.sh (one-shot, pas daemon).
# Arguments : $1 = propriété VPIV (modeCam | paraCam | snap)
#             $2 = valeur
#             $3 = instance (front | rear | *)  [défaut : rear]
#
# MODES CAMÉRA
# ------------
#   off      : arrêt IP Webcam via Tasker (RZ_CamStop)
#   stream   : démarrage flux vidéo continu via Tasker (RZ_CamStart)
#   snapshot : URL photo instantanée (IP Webcam déjà actif)
#   error    : état interne positionné automatiquement sur erreur non critique
#
# URL STREAM DYNAMIQUE SELON typeReseau
# -------------------------------------
# L'URL publiée vers SP dépend de CfgS.typeReseau dans global.json :
#   WifiInternetBox : http://{ip_se}:8080/video  (réseau box locale)
#   Wifi4KSP        : http://{ip_se}:8080/video  (hotspot SP smartphone)
#   WifiSPSSI       : http://{ip_se}:8080/video  (réseau SP SSID connu)
#   4K              : rtmp://{duckdns}/live/rz    (SIM dédiée SE — RTMP)
# L'IP SE est détectée dynamiquement via wlan0 — valide quel que soit le réseau.
#
# LOGIQUE DE PROFIL CPU
# ---------------------
# get_optimal_params() adapte res/fps si CPU > 70% :
#   normal    : res et fps issus de paraCam (valeurs SP)
#   optimized : res "low", fps 10 (profil dégradé cam_config.json)
# Le profil CPU prend le dessus sur paraCam en cas de surcharge.
#
# GESTION ERREURS (Table A)
# -------------------------
# Sur erreur : envoi $E:CamSE:(error):<side>:<CODE>#
#              + passage mode:error via send_mode_error()
# Selon CfgS.typePtge dans global.json :
#   typePtge 0 (Ecran) ou 2 (Joystick) → attente urgDelay s → URG_SENSOR_FAIL
#   Autres typePtge                     → COM:error uniquement
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
# CODES ERREUR
# ------------
#   CAM_START_FAIL  : échec démarrage IP Webcam (Tasker bridge timeout)
#   CAM_IP_FAIL     : IP wlan0 non lisible
#   CAM_CONFIG_FAIL : cam_config.json illisible ou absent
#
# INTERACTIONS
# ------------
#   Appelé par  : rz_vpiv_parser.sh ($PROP $VAL $INST)
#   Lit         : config/sensors/cam_config.json (paramètres et état instances)
#   Lit         : config/global.json (.Sys.cpu, .CfgS.typePtge, .CfgS.typeReseau)
#   Écrit dans  : config/sensors/cam_config.json (mise à jour mode/streamURL)
#   Écrit dans  : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#   Appelle     : scripts/utils/rz_tasker_bridge.sh (RZ_CamStart / RZ_CamStop)
#
# PRÉCAUTIONS
# -----------
# - IP Webcam Pro doit être installée : com.pas.webcam.pro
# - Tasker doit être actif en arrière-plan avec RZ_CamStart et RZ_CamStop
# - rz_tasker_bridge.sh doit être exécutable
# - ifconfig wlan0 : fallback ip addr si absent
# - urgDelay est lu dans cam_config.json par instance (paraCam)
# - ⚠️ Les trames VPIV commencent par \$ (dollar échappé)
# - Pour typeReseau=4K (SIM), le streaming RTMP n'est pas encore implémenté
#   → fallback sur URL locale en attendant
#
# DÉPENDANCES
# -----------
#   jq                  : lecture/écriture JSON
#   curl                : vérification port 8080 après démarrage Tasker
#   rz_tasker_bridge.sh : déclenchement tâches Tasker
#   ifconfig / ip addr  : lecture IP wlan0
#   fifo_termux_in      : créé par fifo_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (Tasker bridge, URL dynamique typeReseau, fix $INST depuis parser)
# DATE    : 2026-05-03
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
BRIDGE_SCRIPT="$BASE_DIR/scripts/utils/rz_tasker_bridge.sh"

# URL RTMP pour streaming distant (typeReseau=4K)
#RTMP_URL="rtmp://robotz-vincent.duckdns.org/live/rz"

# Délai attente démarrage IP Webcam après appel Tasker (secondes)
CAM_START_TIMEOUT=8

# Port IP Webcam
CAM_PORT=8080

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

# Lecture IP locale (ifconfig puis fallback ip addr)
get_ip() {
    local ip
    ip=$(ifconfig wlan0 2>/dev/null | awk '/inet /{print $2}')
    if [ -z "$ip" ]; then
        ip=$(ip addr show wlan0 2>/dev/null | awk '/inet /{gsub(/\/.*/, "", $2); print $2}')
    fi
    echo "$ip"
}

# Construction URL stream selon typeReseau
# $1 = mode (stream|snapshot)
# $2 = ip_addr
get_stream_url() {
    local mode="$1"
    local ip_addr="$2"
    local type_reseau

    type_reseau=$(jq -r '.CfgS.typeReseau // "WifiInternetBox"' "$GLOBAL_JSON" 2>/dev/null)

    case "$type_reseau" in
        "4K")
            # SIM dédiée SE — streaming RTMP vers serveur distant
            # ⚠️ RTMP non encore implémenté côté serveur → fallback URL locale
            if [ "$mode" == "snapshot" ]; then
                echo "http://${ip_addr}:${CAM_PORT}/shot.jpg"
            else
                log_cam "typeReseau=4K : RTMP pas encore implémenté, fallback URL locale"
                echo "http://${ip_addr}:${CAM_PORT}/video"
            fi
            ;;
        *)
            # Tous les modes WiFi (WifiInternetBox, Wifi4KSP, WifiSPSSI)
            # → URL locale avec IP courante du Doro (détectée dynamiquement)
            if [ "$mode" == "snapshot" ]; then
                echo "http://${ip_addr}:${CAM_PORT}/shot.jpg"
            else
                echo "http://${ip_addr}:${CAM_PORT}/video"
            fi
            ;;
    esac
}

# =============================================================================
# GESTION ERREURS
# Envoie error + modeCam:error + URG ou COM:error selon typePtge
# $1 = instance (front|rear)
# $2 = code erreur (CAM_START_FAIL | CAM_IP_FAIL | CAM_CONFIG_FAIL)
# =============================================================================

handle_error() {
    local side="$1"
    local code="$2"
    local type_ptge
    local urg_delay

    log_cam "ERREUR [$side] : $code"

    # Événement erreur caméra → SP
    send_vpiv "\$E:CamSE:error:${side}:${code}#"

    # Passage modeCam:error dans cam_config.json
    send_mode_state "$side" "error"

    # Lecture typePtge dans global.json
    type_ptge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_JSON" 2>/dev/null || echo "0")

    if [[ "$type_ptge" == "0" || "$type_ptge" == "2" ]]; then
        # typePtge Ecran (0) ou Joystick (2) : perte caméra = perte contrôle visuel
        urg_delay=$(jq -r ".${side}.urgDelay // 5" "$CONFIG_CAM" 2>/dev/null || echo "5")
        log_cam "typePtge=$type_ptge : URG dans ${urg_delay}s (perte contrôle visuel)"
        sleep "$urg_delay"
        send_vpiv "\$E:Urg:source:SE:URG_SENSOR_FAIL#"
    else
        # Autres typePtge : erreur non critique → COM:error
        send_vpiv "\$I:COM:error:SE:\"CamSE ${side} : ${code}\"#"
    fi
}

# =============================================================================
# MISE À JOUR mode DANS cam_config.json (par instance)
# $1 = instance (front|rear)
# $2 = mode (off|stream|snapshot|error)
# =============================================================================

send_mode_state() {
    local side="$1"
    local mode_val="$2"

    jq --arg side "$side" --arg mode "$mode_val" \
       '.[$side].mode = $mode' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # ACK modeCam → SP
    send_vpiv "\$I:CamSE:modeCam:${side}:${mode_val}#"
    log_cam "modeCam[$side] → $mode_val"
}

# =============================================================================
# FONCTION : notify_system_stop
# Informe les autres modules SE que la caméra prend la priorité ressources
# =============================================================================

notify_system_stop() {
    send_vpiv "\$I:COM:info:SE:\"CAMERA_START_PRIORITY\"#"
}

# =============================================================================
# FONCTION : get_optimal_params
# Adapte res/fps selon la charge CPU (profil normal ou optimized)
# $1 = instance (front|rear)
# Retourne : "normal" ou "optimized"
# =============================================================================

get_optimal_params() {
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
# FONCTION : start_webcam
# Démarre IP Webcam via Tasker (RZ_CamStart) et publie l'URL stream vers SP
# $1 = instance (front|rear)
# $2 = mode (stream|snapshot)
# =============================================================================

start_webcam() {
    local side="$1"
    local target_mode="$2"
    local profile
    local res
    local fps
    local ip_addr
    local stream_url

    # Vérification cam_config.json accessible
    if [ ! -f "$CONFIG_CAM" ]; then
        handle_error "$side" "CAM_CONFIG_FAIL"
        return 1
    fi

    # Profil CPU : normal ou optimized
    profile=$(get_optimal_params "$side")

    # Paramètres : priorité profil CPU, sinon paraCam de l'instance
    if [ "$profile" == "optimized" ]; then
        res=$(jq -r '.profiles.optimized.res' "$CONFIG_CAM")
        fps=$(jq -r '.profiles.optimized.fps' "$CONFIG_CAM")
    else
        res=$(jq -r ".${side}.res // \"720p\"" "$CONFIG_CAM")
        fps=$(jq -r ".${side}.fps // 30"       "$CONFIG_CAM")
    fi

    # Signal priorité aux autres modules
    notify_system_stop
    sleep 1

    log_cam "Démarrage IP Webcam : $side mode=$target_mode res=$res fps=$fps (profil=$profile)"

    # -------------------------------------------------------------------------
    # DÉLÉGATION À TASKER via rz_tasker_bridge.sh
    # RZ_CamStart reçoit l'instance (rear/front) en paramètre
    # Tasker lance IP Webcam Pro et active le serveur en arrière-plan
    # -------------------------------------------------------------------------
    if [ ! -f "$BRIDGE_SCRIPT" ]; then
        log_cam "ERREUR : rz_tasker_bridge.sh introuvable ($BRIDGE_SCRIPT)"
        handle_error "$side" "CAM_START_FAIL"
        return 1
    fi

    bash "$BRIDGE_SCRIPT" "RZ_CamStart" "$side"
    log_cam "Tasker RZ_CamStart déclenché pour $side — attente ${CAM_START_TIMEOUT}s"

    # -------------------------------------------------------------------------
    # VÉRIFICATION DÉMARRAGE : attente puis test port 8080
    # Si IP absente → CAM_IP_FAIL
    # Si port muet après timeout → CAM_START_FAIL
    # -------------------------------------------------------------------------
    sleep "$CAM_START_TIMEOUT"

    ip_addr=$(get_ip)
    if [ -z "$ip_addr" ]; then
        handle_error "$side" "CAM_IP_FAIL"
        return 1
    fi

    if ! curl -s --max-time 2 "http://${ip_addr}:${CAM_PORT}/" > /dev/null 2>&1; then
        handle_error "$side" "CAM_START_FAIL"
        return 1
    fi

    # -------------------------------------------------------------------------
    # MISE À JOUR cam_config.json
    # -------------------------------------------------------------------------
    jq --arg ip "$ip_addr" '.status.last_ip = $ip' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    jq --arg side "$side" '.status.current_side = $side' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # -------------------------------------------------------------------------
    # CONSTRUCTION URL DYNAMIQUE selon typeReseau
    # -------------------------------------------------------------------------
    stream_url=$(get_stream_url "$target_mode" "$ip_addr")

    # Mise à jour streamURL dans l'instance
    jq --arg side "$side" --arg url "$stream_url" \
       '.[$side].streamURL = $url' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # ACK StreamURL → SP
    send_vpiv "\$I:CamSE:StreamURL:${side}:${stream_url}#"
    log_cam "StreamURL[$side] → $stream_url"

    # ACK mode → SP
    send_mode_state "$side" "$target_mode"
}

# =============================================================================
# FONCTION : stop_webcam
# Arrête IP Webcam via Tasker (RZ_CamStop) et remet mode à "off" sur l'instance
# $1 = instance (front|rear|*) — si "*" : arrêt global
# =============================================================================

stop_webcam() {
    local side="$1"

    log_cam "Arrêt IP Webcam [$side] via Tasker RZ_CamStop"

    # Délégation à Tasker
    bash "$BRIDGE_SCRIPT" "RZ_CamStop" "$side"

    if [ "$side" == "*" ]; then
        # Arrêt global : les deux instances passent en off
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
# Génère l'URL snapshot immédiate depuis IP Webcam en cours
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

    local snap_url
    snap_url=$(get_stream_url "snapshot" "$ip_addr")
    log_cam "Snapshot [$side] : $snap_url"

    # Mise à jour streamURL dans l'instance
    jq --arg side "$side" --arg url "$snap_url" \
       '.[$side].streamURL = $url' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # Retour URL → SP via StreamURL
    send_vpiv "\$I:CamSE:StreamURL:${side}:${snap_url}#"
}

# =============================================================================
# FONCTION : apply_para_cam
# Applique les paramètres paraCam reçus de SP pour une instance
# $1 = instance (front|rear)
# $2 = JSON paramètres : {"res":"720p","fps":30,"quality":80,"urgDelay":5}
# =============================================================================

apply_para_cam() {
    local side="$1"
    local params="$2"

    # Validation JSON minimal
    if ! echo "$params" | jq '.' > /dev/null 2>&1; then
        log_cam "ERREUR : paraCam JSON invalide pour $side : $params"
        send_vpiv "\$I:COM:error:SE:\"CamSE ${side} : paraCam JSON invalide\"#"
        return 1
    fi

    # Extraction des champs (valeurs existantes conservées si champ absent)
    local res fps quality urg_delay
    res=$(      echo "$params" | jq -r '.res       // empty')
    fps=$(      echo "$params" | jq -r '.fps       // empty')
    quality=$(  echo "$params" | jq -r '.quality   // empty')
    urg_delay=$(echo "$params" | jq -r '.urgDelay  // empty')

    # Mise à jour sélective dans cam_config.json
    local tmp
    tmp=$(jq --arg side "$side" \
             --arg res      "${res:-null}" \
             --arg fps      "${fps:-null}" \
             --arg quality  "${quality:-null}" \
             --arg urgDelay "${urg_delay:-null}" \
          '
          if $res      != "null" then .[$side].res       = ($res|tonumber? // $res)  else . end |
          if $fps      != "null" then .[$side].fps       = ($fps|tonumber)           else . end |
          if $quality  != "null" then .[$side].quality   = ($quality|tonumber)       else . end |
          if $urgDelay != "null" then .[$side].urgDelay  = ($urgDelay|tonumber)      else . end
          ' "$CONFIG_CAM")

    echo "$tmp" > "$CONFIG_CAM"

    # ACK paraCam → SP avec les valeurs finales de l'instance
    local ack
    ack=$(jq -c ".${side} | {res,fps,quality,urgDelay}" "$CONFIG_CAM")
    send_vpiv "\$I:CamSE:paraCam:${side}:${ack}#"
    log_cam "paraCam[$side] appliqué : $ack"
}

# =============================================================================
# AIGUILLAGE PROPRIÉTÉS VPIV
# $1 = propriété VPIV (modeCam | paraCam | snap)
# $2 = valeur
# $3 = instance (front | rear | *)  [défaut : rear]
# ⚠️ $3 est transmis depuis rz_vpiv_parser.sh — corrigé en v3.0
# =============================================================================

PROP="$1"
VAL="$2"
SIDE="${3:-rear}"    # Instance par défaut : rear

# Validation instance
if [[ "$SIDE" != "front" && "$SIDE" != "rear" && "$SIDE" != "*" ]]; then
    log_cam "ERREUR : instance inconnue '$SIDE' (attendu : front|rear|*)"
    exit 1
fi

log_cam "Appel : PROP=$PROP VAL=$VAL SIDE=$SIDE"

case "$PROP" in

    # -----------------------------------------------------------------------
    # modeCam : commande principale (off | stream | snapshot)
    # -----------------------------------------------------------------------
    "modeCam")
        case "$VAL" in
            "off")
                stop_webcam "$SIDE"
                ;;
            "stream"|"snapshot")
                start_webcam "$SIDE" "$VAL" &
                ;;
            *)
                log_cam "ERREUR : modeCam '$VAL' inconnu (attendu : off|stream|snapshot)"
                send_vpiv "\$I:COM:error:SE:\"CamSE ${SIDE} : modeCam inconnu '${VAL}'\"#"
                exit 1
                ;;
        esac
        ;;

    # -----------------------------------------------------------------------
    # paraCam : mise à jour paramètres d'une instance
    # -----------------------------------------------------------------------
    "paraCam")
        apply_para_cam "$SIDE" "$VAL"
        ;;

    # -----------------------------------------------------------------------
    # snap : capture snapshot immédiate — répond via StreamURL
    # -----------------------------------------------------------------------
    "snap")
        take_snapshot "$SIDE"
        ;;

    # -----------------------------------------------------------------------
    # Propriété inconnue
    # -----------------------------------------------------------------------
    *)
        log_cam "Propriété inconnue ou non supportée : '$PROP'"
        send_vpiv "\$I:COM:error:SE:\"CamSE : propriété inconnue '${PROP}'\"#"
        exit 1
        ;;
esac