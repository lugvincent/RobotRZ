#!/bin/bash
# =============================================================================
# SCRIPT  : rz_appli_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/appli/rz_appli_manager.sh
#
# OBJECTIF
# --------
# Daemon de gestion des applications Android côté SE.
# Lit fifo_appli_in en continu, décode les trames VPIV CAT=A MODULE=Appli
# et déclenche la tâche Tasker correspondante via rz_tasker_bridge.sh.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Lit fifo_appli_in (trames écrites par rz_vpiv_parser.sh)
# 2. Parse la trame VPIV : $A:Appli:PROP:INST:VAL#
# 3. Mappe PROP+VAL vers une tâche Tasker + paramètre
# 4. Appelle rz_tasker_bridge.sh NOM_TACHE PARAM
# 5. Le ACK retour est géré par Tasker (écrit dans vpiv_out.txt)
#
# TABLE DE ROUTAGE
# ----------------
#   PROP=BabyCam    VAL=On/Off  → RZ_Baby     On|Off
#   PROP=IA_Conv    VAL=On/Off  → RZ_IA_Conv  On|Off
#   PROP=zoom       VAL=On/Off  → RZ_Zoom     On|Off
#   PROP=NavGPS     VAL=On/Off  → RZ_NavGPS   On|Off
#   PROP=ExprTasker VAL=*       → RZ_Expression <VAL>  (pas d'ACK)
#   PROP=tasker     VAL=On      → RZ_OuvreMenu
#
# VPIV ENTRANTS (depuis fifo_appli_in)
# -------------------------------------
#   $A:Appli:BabyCam:*:On#
#   $A:Appli:BabyCam:*:Off#
#   $A:Appli:IA_Conv:*:On#
#   $A:Appli:IA_Conv:*:Off#
#   $A:Appli:zoom:*:On#
#   $A:Appli:NavGPS:*:On#
#   $A:Appli:NavGPS:*:Off#
#   $A:Appli:ExprTasker:Expression:sourire#
#   $A:Appli:tasker:*:On#
#
# ARTICULATION
# ------------
#   Lit     : fifo/fifo_appli_in  (écrit par rz_vpiv_parser.sh)
#   Appelle : rz_tasker_bridge.sh (déclenche trigger.txt → Tasker)
#   Log     : logs/appli_manager.log
#
# PRÉCAUTIONS
# -----------
# - fifo_appli_in doit exister (créé par fifo_manager.sh create)
# - rz_tasker_bridge.sh doit être accessible et exécutable
# - Tasker doit être actif en arrière-plan (profil 222 actif)
# - Les ACK retour sont gérés par Tasker via vpiv_out.txt
#   → ne pas attendre de retour dans ce script
#
# DÉPENDANCES
# -----------
#   rz_tasker_bridge.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0  (création — routage CAT=A MODULE=Appli vers Tasker)
# DATE    : 2026-05-09
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
fi

FIFO_APPLI="$BASE_DIR/fifo/fifo_appli_in"
BRIDGE="$BASE_DIR/scripts/utils/rz_tasker_bridge.sh"
LOG_FILE="$BASE_DIR/logs/appli_manager.log"

# =============================================================================
# UTILITAIRES
# =============================================================================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [APPLI] $1" | tee -a "$LOG_FILE"
}

# =============================================================================
# ROUTAGE — Mappe une trame VPIV vers tâche Tasker + paramètre
#
# Paramètres : $1=PROP  $2=INST  $3=VAL
# Retourne via variables globales : TASK_NAME et TASK_PARAM
# =============================================================================

route_appli() {
    local PROP="$1"
    local INST="$2"
    local VAL="$3"
    TASK_NAME=""
    TASK_PARAM=""

    case "$PROP" in

        # ── BabyCam ──────────────────────────────────────────────────────────
        "BabyCam"|"Baby")
            TASK_NAME="RZ_Baby"
            TASK_PARAM="$VAL"
            ;;
        # ── IA_Conv ──────────────────────────────────────────────────────────
        "IA_Conv"|"IA")
            TASK_NAME="RZ_IA_Conv"
            TASK_PARAM="$VAL"
            ;;

        # ── Zoom ─────────────────────────────────────────────────────────────
        "zoom"|"Zoom")
            TASK_NAME="RZ_Zoom"
            TASK_PARAM="$VAL"
            ;;

        # ── Navigation GPS ───────────────────────────────────────────────────
        "NavGPS")
            TASK_NAME="RZ_NavGPS"
            TASK_PARAM="$VAL"
            ;;

        # ── Expression Tasker — VAL = nom expression ──────────────────────
        # INST contient "Expression" (champ VPIV utilisé comme contexte)
        # Pas d'ACK — l'expression revient à neutre automatiquement
        "ExprTasker")
            TASK_NAME="RZ_Expression"
            TASK_PARAM="$VAL"   # ex: sourire, colere, triste...
            ;;

        # ── Lancer/afficher Tasker (menu principal) ───────────────────────
        "tasker")
            if [[ "$VAL" == "On" ]]; then
                TASK_NAME="RZ_OuvreMenu"
                TASK_PARAM=""
            fi
            ;;

        # ── PROP inconnue ─────────────────────────────────────────────────
        *)
            log "WARN : PROP inconnue '$PROP' — trame ignorée"
            return 1
            ;;
    esac

    return 0
}

# =============================================================================
# PROCESS — Traite une trame VPIV reçue de fifo_appli_in
# =============================================================================

process_trame() {
    local trame="$1"

    # Validation format VPIV : $A:Appli:PROP:INST:VAL#
    if [[ ! "$trame" =~ ^\$A:Appli:([^:]+):([^:]+):(.*)#$ ]]; then
        log "WARN : trame ignorée (format inattendu) : $trame"
        return
    fi

    local PROP="${BASH_REMATCH[1]}"
    local INST="${BASH_REMATCH[2]}"
    local VAL="${BASH_REMATCH[3]}"

    log "Reçu : PROP=$PROP INST=$INST VAL=$VAL"

    # Routage vers tâche Tasker
    if route_appli "$PROP" "$INST" "$VAL"; then
        if [ -n "$TASK_NAME" ]; then
            log "→ Tasker : $TASK_NAME ($TASK_PARAM)"
            bash "$BRIDGE" "$TASK_NAME" "$TASK_PARAM"
        fi
    fi
}

# =============================================================================
# BOUCLE PRINCIPALE — Daemon lecture fifo_appli_in
# =============================================================================

main() {
    log "=========================================="
    log "Démarrage rz_appli_manager.sh v1.0"
    log "=========================================="

    # Vérifications
    if [ ! -f "$BRIDGE" ]; then
        log "ERREUR : bridge introuvable ($BRIDGE)"
        exit 1
    fi

    # Créer la FIFO si absente
    [ -p "$FIFO_APPLI" ] || mkfifo "$FIFO_APPLI"

    log "En écoute sur fifo_appli_in..."

    # Boucle infinie — lit une trame à la fois
    while true; do
        if read -r trame < "$FIFO_APPLI"; then
            [ -n "$trame" ] && process_trame "$trame"
        fi
    done
}

main