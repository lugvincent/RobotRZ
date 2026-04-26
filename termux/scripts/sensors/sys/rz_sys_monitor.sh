#!/bin/bash
# =============================================================================
# SCRIPT  : rz_sys_monitor.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/sys/rz_sys_monitor.sh
#
# OBJECTIF
# --------
# Surveillance matérielle du smartphone embarqué (SE) :
#   CPU, Température, Stockage, Mémoire, Batterie SE.
# Publie les données vers SP via FIFO → VPIV → MQTT.
# Déclenche alertes (COM:warn) et urgences (Urg:source) si seuils franchis.
# Met à jour global.json pour synchronisation inter-modules SE.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Charge sys_runtime.json → envoie ACK modeSys et paraSys à SP.
# 2. Envoie Sys.uptime une seule fois au démarrage (valeur ponctuelle).
# 3. Boucle selon le mode :
#       OFF    : aucune surveillance, script terminé normalement.
#       FLOW   : publie dataCont* à chaque cycle (période freqCont secondes).
#       MOY    : accumule nbCycles mesures, publie dataMoy* (période freqMoy secondes).
#       ALERTE : aucun flux données, surveillance seuils uniquement.
# 4. Chaque cycle : vérifie les seuils via alerts_sys.sh (règle 10s warn+urg).
# 5. En cas d'urgence : arrêt des émissions, attente commandes SP (clear ou reset).
#
# TABLE A — VPIV PRODUITS (version finale stabilisée)
# ----------------------------------------------------
# SE → SP :
#   modeSys         I   ACK mode appliqué
#   paraSys         I   ACK paramètres appliqués (format sérialisé)
#   uptime          I   Envoyé 1 fois au démarrage (secondes depuis boot)
#   dataContCpu     F   % CPU, mode FLOW, période freqCont
#   dataMoyCpu      F   % CPU moyenné nbCycles, mode MOY, période freqMoy
#   dataContThermal F   °C, mode FLOW
#   dataMoyThermal  F   °C moyenné, mode MOY
#   dataContStorage F   % stockage SDCard, mode FLOW
#   dataMoyStorage  F   % stockage moyenné, mode MOY
#   dataContMem     F   % RAM, mode FLOW
#   dataMoyMem      F   % RAM moyenné, mode MOY
#   dataContBatt    F   % batterie SE, mode FLOW
#   dataMoyBatt     F   % batterie SE moyenné, mode MOY
#
# SP → SE (lus dans sys_runtime.json) :
#   modeSys  enum   OFF|FLOW|MOY|ALERTE
#   paraSys  object freqCont(s), freqMoy(s), nbCycles,
#                   thresholds:{cpu,thermal,storage,mem}:{warn,urg}
#                              {batt}:{warn,urg}  ← sens inversé (alerte si < seuil)
#
# MÉCANISME ALERTES (via alerts_sys.sh)
# --------------------------------------
# Règle 10s sur DEUX seuils par métrique :
#   seuil_warn >= 10s → $I:COM:warn:SE:"message"#  (SP affiche, pas d'arrêt)
#   seuil_urg  >= 10s → $E:Urg:source:SE:URG_...#  (SE arrête émissions, attend SP)
#
# Sens d'alerte :
#   cpu, thermal, storage, mem : alerte si valeur TROP HAUTE (> seuil)
#   batt                       : alerte si valeur TROP BASSE  (< seuil)
#
# CODES URG :
#   URG_CPU_CHARGE  (cpu)
#   URG_OVERHEAT    (thermal)
#   URG_SENSOR_FAIL (storage, mem)
#   URG_LOW_BAT     (batt)
#
# REPRISE APRÈS URGENCE
# ----------------------
# SE attend un nouveau modeSys de SP. Pas de reprise automatique.
# SP envoie clear via CfgS:emg puis un nouveau $V:Sys:modeSys:*:<mode>#.
#
# ARTICULATION AVEC LES AUTRES SCRIPTS
# --------------------------------------
#   check_config.sh      : génère sys_runtime.json avant démarrage
#   courant_init.sh      : (dev) définit les paramètres
#   urgence.sh           : sourcé → fournit trigger_urgence()
#   alerts_sys.sh        : sourcé → fournit check_alert() et init_alert_trackers()
#   rz_vpiv_parser.sh    : lit fifo_termux_in, publie sur MQTT
#   rz_state-manager.sh  : lit global.json (.Sys.cpu, .Sys.thermal...)
#   rz_stt_handler.sh    : lit global.json (.Sys.cpu)
#
# PRÉCAUTIONS
# -----------
# - Noms champs global.json conformes Table A : cpu, thermal, storage, mem, batt
# - freqCont/freqMoy en SECONDES (pas Hz comme Gyro/Mag) : plus petit = plus fréquent.
# - termux-battery-status nécessite le package termux-api.
# - get_thermal() : termux-sensor en premier, fallback sysfs thermal_zone0.
#
# DÉPENDANCES
# -----------
#   jq, awk, top, df  : disponibles dans Termux
#   termux-sensor     : optionnel pour la température
#   termux-battery-status : pour la batterie (pkg install termux-api)
#   fifo_termux_in    : créé par fifo_manager.sh
#   urgence.sh, alerts_sys.sh : dans lib/
#
# AUTEUR  : Vincent Philippe
# VERSION : 4.0  (ajout métrique batt, sens alerte inversé)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

RUNTIME_JSON="$BASE_DIR/config/sys_runtime.json"
GLOBAL_FILE="$BASE_DIR/config/global.json"
LOG_FILE="$BASE_DIR/logs/sys_monitor.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"
LIB_DIR="$BASE_DIR/scripts/sensors/sys/lib"

# =============================================================================
# IMPORT BIBLIOTHÈQUES
# =============================================================================

# shellcheck source=/dev/null
source "$LIB_DIR/urgence.sh"
# shellcheck source=/dev/null
source "$LIB_DIR/alerts_sys.sh"

# Déclarations préventives pour ShellCheck :
# Ces tableaux sont définis dans alerts_sys.sh (sourcé ci-dessus).
# ShellCheck ne suit pas les fichiers sourcés → fausse alerte "referenced but not assigned".
# La déclaration ici est purement indicative, elle sera écrasée par init_alert_trackers().
declare -A alert_urg_active=()
declare -A alert_warn_active=()
declare -A alert_urg_since=()
declare -A alert_warn_since=()
export alert_urg_active alert_warn_active alert_urg_since alert_warn_since

# =============================================================================
# VALEURS PAR DÉFAUT (écrasées par load_config)
# =============================================================================

modeSys="FLOW"
freqCont=10        # Période flux continu en secondes (freqCont < freqMoy = plus fréquent)
freqMoy=60         # Période flux moyenné en secondes
nbCycles=3         # Nombre de mesures pour la moyenne (mode MOY)

# Seuils warn → COM:warn après 10s (sens haut sauf batt)
seuil_warn_cpu=80
seuil_warn_thermal=70
seuil_warn_storage=85
seuil_warn_mem=80
seuil_warn_batt=30      # Batterie : warn si SOUS 30%

# Seuils urg → $E:Urg:source après 10s
seuil_urg_cpu=90
seuil_urg_thermal=85
seuil_urg_storage=95
seuil_urg_mem=90
seuil_urg_batt=15       # Batterie : urgence si SOUS 15%

# Drapeau urgence active (SE arrête ses émissions)
urgence_active=false

# =============================================================================
# INITIALISATION LOG
# =============================================================================

mkdir -p "$(dirname "$LOG_FILE")"
touch "$LOG_FILE"

log_sys() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [SYS] $1" | tee -a "$LOG_FILE"
}

# =============================================================================
# ENVOI VPIV VIA FIFO (non-bloquant, watchdog 2s)
# =============================================================================

send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_IN" ]; then
        log_sys "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_IN" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null
}

# =============================================================================
# CHARGEMENT CONFIGURATION
# =============================================================================

load_config() {
    if [ ! -f "$RUNTIME_JSON" ]; then
        log_sys "WARN : sys_runtime.json absent. Valeurs par défaut utilisées."
        return 1
    fi

    modeSys=$(jq -r            '.sys.modeSys // "FLOW"'                          "$RUNTIME_JSON")
    freqCont=$(jq -r           '.sys.paraSys.freqCont // 10'                     "$RUNTIME_JSON")
    freqMoy=$(jq -r            '.sys.paraSys.freqMoy // 60'                      "$RUNTIME_JSON")
    nbCycles=$(jq -r           '.sys.paraSys.nbCycles // 3'                      "$RUNTIME_JSON")

    seuil_warn_cpu=$(jq -r     '.sys.paraSys.thresholds.cpu.warn // 80'          "$RUNTIME_JSON")
    seuil_urg_cpu=$(jq -r      '.sys.paraSys.thresholds.cpu.urg // 90'           "$RUNTIME_JSON")
    seuil_warn_thermal=$(jq -r '.sys.paraSys.thresholds.thermal.warn // 70'      "$RUNTIME_JSON")
    seuil_urg_thermal=$(jq -r  '.sys.paraSys.thresholds.thermal.urg // 85'       "$RUNTIME_JSON")
    seuil_warn_storage=$(jq -r '.sys.paraSys.thresholds.storage.warn // 85'      "$RUNTIME_JSON")
    seuil_urg_storage=$(jq -r  '.sys.paraSys.thresholds.storage.urg // 95'       "$RUNTIME_JSON")
    seuil_warn_mem=$(jq -r     '.sys.paraSys.thresholds.mem.warn // 80'          "$RUNTIME_JSON")
    seuil_urg_mem=$(jq -r      '.sys.paraSys.thresholds.mem.urg // 90'           "$RUNTIME_JSON")
    seuil_warn_batt=$(jq -r    '.sys.paraSys.thresholds.batt.warn // 30'         "$RUNTIME_JSON")
    seuil_urg_batt=$(jq -r     '.sys.paraSys.thresholds.batt.urg // 15'          "$RUNTIME_JSON")

    log_sys "Config : mode=$modeSys freqCont=${freqCont}s freqMoy=${freqMoy}s nbCycles=$nbCycles"
    log_sys "  cpu[${seuil_warn_cpu}/${seuil_urg_cpu}] thermal[${seuil_warn_thermal}/${seuil_urg_thermal}]"
    log_sys "  storage[${seuil_warn_storage}/${seuil_urg_storage}] mem[${seuil_warn_mem}/${seuil_urg_mem}]"
    log_sys "  batt[warn<${seuil_warn_batt}% urg<${seuil_urg_batt}%]  ← sens inversé"

    # ACK modeSys → SP
    send_vpiv "\$I:Sys:modeSys:SE:${modeSys}#"

    # ACK paraSys complet → SP (format sérialisé Table A)
    local ack
    ack="{\"freqCont\":${freqCont},\"freqMoy\":${freqMoy},\"nbCycles\":${nbCycles},"
    ack+="\"thresholds\":{"
    ack+="\"cpu\":{\"warn\":${seuil_warn_cpu},\"urg\":${seuil_urg_cpu}},"
    ack+="\"thermal\":{\"warn\":${seuil_warn_thermal},\"urg\":${seuil_urg_thermal}},"
    ack+="\"storage\":{\"warn\":${seuil_warn_storage},\"urg\":${seuil_urg_storage}},"
    ack+="\"mem\":{\"warn\":${seuil_warn_mem},\"urg\":${seuil_urg_mem}},"
    ack+="\"batt\":{\"warn\":${seuil_warn_batt},\"urg\":${seuil_urg_batt}}}}"
    send_vpiv "\$I:Sys:paraSys:SE:${ack}#"
}

# =============================================================================
# UPTIME — UNE SEULE FOIS AU DÉMARRAGE (valeur ponctuelle, Table A)
# =============================================================================

send_uptime() {
    local uptime
    uptime=$(awk '{printf "%d", $1}' /proc/uptime 2>/dev/null || echo "0")
    send_vpiv "\$I:Sys:uptime:SE:${uptime}#"
    log_sys "Uptime envoyé : ${uptime}s"
}

# =============================================================================
# FONCTIONS D'ACQUISITION
# =============================================================================

get_cpu() {
    # Charge CPU en % (entier) via top
    # Format Android Termux : "800%cpu 0%user ... 800%idle"
    # Formule : 100 - (idle_total / cpu_total × 100)
    local result
    result=$(top -bn1 2>/dev/null | awk '/%cpu/ {
        for(i=1;i<=NF;i++) {
            if($i~/[0-9]%idle/) {
                idle=$i; gsub(/%idle/,"",idle)
                cpu_tot=$1; gsub(/%cpu/,"",cpu_tot)
                printf "%d", 100-(idle/cpu_tot*100)
                exit
            }
        }
    }' 2>/dev/null)
    # Fallback si top ne retourne rien
    [ -z "$result" ] && result=0
    echo "$result"
}

get_thermal() {
    # Température en °C (entier) — termux-sensor puis fallback sysfs
    local temp=""
    if command -v termux-sensor &>/dev/null; then
        temp=$(termux-sensor -s "Temperature" -n 1 2>/dev/null \
               | jq -r '.values[0] // empty' 2>/dev/null)
        temp="${temp%.*}"   # Partie entière uniquement
    fi
    if [ -z "$temp" ] || [ "$temp" = "0" ]; then
        local raw
        raw=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)
        [ -n "$raw" ] && temp=$((raw / 1000)) || temp=0
    fi
    echo "${temp:-0}"
}

get_storage() {
    # Occupation partition SDCard en % (entier)
    df /sdcard 2>/dev/null | awk 'NR==2 {gsub(/%/,""); print $5}' || echo "0"
}

get_mem() {
    # Pourcentage mémoire utilisée via /proc/meminfo
    awk '/MemTotal/    {total=$2}
         /MemAvailable/{avail=$2}
         END { if(total>0) printf "%d", (total-avail)*100/total; else print "0" }
        ' /proc/meminfo 2>/dev/null || echo "0"
}

get_batt() {
    # Niveau batterie SE en % via termux-battery-status (package termux-api requis)
    if command -v termux-battery-status &>/dev/null; then
        termux-battery-status 2>/dev/null | jq -r '.percentage // 0' 2>/dev/null || echo "0"
    else
        log_sys "WARN : termux-battery-status indisponible (pkg install termux-api)"
        echo "0"
    fi
}

# =============================================================================
# MISE À JOUR GLOBAL.JSON (noms conformes Table A)
# =============================================================================

update_global() {
    local cpu=$1 thermal=$2 storage=$3 mem=$4 batt=$5
    local date_now
    date_now=$(date '+%Y-%m-%d %H:%M:%S')

    if [ -f "$GLOBAL_FILE" ] && command -v jq &>/dev/null; then
        jq --argjson cpu     "$cpu"     \
           --argjson thermal "$thermal" \
           --argjson storage "$storage" \
           --argjson mem     "$mem"     \
           --argjson batt    "$batt"    \
           --arg     date    "$date_now" \
           '.Sys.cpu=$cpu | .Sys.thermal=$thermal | .Sys.storage=$storage | .Sys.mem=$mem | .Sys.batt=$batt | .Sys.last_update=$date' \
           "$GLOBAL_FILE" > "${GLOBAL_FILE}.tmp" \
        && mv "${GLOBAL_FILE}.tmp" "$GLOBAL_FILE"
    fi
}

# =============================================================================
# UN CYCLE COMPLET : acquisition + global.json + alertes
# Résultats dans variables globales : _cpu _thermal _storage _mem _batt
# =============================================================================

run_cycle() {
    _cpu=$(get_cpu)
    _thermal=$(get_thermal)
    _storage=$(get_storage)
    _mem=$(get_mem)
    _batt=$(get_batt)

    # Mise à jour global.json pour les autres modules SE
    update_global "$_cpu" "$_thermal" "$_storage" "$_mem" "$_batt"

    # Vérification seuils — sens "haut" pour cpu/thermal/storage/mem, "bas" pour batt
    check_alert "cpu"     "$_cpu"     "$seuil_warn_cpu"     "$seuil_urg_cpu"     "URG_CPU_CHARGE"  "%" "haut"
    check_alert "thermal" "$_thermal" "$seuil_warn_thermal" "$seuil_urg_thermal" "URG_OVERHEAT"    "°C" "haut"
    check_alert "storage" "$_storage" "$seuil_warn_storage" "$seuil_urg_storage" "URG_SENSOR_FAIL" "%" "haut"
    check_alert "mem"     "$_mem"     "$seuil_warn_mem"     "$seuil_urg_mem"     "URG_SENSOR_FAIL" "%" "haut"
    check_alert "batt"    "$_batt"    "$seuil_warn_batt"    "$seuil_urg_batt"    "URG_LOW_BAT"     "%" "bas"

    # Détection urgence déclenchée → arrêt émissions
    if [[ "${alert_urg_active[cpu]}"     == true || \
          "${alert_urg_active[thermal]}" == true || \
          "${alert_urg_active[storage]}" == true || \
          "${alert_urg_active[mem]}"     == true || \
          "${alert_urg_active[batt]}"    == true ]]; then
        urgence_active=true
    fi
}

# =============================================================================
# ATTENTE COMMANDES SP APRÈS URGENCE
# SE arrête émissions, continue la surveillance passive (global.json).
# Reprise uniquement sur réception d'un nouveau $V:Sys:modeSys:*:<mode>#
# (traité par rz_vpiv_parser.sh qui redémarre ce script).
# =============================================================================

attente_urgence() {
    log_sys "URGENCE ACTIVE : émissions arrêtées. Attente modeSys de SP..."
    while true; do
        _cpu=$(get_cpu)
        _thermal=$(get_thermal)
        _storage=$(get_storage)
        _mem=$(get_mem)
        _batt=$(get_batt)
        update_global "$_cpu" "$_thermal" "$_storage" "$_mem" "$_batt"
        sleep "$freqCont"
    done
}

# =============================================================================
# MODE FLOW — Publication dataCont* à freqCont secondes
# =============================================================================

mode_flow() {
    log_sys "Mode FLOW démarré (freqCont=${freqCont}s)"

    while true; do
        run_cycle
        [[ "$urgence_active" == true ]] && attente_urgence

        send_vpiv "\$F:Sys:dataContCpu:SE:${_cpu}#"
        send_vpiv "\$F:Sys:dataContThermal:SE:${_thermal}#"
        send_vpiv "\$F:Sys:dataContStorage:SE:${_storage}#"
        send_vpiv "\$F:Sys:dataContMem:SE:${_mem}#"
        send_vpiv "\$F:Sys:dataContBatt:SE:${_batt}#"

        sleep "$freqCont"
    done
}

# =============================================================================
# MODE MOY — Publication dataMoy* à freqMoy secondes (moyenne sur nbCycles)
# =============================================================================

mode_moy() {
    log_sys "Mode MOY démarré (freqMoy=${freqMoy}s, nbCycles=$nbCycles)"

    local sum_cpu=0 sum_thermal=0 sum_storage=0 sum_mem=0 sum_batt=0
    local count=0

    while true; do
        run_cycle
        [[ "$urgence_active" == true ]] && attente_urgence

        sum_cpu=$((     sum_cpu     + _cpu     ))
        sum_thermal=$(( sum_thermal + _thermal ))
        sum_storage=$(( sum_storage + _storage ))
        sum_mem=$((     sum_mem     + _mem     ))
        sum_batt=$((    sum_batt    + _batt    ))
        count=$(( count + 1 ))

        if (( count >= nbCycles )); then
            send_vpiv "\$F:Sys:dataMoyCpu:SE:$((     sum_cpu     / nbCycles ))#"
            send_vpiv "\$F:Sys:dataMoyThermal:SE:$(( sum_thermal / nbCycles ))#"
            send_vpiv "\$F:Sys:dataMoyStorage:SE:$(( sum_storage / nbCycles ))#"
            send_vpiv "\$F:Sys:dataMoyMem:SE:$((     sum_mem     / nbCycles ))#"
            send_vpiv "\$F:Sys:dataMoyBatt:SE:$((    sum_batt    / nbCycles ))#"
            # Réinitialisation accumulateurs
            sum_cpu=0; sum_thermal=0; sum_storage=0; sum_mem=0; sum_batt=0; count=0
        fi

        sleep "$freqMoy"
    done
}

# =============================================================================
# MODE ALERTE — Surveillance seuils uniquement, aucun flux données
# =============================================================================

mode_alerte() {
    log_sys "Mode ALERTE démarré (freqCont=${freqCont}s — surveillance seuils uniquement)"

    while true; do
        run_cycle
        [[ "$urgence_active" == true ]] && attente_urgence
        sleep "$freqCont"
    done
}

# =============================================================================
# MAIN
# =============================================================================

main() {
    log_sys "=========================================="
    log_sys "Démarrage rz_sys_monitor.sh  v4.0"
    log_sys "=========================================="

    # Vérifications dépendances
    for tool in jq awk; do
        if ! command -v "$tool" &>/dev/null; then
            log_sys "ERREUR CRITIQUE : $tool non installé. Arrêt."
            exit 1
        fi
    done

    if [ ! -p "$FIFO_IN" ]; then
        log_sys "WARN : FIFO $FIFO_IN absente. Lancer fifo_manager.sh create."
    fi

    # Initialisation global.json si absent
    if [ ! -f "$GLOBAL_FILE" ]; then
        log_sys "WARN : global.json absent → création structure de base"
        mkdir -p "$(dirname "$GLOBAL_FILE")"
        cat > "$GLOBAL_FILE" <<'EOF'
{
  "Sys":  { "cpu": 0, "thermal": 0, "storage": 0, "mem": 0, "batt": 0, "last_update": "" },
  "CfgS": { "modeRZ": 1, "typePtge": 0, "reset": false },
  "Urg":  { "etat": "clear", "source": "NONE" },
  "COM":  { "info": "", "warn": "", "error": "", "debug": "" }
}
EOF
    fi

    # Initialisation trackers alertes (alerts_sys.sh) — 5 métriques dont batt
    init_alert_trackers

    # Chargement config + envoi ACK
    load_config

    # Uptime : valeur ponctuelle au démarrage uniquement
    send_uptime

    # Aiguillage mode
    log_sys "Lancement mode : $modeSys"
    case "$modeSys" in
        OFF)
            log_sys "Mode OFF : aucune surveillance. Script terminé normalement."
            send_vpiv "\$I:COM:info:SE:\"Sys monitor OFF\"#"
            ;;
        FLOW)
            mode_flow
            ;;
        MOY)
            mode_moy
            ;;
        ALERTE)
            mode_alerte
            ;;
        *)
            log_sys "ERREUR : modeSys inconnu = '$modeSys'"
            send_vpiv "\$I:COM:error:SE:\"modeSys inconnu : $modeSys\"#"
            exit 1
            ;;
    esac
}

main