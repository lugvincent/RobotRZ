#!/bin/bash
# =============================================================================
# SCRIPT  : rz_camera_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/cam/rz_camera_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire de la caméra du smartphone embarqué (SE).
# Pilotage hybride via IP Webcam (application Android) et Android Intents (am).
# Gère deux instances : front (caméra avant) et rear (caméra arrière).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script est appelé à la demande par rz_vpiv_parser.sh (one-shot, pas daemon).
# Arguments : $1 = propriété VPIV (mode | paraCam | (snap))
#             $2 = valeur
#             $3 = instance (front | rear)  [optionnel selon propriété]
#
# MODES CAMÉRA
# ------------
#   off      : arrêt IP Webcam (am force-stop)
#   stream   : démarrage flux vidéo continu (IP Webcam)
#   snapshot : démarrage IP Webcam en mode photo (shot.jpg)
#   error    : état interne positionné automatiquement sur erreur non critique
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
#   $V:CamSE:mode:<side>:stream#
#   $V:CamSE:mode:<side>:snapshot#
#   $V:CamSE:mode:<side>:off#
#   $V:CamSE:paraCam:<side>:{"res":"720p","fps":30,"quality":80,"urgDelay":5}#
#   $V:CamSE:(snap):<side>:OK#
#
# SE → SP :
#   $I:CamSE:mode:<side>:stream#          ACK mode appliqué
#   $I:CamSE:mode:<side>:off#             ACK arrêt
#   $I:CamSE:mode:<side>:error#           Mode erreur positionné
#   $I:CamSE:paraCam:<side>:{...}#        ACK paramètres appliqués
#   $I:CamSE:StreamURL:<side>:<url>#      URL stream ou snapshot
#   $E:CamSE:(error):<side>:<CODE>#       Événement erreur
#   $I:COM:error:SE:"CamSE <side> : ..."# Erreur non critique
#   $E:Urg:source:SE:URG_SENSOR_FAIL#     Urgence (typePtge 0 ou 2)
#
# CODES ERREUR
# ------------
#   CAM_START_FAIL  : échec démarrage IP Webcam (am start)
#   CAM_IP_FAIL     : IP wlan0 non lisible
#   CAM_CONFIG_FAIL : cam_config.json illisible ou absent
#
# INTERACTIONS
# ------------
#   Appelé par  : rz_vpiv_parser.sh
#   Lit         : config/sensors/cam_config.json (paramètres et état instances)
#   Lit         : config/global.json (.Sys.cpu, .CfgS.typePtge)
#   Écrit dans  : config/sensors/cam_config.json (mise à jour mode/streamURL)
#   Écrit dans  : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# - am (Android Manager) est disponible uniquement sur smartphone.
# - IP Webcam doit être installée : com.pas.webcam
# - ifconfig wlan0 : fallback ip addr si absent.
# - urgDelay est lu dans cam_config.json par instance (paraCam).
# - ⚠️ Les trames VPIV commencent par \$ (dollar échappé).
# - notify_system_stop() informe les autres modules SE de la priorité caméra.
#
# DÉPENDANCES
# -----------
#   jq       : lecture/écriture JSON
#   am       : Android Manager (démarrage/arrêt IP Webcam)
#   ifconfig : lecture IP (ou ip addr en fallback)
#   fifo_termux_in : créé par fifo_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (Table A complète, instances front/rear, mode absorbe active,
#                  paraCam, gestion erreur URG/COM selon typePtge)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

CONFIG_CAM="$BASE_DIR/config/sensors/cam_config.json"
GLOBAL_JSON="$BASE_DIR/config/global.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/camera.log"

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

# =============================================================================
# GESTION ERREURS
# Envoie (error) + mode:error + URG ou COM:error selon typePtge
# $1 = instance (front|rear)
# $2 = code erreur (CAM_START_FAIL | CAM_IP_FAIL | CAM_CONFIG_FAIL)
# =============================================================================

handle_error() {
    local side="$1"
    local code="$2"
    local type_ptge
    local urg_delay

    log_cam "ERREUR [$side] : $code"

    # Événement erreur caméra → SP (Table A : (error) CAT=E)
    send_vpiv "\$E:CamSE:(error):${side}:${code}#"

    # Passage mode:error dans cam_config.json
    send_mode_state "$side" "error"

    # Lecture typePtge dans global.json
    type_ptge=$(jq -r '.CfgS.typePtge // 0' "$GLOBAL_JSON" 2>/dev/null || echo "0")

    if [[ "$type_ptge" == "0" || "$type_ptge" == "2" ]]; then
        # typePtge Ecran (0) ou Joystick (2) : perte caméra = perte contrôle visuel
        # → attente urgDelay secondes puis URG_SENSOR_FAIL
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

    # Mise à jour JSON (clé dynamique via jq --arg)
    jq --arg side "$side" --arg mode "$mode_val" \
       '.[$side].mode = $mode' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # ACK mode → SP
    send_vpiv "\$I:CamSE:mode:${side}:${mode_val}#"
    log_cam "mode[$side] → $mode_val"
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
# Retourne : "normal" ou "optimized" (utilisé pour lire cam_config.profiles)
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
# FONCTION : start_webcam
# Démarre IP Webcam sur l'instance demandée en mode stream ou snapshot
# $1 = instance (front|rear)
# $2 = mode (stream|snapshot)
# =============================================================================

start_webcam() {
    local side="$1"
    local target_mode="$2"
    local profile
    local res
    local fps
    local cam_id
    local ip_addr

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
        # Lecture des paramètres de l'instance dans paraCam
        res=$(jq -r ".${side}.res // \"720p\"" "$CONFIG_CAM")
        fps=$(jq -r ".${side}.fps // 30"       "$CONFIG_CAM")
    fi

    # Signal priorité aux autres modules
    notify_system_stop
    sleep 1

    # Identifiant caméra Android : 0 = rear, 1 = front
    cam_id=0
    [[ "$side" == "front" ]] && cam_id=1

    log_cam "Démarrage IP Webcam : $side mode=$target_mode res=$res fps=$fps (profil=$profile)"

    # Démarrage IP Webcam via Android Intent
    am start -n com.pas.webcam/.Rolling \
        --ei "com.pas.webcam.EXTRA_CAMERA"     "$cam_id" \
        --es "com.pas.webcam.EXTRA_RESOLUTION" "$res"    \
        --ei "com.pas.webcam.EXTRA_FPS_LIMIT"  "$fps"

    # Vérification démarrage : attente 3s puis lecture IP
    # Si IP absente → problème réseau (CAM_IP_FAIL)
    # Si IP présente mais port muet → IP Webcam na pas démarré (CAM_START_FAIL)
    sleep 3
    ip_addr=$(get_ip)

    if [ -z "$ip_addr" ]; then
        handle_error "$side" "CAM_IP_FAIL"
        return 1
    fi

    # Vérification que IP Webcam répond sur le port 8080 (timeout 2s)
    if ! curl -s --max-time 2 "http://${ip_addr}:8080/" > /dev/null 2>&1; then
        handle_error "$side" "CAM_START_FAIL"
        return 1
    fi

    # Mise à jour last_ip dans cam_config.json
    jq --arg ip "$ip_addr" '.status.last_ip = $ip' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # Mise à jour current_side
    jq --arg side "$side" '.status.current_side = $side' \
       "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
    && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"

    # Construction URL selon mode
    local stream_url
    if [ "$target_mode" == "snapshot" ]; then
        stream_url="http://${ip_addr}:8080/shot.jpg"
    else
        stream_url="http://${ip_addr}:8080/video"
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
}

# =============================================================================
# FONCTION : stop_webcam
# Arrête IP Webcam et remet mode à "off" sur l'instance
# $1 = instance (front|rear) — si "*" : arrêt global
# =============================================================================

stop_webcam() {
    local side="$1"

    log_cam "Arrêt IP Webcam [$side]"
    am force-stop com.pas.webcam

    if [ "$side" == "*" ]; then
        # Arrêt global : les deux instances passent en off
        jq '.front.mode = "off" | .rear.mode = "off"' \
           "$CONFIG_CAM" > "${CONFIG_CAM}.tmp" \
        && mv "${CONFIG_CAM}.tmp" "$CONFIG_CAM"
        send_vpiv "\$I:CamSE:mode:front:off#"
        send_vpiv "\$I:CamSE:mode:rear:off#"
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

    local snap_url="http://${ip_addr}:8080/shot.jpg"
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

    # Mise à jour sélective dans cam_config.json (uniquement les champs présents)
    local tmp
    tmp=$(jq --arg side "$side" \
             --arg res      "${res:-null}" \
             --arg fps      "${fps:-null}" \
             --arg quality  "${quality:-null}" \
             --arg urgDelay "${urg_delay:-null}" \
          '
          if $res      != "null" then .[$side].res       = ($res|tonumber? // $res)       else . end |
          if $fps      != "null" then .[$side].fps       = ($fps|tonumber)                else . end |
          if $quality  != "null" then .[$side].quality   = ($quality|tonumber)            else . end |
          if $urgDelay != "null" then .[$side].urgDelay  = ($urgDelay|tonumber)           else . end
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
# $1 = propriété, $2 = valeur, $3 = instance (optionnel)
# =============================================================================

PROP="$1"
VAL="$2"
SIDE="${3:-rear}"    # Instance par défaut : rear

# Validation instance
if [[ "$SIDE" != "front" && "$SIDE" != "rear" && "$SIDE" != "*" ]]; then
    log_cam "ERREUR : instance inconnue '$SIDE' (attendu : front|rear)"
    exit 1
fi

case "$PROP" in

    # -----------------------------------------------------------------------
    # mode : commande principale (off | stream | snapshot)
    # -----------------------------------------------------------------------
    "mode")
        case "$VAL" in
            "off")
                stop_webcam "$SIDE"
                ;;
            "stream"|"snapshot")
                start_webcam "$SIDE" "$VAL" &
                ;;
            *)
                log_cam "ERREUR : mode '$VAL' inconnu (attendu : off|stream|snapshot)"
                send_vpiv "\$I:COM:error:SE:\"CamSE ${SIDE} : mode inconnu '${VAL}'\"#"
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
    # (snap) : capture snapshot immédiate — répond via StreamURL
    # -----------------------------------------------------------------------
    "(snap)")
        take_snapshot "$SIDE"
        ;;

    # -----------------------------------------------------------------------
    # Propriété inconnue
    # -----------------------------------------------------------------------
    *)
        log_cam "Propriété inconnue ou non supportée en v1 : '$PROP'"
        send_vpiv "\$I:COM:error:SE:\"CamSE : propriété inconnue '${PROP}'\"#"
        exit 1
        ;;
esac