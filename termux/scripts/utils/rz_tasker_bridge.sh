#!/bin/bash
# =============================================================================
# SCRIPT  : rz_tasker_bridge.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/utils/rz_tasker_bridge.sh
#
# OBJECTIF
# --------
# Pont de communication entre les scripts Termux (SE) et Tasker (Android).
# Deux modes de fonctionnement :
#   - Mode direct  : déclenche une tâche Tasker nommée (appel avec arguments)
#   - Mode daemon  : lit fifo_tasker_in en continu et route les trames
#                    CAT=A MODULE=CamSE/TorchSE vers les tâches Tasker
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# MODE DIRECT (appelé par rz_appli_manager.sh ou tout script SE)
#   Encode les paramètres en JSON et écrit trigger.txt.
#   Un profil Tasker (Événement → Fichier modifié) surveille ce fichier
#   et déclenche RZ_Dispatch qui lit le JSON et appelle la tâche concernée.
#
# MODE DAEMON (lancé par rz_start.sh avec --daemon)
#   Lit fifo_tasker_in en continu.
#   Parse les trames VPIV CAT=A MODULE=CamSE ou TorchSE.
#   Mappe vers la tâche Tasker correspondante et écrit trigger.txt.
#
# ARCHITECTURE DU DÉCLENCHEMENT
# ------------------------------
#   [Mode direct]
#   rz_appli_manager.sh
#       → rz_tasker_bridge.sh RZ_CamStart rear
#           → trigger.txt
#               → Profil Tasker (Fichier modifié)
#                   → RZ_Dispatch → RZ_CamStart / RZ_TorchOn / ...
#
#   [Mode daemon]
#   rz_vpiv_parser.sh → fifo_tasker_in
#       → rz_tasker_bridge.sh --daemon (boucle)
#           → trigger.txt → Profil Tasker → RZ_Dispatch → tâche
#
# ARGUMENTS — MODE DIRECT
# -----------------------
#   $1 = NOM_TACHE  : nom exact de la tâche Tasker à déclencher
#                     (ex: RZ_CamStart, RZ_CamStop, RZ_TorchOn, RZ_TorchOff)
#   $2 = PARAMETRE  : paramètre optionnel transmis à la tâche
#                     (ex: rear, front, On, Off)
#
# ARGUMENTS — MODE DAEMON
# -----------------------
#   $1 = --daemon   : lance la boucle de lecture fifo_tasker_in
#
# FORMAT DU TRIGGER
# -----------------
# Le fichier trigger.txt contient un JSON sur une seule ligne (sans \n) :
#   {"task":"RZ_CamStart","param":"rear","ts":"1234567890"}
#
# Le champ "ts" (timestamp) garantit que le fichier est toujours modifié,
# même si la tâche et le paramètre sont identiques à l'appel précédent.
# Sans ce champ, Tasker ne détecterait pas de changement et ne se déclencherait pas.
#
# TABLE DE ROUTAGE — MODE DAEMON (fifo_tasker_in)
# ------------------------------------------------
#   $A:CamSE:modeCam:rear:stream#   → RZ_CamStart  param=rear
#   $A:CamSE:modeCam:front:stream#  → RZ_CamStart  param=front
#   $A:CamSE:modeCam:*:off#         → RZ_CamStop   param=
#   $A:TorchSE:active:*:On#         → RZ_TorchOn   param=
#   $A:TorchSE:active:*:Off#        → RZ_TorchOff  param=
#
# RÈGLES TASKER (IMPORTANTES)
# ---------------------------
# - Le fichier trigger doit être dans /storage/emulated/0/ (accessible à Tasker)
# - /sdcard/ est un alias de /storage/emulated/0/ côté Termux uniquement
# - Tasker lit le fichier avec le chemin absolu /storage/emulated/0/...
# - Le JSON ne doit PAS contenir de \n final (utiliser printf, pas echo)
# - Dans le JavaScriplet Tasker : setGlobal('RZtask', ...) crée %RZtask (globale)
# - Dans Exécuter Tâche : utiliser %RZtask (au moins une majuscule = globale)
#
# INTERACTIONS
# ------------
#   Appelé par  : rz_appli_manager.sh (mode direct)
#                 rz_start.sh (mode daemon)
#   Lit         : fifo/fifo_tasker_in (mode daemon uniquement)
#   Écrit dans  : /storage/emulated/0/Tasker/RobotRZ/trigger.txt
#   Déclenche   : Profil Tasker 117 "Fichier modifié" → RZ_Dispatch
#   Log dans    : ~/robotRZ-smartSE/termux/logs/tasker_bridge.log
#
# PRÉCAUTIONS
# -----------
# - Tasker doit être actif en arrière-plan (pas tué, pas optimisé batterie)
# - Le profil Tasker doit pointer sur /storage/emulated/0/Tasker/RobotRZ/trigger.txt
# - allow-external-apps = true doit être dans ~/.termux/termux.properties
# - La tâche Tasker doit exister avec le nom exact transmis
# - En mode daemon : fifo_tasker_in doit exister (créé par fifo_manager.sh)
#
# DÉPENDANCES
# -----------
#   printf  : écriture JSON sans \n final
#   mkdir   : création du dossier trigger si absent
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (ajout mode daemon — routage fifo_tasker_in CamSE/TorchSE)
# DATE    : 2026-05-09
#
# HISTORIQUE
# ----------
#   2.0 (2026-05-03) : architecture FIFO, suppression mosquitto_pub direct
#   3.0 (2026-05-09) : ajout mode --daemon, routage CamSE/TorchSE
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

# Chemin absolu du fichier trigger — accessible à Tasker ET à Termux
TRIGGER_FILE="/storage/emulated/0/Tasker/RobotRZ/trigger.txt"
TRIGGER_DIR=$(dirname "$TRIGGER_FILE")

# Chemins SE
BASE_DIR="$HOME/robotRZ-smartSE/termux"
LOG_FILE="$BASE_DIR/logs/tasker_bridge.log"
FIFO_TASKER_IN="$BASE_DIR/fifo/fifo_tasker_in"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_bridge() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [TASKER_BRIDGE] $1" >> "$LOG_FILE"
}

# =============================================================================
# ÉCRITURE DU TRIGGER JSON
# Fonction commune aux deux modes.
# printf évite le \n final qui ferait échouer JSON.parse() dans Tasker.
# Le timestamp garantit que le fichier est toujours modifié → Tasker se déclenche.
# =============================================================================

write_trigger() {
    local task="$1"
    local param="${2:-}"

    # Créer le dossier trigger si absent
    if [ ! -d "$TRIGGER_DIR" ]; then
        if ! mkdir -p "$TRIGGER_DIR"; then
            log_bridge "ERREUR : impossible de créer $TRIGGER_DIR"
            return 1
        fi
        log_bridge "Dossier trigger créé : $TRIGGER_DIR"
    fi

    local ts
    ts=$(date +%s)

    if ! printf '{"task":"%s","param":"%s","ts":"%s"}' \
        "$task" "$param" "$ts" > "$TRIGGER_FILE"; then
        log_bridge "ERREUR : écriture trigger échouée pour $task"
        return 1
    fi

    log_bridge "Trigger → tâche=$task param=$param ts=$ts"
    return 0
}

# =============================================================================
# MODE DAEMON — Routage fifo_tasker_in → trigger.txt
#
# Lit fifo_tasker_in en continu.
# Parse les trames VPIV CAT=A MODULE=CamSE ou TorchSE.
# Mappe vers la tâche Tasker et écrit trigger.txt via write_trigger().
# =============================================================================

route_trame() {
    local trame="$1"

    # Format attendu : $A:MODULE:PROP:INST:VAL#
    if [[ ! "$trame" =~ ^\$A:([^:]+):([^:]+):([^:]+):(.*)#$ ]]; then
        log_bridge "WARN daemon : trame ignorée (format inattendu) : $trame"
        return
    fi

    local MODULE="${BASH_REMATCH[1]}"
     # BASH_REMATCH[2] = PROP (modeCam, active...) — non utilisé dans le routage
    local INST="${BASH_REMATCH[3]}"
    local VAL="${BASH_REMATCH[4]}"
    local task="" param=""

    case "$MODULE" in

        # ── Caméra ────────────────────────────────────────────────────────
        "CamSE")
            case "$VAL" in
                "stream"|"snapshot")
                    task="RZ_CamStart"
                    param="$INST"       # rear ou front
                    ;;
                "off"|"Off")
                    task="RZ_CamStop"
                    param=""
                    ;;
                *)
                    log_bridge "WARN daemon : CamSE VAL inconnue '$VAL'"
                    return
                    ;;
            esac
            ;;

        # ── Torche ────────────────────────────────────────────────────────
        "TorchSE")
            case "$VAL" in
                "On")  task="RZ_TorchOn";  param="" ;;
                "Off") task="RZ_TorchOff"; param="" ;;
                *)
                    log_bridge "WARN daemon : TorchSE VAL inconnue '$VAL'"
                    return
                    ;;
            esac
            ;;

        # ── MODULE inconnu ────────────────────────────────────────────────
        *)
            log_bridge "WARN daemon : MODULE inconnu '$MODULE' — trame ignorée"
            return
            ;;
    esac

    log_bridge "Daemon route → $task ($param)"
    write_trigger "$task" "$param"
}

daemon_mode() {
    log_bridge "=========================================="
    log_bridge "Mode daemon démarré v3.0"
    log_bridge "Écoute : $FIFO_TASKER_IN"
    log_bridge "=========================================="

    # Créer la FIFO si absente
    [ -p "$FIFO_TASKER_IN" ] || mkfifo "$FIFO_TASKER_IN"

    # Boucle infinie — lit une trame à la fois
    while true; do
        if read -r trame < "$FIFO_TASKER_IN"; then
            [ -n "$trame" ] && route_trame "$trame"
        fi
    done
}

# =============================================================================
# MODE DIRECT — Appel avec NOM_TACHE [PARAMETRE]
# Mode existant inchangé — appelé par rz_appli_manager.sh et autres scripts.
# =============================================================================

direct_mode() {
    local task="$1"
    local param="${2:-}"

    if [ -z "$task" ]; then
        echo "Usage: rz_tasker_bridge.sh NOM_TACHE [PARAMETRE]"
        echo "       rz_tasker_bridge.sh --daemon"
        echo "Exemple: rz_tasker_bridge.sh RZ_CamStart rear"
        log_bridge "ERREUR : appel sans NOM_TACHE"
        exit 1
    fi

    write_trigger "$task" "$param" || exit 1
    exit 0
}

# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

case "${1:-}" in
    --daemon)
        daemon_mode
        ;;
    *)
        direct_mode "$1" "$2"
        ;;
esac