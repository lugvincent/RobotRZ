#!/bin/bash
# =============================================================================
# SCRIPT  : rz_appli_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/appli/rz_appli_manager.sh
#
# OBJECTIF
# --------
# Daemon de gestion des applications Android pilotées par SP via VPIV CAT=A.
# Maintient l'état de chaque app, vérifie les conflits avant lancement,
# dispatche vers Tasker (via rz_tasker_bridge.sh) ou am start direct,
# et retourne un ACK VPIV structuré vers SP.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Lit en continu le FIFO fifo_appli_in.
# Chaque trame VPIV reçue est parsée, validée, puis traitée :
#   1. check_conflits()  : vérifie état système avant lancement
#   2. lancer_app()      : dispatche vers bridge Tasker ou am start
#   3. ACK VPIV          : $I:Appli:<prop>:*:OK# ou FAIL
#
# APPS GÉRÉES (v1)
# ----------------
#   Baby        : App Baby → Tasker (RZ_Baby)
#   tasker      : Tasker lui-même → am start direct
#   zoom        : Zoom → Tasker (RZ_Zoom) + am start fallback
#   BabyMonitor : Baby Monitor → Tasker (RZ_BabyMonitor)
#   NavGPS      : Navigation GPS → Tasker (RZ_NavGPS)
#
# CONFLITS VÉRIFIÉS (check_conflits)
# ------------------------------------
# warn (envoi COM:warn, lancement quand même) :
#   - Caméra déjà active (cam_config.json)  → Zoom, BabyMonitor
#   - Micro coupé (modeMicro=off)           → Baby, BabyMonitor
#   - Robot en déplacement (modeRZ=3)       → Zoom, NavGPS, BabyMonitor
#
# bloquant (envoi COM:error, ACK FAIL, abandon) :
#   - Tasker OFF                            → Baby, BabyMonitor, NavGPS
#   - Urgence active (Urg.statut=active)    → toutes apps
#   - CPU > seuil urg                       → Zoom, BabyMonitor
#
# TABLE A — VPIV TRAITÉS
# ----------------------
# SP → SE (reçus via FIFO) :
#   $A:Appli:Baby:*:On#
#   $A:Appli:tasker:*:On#
#   $A:Appli:zoom:*:On#
#   $A:Appli:BabyMonitor:*:On#
#   $A:Appli:NavGPS:*:On#
#   (idem avec Off pour arrêt)
#   $A:Appli:ExprTasker:Expression:sourire#
#   $A:Appli:ExprTasker:Info:Bonjour je suis RZ#
#   $A:Appli:ExprTasker:Off:*#
#
# SE → SP (via FIFO_OUT) :
#   $I:Appli:<prop>:*:OK#       ACK succès (apps On/Off)
#   $I:Appli:ExprTasker:<inst>:OK#   ACK ExprTasker (inst = Expression|Info|Off)
#   $I:Appli:<prop>:*:FAIL#     ACK échec (conflit bloquant ou erreur bridge)
#   $I:COM:warn:SE:"..."#       Avertissement conflit non bloquant
#   $I:COM:error:SE:"..."#      Erreur conflit bloquant ou technique
#
# INTERACTIONS
# ------------
#   Lit        : fifo/fifo_appli_in  (trames VPIV CAT=A)
#   Lit        : config/appli_config.json (état apps + noms tâches Tasker)
#   Lit        : config/global.json (modeRZ, Urg, Sys.cpu)
#   Lit        : config/sensors/cam_config.json (état caméra)
#   Lit        : config/sensors/mic_config.json (modeMicro)
#   Écrit      : config/appli_config.json (mise à jour état)
#   Écrit      : fifo/fifo_termux_in (ACK → rz_vpiv_parser → MQTT → SP)
#   Appelle    : utils/rz_tasker_bridge.sh (tâches Tasker)
#
# PRÉCAUTIONS
# -----------
# - fifo_appli_in doit être créé par fifo_manager.sh (ajouter à la liste).
# - rz_tasker_bridge.sh doit être dans le répertoire utils/.
# - appli_config.json doit exister (généré par check_config.sh).
# - ⚠️ am start nécessite Android (Termux sur smartphone uniquement).
# - Le manager ne relaie pas les VPIV Arduino (CAT=A uniquement pour Appli).
#
# DÉPENDANCES
# -----------
#   jq, am (Android Manager), rz_tasker_bridge.sh, fifo_appli_in
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.2  (ExprTasker : VPIV standard INST=modeExpr, pas de JSON en VALUE)
# DATE    : 2026-03-05
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
fi

APPLI_CONFIG="$BASE_DIR/config/appli_config.json"
GLOBAL_JSON="$BASE_DIR/config/global.json"
CAM_CONFIG="$BASE_DIR/config/sensors/cam_config.json"
MIC_CONFIG="$BASE_DIR/config/sensors/mic_config.json"
SYS_CONFIG="$BASE_DIR/config/sensors/sys_config.json"
FIFO_IN="$BASE_DIR/fifo/fifo_appli_in"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/appli_manager.log"
BRIDGE="$BASE_DIR/utils/rz_tasker_bridge.sh"

# Timeout bridge Tasker (secondes)
BRIDGE_TIMEOUT=5

# =============================================================================
# UTILITAIRES
# =============================================================================

log_app() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [APPLI] $1" >> "$LOG_FILE"
}

# Envoi VPIV via FIFO (non-bloquant, watchdog 2s)
send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_OUT" ]; then
        log_app "WARN : FIFO_OUT absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_OUT" &
    local pid_w=$!
    sleep 2 && kill "$pid_w" 2>/dev/null &
    wait "$pid_w" 2>/dev/null
}

# Lecture valeur JSON avec défaut
lire_json() {
    local query="$1"
    local defaut="$2"
    jq -r "${query} // \"${defaut}\"" "$GLOBAL_JSON" 2>/dev/null || echo "$defaut"
}

# Mise à jour état app dans appli_config.json
update_state() {
    local prop="$1"
    local state="$2"
    local now
    now=$(date '+%Y-%m-%d %H:%M:%S')
    jq --arg prop "$prop" --arg state "$state" --arg now "$now" \
       '.appli[$prop].state = $state | .appli[$prop].last_change = $now' \
       "$APPLI_CONFIG" > "${APPLI_CONFIG}.tmp" \
    && mv "${APPLI_CONFIG}.tmp" "$APPLI_CONFIG"
}

# =============================================================================
# CHECK_CONFLITS
# Vérifie les conflits avant lancement d'une app.
# $1 = nom propriété (Baby | tasker | zoom | BabyMonitor | NavGPS)
# $2 = valeur demandée (On | Off)
# Retourne : 0 = OK (lancement autorisé)
#            1 = BLOQUANT (lancement interdit)
# Les warn sont envoyés mais ne bloquent pas.
# =============================================================================

check_conflits() {
    local prop="$1"
    local val="$2"

    # Arrêt (Off) : pas de vérification de conflits
    if [ "$val" == "Off" ]; then
        return 0
    fi

    # ------------------------------------------------------------------
    # CONFLIT BLOQUANT 1 : Urgence active → toutes les apps
    # ------------------------------------------------------------------
    local urg_statut
    urg_statut=$(jq -r '.Urg.statut // "cleared"' "$GLOBAL_JSON" 2>/dev/null)
    if [ "$urg_statut" == "active" ]; then
        send_vpiv "\$I:COM:error:SE:\"Appli:${prop} : lancement refusé (urgence active)\"#"
        log_app "BLOQUANT [$prop] : urgence active"
        return 1
    fi

    # ------------------------------------------------------------------
    # CONFLIT BLOQUANT 2 : CPU surcharge → Zoom, BabyMonitor
    # ------------------------------------------------------------------
    if [[ "$prop" == "zoom" || "$prop" == "BabyMonitor" ]]; then
        local cpu_val
        cpu_val=$(jq -r '.Sys.cpu // 0' "$GLOBAL_JSON" 2>/dev/null | cut -d. -f1)
        # Lecture seuil urg CPU depuis sys_config.json
        local cpu_urg
        cpu_urg=$(jq -r '.thresholds.cpu.urg // 90' "$SYS_CONFIG" 2>/dev/null)
        if (( cpu_val >= cpu_urg )); then
            send_vpiv "\$I:COM:error:SE:\"Appli:${prop} : lancement refusé (CPU ${cpu_val}% >= seuil ${cpu_urg}%)\"#"
            log_app "BLOQUANT [$prop] : CPU ${cpu_val}% >= ${cpu_urg}%"
            return 1
        fi
    fi

    # ------------------------------------------------------------------
    # CONFLIT BLOQUANT 3 : Tasker OFF → Baby, BabyMonitor, NavGPS
    # ------------------------------------------------------------------
    if [[ "$prop" == "Baby" || "$prop" == "BabyMonitor" || "$prop" == "NavGPS" ]]; then
        local tasker_state
        tasker_state=$(jq -r '.appli.tasker.state // "Off"' "$APPLI_CONFIG" 2>/dev/null)
        if [ "$tasker_state" == "Off" ]; then
            send_vpiv "\$I:COM:error:SE:\"Appli:${prop} : lancement refusé (Tasker OFF — lancer Tasker d'abord)\"#"
            log_app "BLOQUANT [$prop] : Tasker est OFF"
            return 1
        fi
    fi

    # ------------------------------------------------------------------
    # CONFLIT WARN 1 : Caméra déjà active → Zoom, BabyMonitor
    # ------------------------------------------------------------------
    if [[ "$prop" == "zoom" || "$prop" == "BabyMonitor" ]]; then
        if [ -f "$CAM_CONFIG" ]; then
            local cam_rear cam_front
            cam_rear=$( jq -r '.rear.mode  // "off"' "$CAM_CONFIG" 2>/dev/null)
            cam_front=$(jq -r '.front.mode // "off"' "$CAM_CONFIG" 2>/dev/null)
            if [[ "$cam_rear" != "off" || "$cam_front" != "off" ]]; then
                send_vpiv "\$I:COM:warn:SE:\"Appli:${prop} : caméra déjà active (mode=${cam_rear}/${cam_front})\"#"
                log_app "WARN [$prop] : caméra déjà active"
                # warn : pas de return 1, on continue
            fi
        fi
    fi

    # ------------------------------------------------------------------
    # CONFLIT WARN 2 : Micro coupé → Baby, BabyMonitor
    # ------------------------------------------------------------------
    if [[ "$prop" == "Baby" || "$prop" == "BabyMonitor" ]]; then
        if [ -f "$MIC_CONFIG" ]; then
            local mode_micro
            mode_micro=$(jq -r '.mic.modeMicro // "off"' "$MIC_CONFIG" 2>/dev/null)
            if [ "$mode_micro" == "off" ]; then
                send_vpiv "\$I:COM:warn:SE:\"Appli:${prop} : microphone coupé (modeMicro=off)\"#"
                log_app "WARN [$prop] : micro coupé"
            fi
        fi
    fi

    # ------------------------------------------------------------------
    # CONFLIT WARN 3 : Robot en déplacement → Zoom, NavGPS, BabyMonitor
    # ------------------------------------------------------------------
    if [[ "$prop" == "zoom" || "$prop" == "NavGPS" || "$prop" == "BabyMonitor" ]]; then
        local mode_rz
        mode_rz=$(jq -r '.CfgS.modeRZ // 1' "$GLOBAL_JSON" 2>/dev/null)
        if [ "$mode_rz" == "3" ]; then
            send_vpiv "\$I:COM:warn:SE:\"Appli:${prop} : robot en déplacement (modeRZ=3)\"#"
            log_app "WARN [$prop] : robot en déplacement"
        fi
    fi

    return 0
}

# =============================================================================
# LANCER_APP
# Dispatche vers Tasker (bridge) ou am start direct selon l'app.
# $1 = prop (nom app)
# $2 = val (On|Off)
# Retourne : 0 = succès, 1 = échec
# =============================================================================

lancer_app() {
    local prop="$1"
    local val="$2"
    local tasker_task
    local package
    local result=0

    # Lecture paramètres depuis appli_config.json
    tasker_task=$(jq -r ".appli.${prop}.tasker_task // \"\"" "$APPLI_CONFIG" 2>/dev/null)
    package=$(    jq -r ".appli.${prop}.package     // \"\"" "$APPLI_CONFIG" 2>/dev/null)

    log_app "Lancement : $prop → $val (task=$tasker_task pkg=$package)"

    # ------------------------------------------------------------------
    # CAS TASKER LUI-MÊME : am start direct (pas de bridge récursif)
    # ------------------------------------------------------------------
    if [ "$prop" == "tasker" ]; then
        if [ "$val" == "On" ]; then
            am start -n "${package}/net.dinglisch.android.taskerm.TaskerAppCompatActivity" \
                > /dev/null 2>&1
            result=$?
        else
            am force-stop "$package" > /dev/null 2>&1
            result=$?
        fi
        return $result
    fi

    # ------------------------------------------------------------------
    # AUTRES APPS : via rz_tasker_bridge.sh
    # ------------------------------------------------------------------
    if [ -z "$tasker_task" ]; then
        log_app "ERREUR : pas de tasker_task défini pour '$prop'"
        send_vpiv "\$I:COM:error:SE:\"Appli:${prop} : tâche Tasker non configurée\"#"
        return 1
    fi

    if [ ! -f "$BRIDGE" ]; then
        log_app "ERREUR : rz_tasker_bridge.sh introuvable ($BRIDGE)"
        send_vpiv "\$I:COM:error:SE:\"Appli : rz_tasker_bridge.sh introuvable\"#"
        return 1
    fi

    bash "$BRIDGE" "$tasker_task" "$val" "$BRIDGE_TIMEOUT"
    result=$?

    # Fallback am start/force-stop si bridge échoue et package connu
    if [ $result -ne 0 ] && [ -n "$package" ]; then
        log_app "Bridge FAIL → fallback am start pour $prop ($package)"
        if [ "$val" == "On" ]; then
            am start -a android.intent.action.MAIN \
                     -c android.intent.category.LAUNCHER \
                     -n "${package}/.MainActivity" > /dev/null 2>&1 \
            || am start --user 0 -a android.intent.action.VIEW \
                     -n "$package" > /dev/null 2>&1
            result=$?
        else
            am force-stop "$package" > /dev/null 2>&1
            result=$?
        fi
    fi

    return $result
}

# =============================================================================
# PROCESS_EXPR_TASKER
# Traite la propriété ExprTasker au format VPIV standard.
# Format reçu : $A:Appli:ExprTasker:<inst>:<value>#
#   INST  = modeExpr : Expression | Info | Off
#   VALUE = valeur   : nom_expression | texte libre | *
#
# modeExpr (INST) :
#   Expression → tâche RZ_Expression, VALUE = nom expression
#                valeurs valides : neutre|sourire|triste|étonnement|colère
#   Info       → tâche RZ_Info, VALUE = texte libre affiché dans scène Tasker
#   Off        → tâche RZ_Expression("neutre"), force retour état neutre
#
# Retour neutre après Expression : géré par Tasker en interne (délai Tasker).
# SE n'envoie pas de second VPIV neutre.
# ACK : $I:Appli:ExprTasker:<inst>:OK# ou FAIL#
# =============================================================================

EXPRESSIONS_VALIDES="neutre sourire triste étonnement colère"

process_expr_tasker() {
    local mode_expr="$1"   # INST du VPIV : Expression | Info | Off
    local valeur="$2"      # VALUE du VPIV : nom expression ou texte

    # Vérification Tasker actif (bloquant)
    local tasker_state
    tasker_state=$(jq -r '.appli.tasker.state // "Off"' "$APPLI_CONFIG" 2>/dev/null)
    if [ "$tasker_state" == "Off" ]; then
        send_vpiv "\$I:COM:error:SE:\"Appli:ExprTasker : Tasker OFF — lancer Tasker d'abord\"#"
        send_vpiv "\$I:Appli:ExprTasker:${mode_expr:-*}:FAIL#"
        log_app "BLOQUANT [ExprTasker] : Tasker est OFF"
        return
    fi

    # Vérification urgence active (bloquant)
    local urg_statut
    urg_statut=$(jq -r '.Urg.statut // "cleared"' "$GLOBAL_JSON" 2>/dev/null)
    if [ "$urg_statut" == "active" ]; then
        send_vpiv "\$I:COM:error:SE:\"Appli:ExprTasker : refusé (urgence active)\"#"
        send_vpiv "\$I:Appli:ExprTasker:${mode_expr:-*}:FAIL#"
        log_app "BLOQUANT [ExprTasker] : urgence active"
        return
    fi

    log_app "ExprTasker : modeExpr=$mode_expr valeur=$valeur"

    local task_name=""
    local task_param=""

    case "$mode_expr" in

        # ------------------------------------------------------------------
        # MODE Expression : appel RZ_Expression avec le nom de l'expression
        # ------------------------------------------------------------------
        "Expression")
            # Validation valeur
            if ! echo "$EXPRESSIONS_VALIDES" | grep -qw "$valeur"; then
                send_vpiv "\$I:COM:error:SE:\"Appli:ExprTasker : expression inconnue '${valeur}' (valides : ${EXPRESSIONS_VALIDES})\"#"
                send_vpiv "\$I:Appli:ExprTasker:${mode_expr}:FAIL#"
                log_app "ERREUR [ExprTasker] : expression inconnue '$valeur'"
                return
            fi
            task_name="RZ_Expression"
            task_param="$valeur"
            ;;

        # ------------------------------------------------------------------
        # MODE Info : appel RZ_Info avec le texte en paramètre
        # ------------------------------------------------------------------
        "Info")
            if [ -z "$valeur" ]; then
                send_vpiv "\$I:COM:warn:SE:\"Appli:ExprTasker Info : valeur (texte) vide\"#"
                log_app "WARN [ExprTasker] : mode Info avec valeur vide"
            fi
            task_name="RZ_Info"
            task_param="$valeur"
            ;;

        # ------------------------------------------------------------------
        # MODE Off : retour forcé à neutre via RZ_Expression("neutre")
        # ------------------------------------------------------------------
        "Off")
            task_name="RZ_Expression"
            task_param="neutre"
            log_app "ExprTasker Off → retour neutre forcé"
            ;;

        # ------------------------------------------------------------------
        # modeExpr inconnu
        # ------------------------------------------------------------------
        *)
            send_vpiv "\$I:COM:error:SE:\"Appli:ExprTasker : modeExpr inconnu '${mode_expr}' (Off|Expression|Info)\"#"
            send_vpiv "\$I:Appli:ExprTasker:${mode_expr}:FAIL#"
            log_app "ERREUR [ExprTasker] : modeExpr inconnu '$mode_expr'"
            return
            ;;
    esac

    # Appel bridge Tasker
    if [ ! -f "$BRIDGE" ]; then
        send_vpiv "\$I:COM:error:SE:\"Appli : rz_tasker_bridge.sh introuvable\"#"
        send_vpiv "\$I:Appli:ExprTasker:${mode_expr:-*}:FAIL#"
        return
    fi

    if bash "$BRIDGE" "$task_name" "$task_param" "$BRIDGE_TIMEOUT"; then
        send_vpiv "\$I:Appli:ExprTasker:${mode_expr}:OK#"
        log_app "[ExprTasker] $task_name($task_param) : OK"
    else
        send_vpiv "\$I:Appli:ExprTasker:${mode_expr}:FAIL#"
        log_app "[ExprTasker] $task_name($task_param) : FAIL"
    fi
}


# =============================================================================
# PROCESS_COMMAND
# Parse et traite une trame VPIV CAT=A reçue sur fifo_appli_in.
# Format attendu : $A:Appli:<prop>:*:<val>#
# =============================================================================

process_command() {
    local msg="$1"

    # ------------------------------------------------------------------
    # CAS ExprTasker : INST = modeExpr, VALUE = valeur
    # Format VPIV standard : $A:Appli:ExprTasker:<inst>:<value>#
    # Intercepté avant le regex On|Off (VALUE ici n'est pas On|Off).
    # ------------------------------------------------------------------
    if [[ "$msg" =~ ^\$A:Appli:ExprTasker:([^:]+):(.*)#$ ]]; then
        local expr_inst="${BASH_REMATCH[1]}"   # Expression | Info | Off
        local expr_val="${BASH_REMATCH[2]}"    # nom expression ou texte
        process_expr_tasker "$expr_inst" "$expr_val"
        return
    fi

    # Validation format VPIV standard (On|Off)
    if ! [[ "$msg" =~ ^\$A:Appli:([^:]+):\*:(On|Off)#$ ]]; then
        log_app "Trame invalide ou non supportée : '$msg'"
        send_vpiv "\$I:COM:error:SE:\"Appli : trame invalide '${msg}'\"#"
        return
    fi

    local prop="${BASH_REMATCH[1]}"
    local val="${BASH_REMATCH[2]}"

    log_app "Commande reçue : $prop → $val"

    # Validation prop connue
    local props_valides="Baby tasker zoom BabyMonitor NavGPS"
    if ! echo "$props_valides" | grep -qw "$prop"; then
        log_app "Propriété inconnue : '$prop'"
        send_vpiv "\$I:COM:error:SE:\"Appli : propriété inconnue '${prop}'\"#"
        return
    fi

    # Vérification des conflits
    if ! check_conflits "$prop" "$val"; then
        # Conflit bloquant : ACK FAIL
        send_vpiv "\$I:Appli:${prop}:*:FAIL#"
        return
    fi

    # Lancement application
    if lancer_app "$prop" "$val"; then
        # Succès : mise à jour état + ACK OK
        update_state "$prop" "$val"
        send_vpiv "\$I:Appli:${prop}:*:OK#"
        log_app "[$prop] → $val : OK"
    else
        # Échec bridge/am : ACK FAIL
        send_vpiv "\$I:Appli:${prop}:*:FAIL#"
        log_app "[$prop] → $val : FAIL"
    fi
}

# =============================================================================
# MAIN — Daemon lecture FIFO
# =============================================================================

main() {
    log_app "=========================================="
    log_app "Démarrage rz_appli_manager.sh  v1.0"
    log_app "=========================================="

    # Vérifications dépendances
    if ! command -v jq &>/dev/null; then
        log_app "ERREUR CRITIQUE : jq non installé. Arrêt."
        exit 1
    fi

    # Vérification / création FIFO
    if [ ! -p "$FIFO_IN" ]; then
        log_app "WARN : $FIFO_IN absent. Création..."
        mkdir -p "$(dirname "$FIFO_IN")"
        mkfifo "$FIFO_IN"
    fi

    # Vérification appli_config.json
    if [ ! -f "$APPLI_CONFIG" ]; then
        log_app "ERREUR : appli_config.json absent ($APPLI_CONFIG). Arrêt."
        exit 1
    fi

    log_app "En attente de commandes sur $FIFO_IN ..."

    # Boucle daemon
    while true; do
        # Lecture bloquante sur FIFO (attend une trame)
        if read -r msg < "$FIFO_IN"; then
            if [ -n "$msg" ]; then
                process_command "$msg"
            fi
        fi
    done
}

# Nettoyage sur sortie
trap 'log_app "Arrêt du daemon Appli."' EXIT

main