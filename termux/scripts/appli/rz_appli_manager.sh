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
#   PROP=IA_Conv    VAL=On/Off  → RZ_IA_Conv  On|Off
#   PROP=zoom       VAL=On/Off  → RZ_Zoom     On|Off  (+ verrou camera/microphone, V1.2)
#   PROP=NavGPS     VAL=On/Off  → RZ_NavGPS   On|Off
#   PROP=ExprTasker VAL=*       → RZ_Expression <VAL>  (pas d'ACK)
#   PROP=tasker     VAL=On      → RZ_OuvreMenu
#
# VERROU RESSOURCES PARTAGÉES — Zoom (V1.2 — juin 2026)
# ---------------------------------------------------------
# Zoom mobilise simultanément la caméra ET le microphone du smartphone pour la
# visioconférence. À PROP=zoom VAL=On, ce script pose les verrous "camera" et
# "microphone" (owner="zoom") via rz_resource_lock.sh AVANT de déclencher
# RZ_Zoom. Si l'une des deux ressources est déjà détenue par un autre acteur
# (ex: "ipwebcam" sur camera), Zoom n'est PAS lancé : COM:error informe
# l'utilisateur (relayé en TTS par rz_vpiv_parser.sh si pilotage vocal actif).
# À PROP=zoom VAL=Off, les deux verrous sont levés (uniquement si "zoom" en
# est bien le détenteur — protection rz_resource_lock.sh).
# Voir Table A : VAR LockSE (CAT=I) pour l'état exposé à SP.
#
# VPIV ENTRANTS (depuis fifo_appli_in)
# -------------------------------------
#   $A:Appli:IA_Conv:*:On#
#   $A:Appli:IA_Conv:*:Off#
#   $A:Appli:zoom:*:On#
#   $A:Appli:zoom:*:Off#
#   $A:Appli:NavGPS:*:On#
#   $A:Appli:NavGPS:*:Off#
#   $A:Appli:ExprTasker:Expression:sourire#
#   $A:Appli:tasker:*:On#
#
# ARTICULATION
# ------------
#   Lit     : fifo/fifo_appli_in  (écrit par rz_vpiv_parser.sh)
#   Appelle : rz_tasker_bridge.sh (déclenche trigger.txt → Tasker)
#   Source  : rz_resource_lock.sh (lock_resource/unlock_resource) (V1.2)
#   Log     : logs/appli_manager.log
#
# PRÉCAUTIONS
# -----------
# - fifo_appli_in doit exister (créé par fifo_manager.sh create)
# - rz_tasker_bridge.sh doit être accessible et exécutable
# - Tasker doit être actif en arrière-plan (profil actif)
# - Les ACK retour sont gérés par Tasker via vpiv_out.txt
#   → ne pas attendre de retour dans ce script
# - Si Zoom est fermé manuellement par l'utilisateur (pas via VAL=Off VPIV),
#   les verrous camera/microphone NE seront PAS levés automatiquement — c'est
#   une limite connue du modèle actuel (pas de détection d'état réel du process
#   Zoom). À surveiller si ça pose problème en usage réel.
#
# DÉPENDANCES
# -----------
#   rz_tasker_bridge.sh
#   rz_resource_lock.sh (V1.2)
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.2  (juin 2026 — verrou camera/microphone pour Zoom)
# DATE    : 2026-06-22
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
RESOURCE_LOCK_SCRIPT="$BASE_DIR/scripts/utils/rz_resource_lock.sh"
LOG_FILE="$BASE_DIR/logs/appli_manager.log"

# Nom de cet acteur dans le système de verrous, pour les apps qui en ont besoin
ZOOM_LOCK_OWNER="zoom"

# =============================================================================
# UTILITAIRES
# =============================================================================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [APPLI] $1" | tee -a "$LOG_FILE"
}

# Chargement des fonctions de verrou ressource (V1.2)
if [ -f "$RESOURCE_LOCK_SCRIPT" ]; then
    # shellcheck source=./rz_resource_lock.sh
    source "$RESOURCE_LOCK_SCRIPT"
else
    log "WARN : rz_resource_lock.sh introuvable ($RESOURCE_LOCK_SCRIPT) — verrouillage désactivé"
fi

# Envoi VPIV direct vers fifo_termux_in (pour COM:error, LockSE — hors fifo_appli_in)
# $1 = trame complète
send_vpiv_direct() {
    local trame="$1"
    local fifo_termux="$BASE_DIR/fifo/fifo_termux_in"
    if [ ! -p "$fifo_termux" ]; then
        log "WARN : fifo_termux_in absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$fifo_termux" &
    local pid_write=$!
    ( sleep 2 && kill "$pid_write" 2>/dev/null ) &
    wait "$pid_write" 2>/dev/null
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

        # ── IA_Conv — conversation Gemini ────────────────────────────────────
        "IA_Conv"|"IA")
            TASK_NAME="RZ_IA_Conv"
            TASK_PARAM="$VAL"
            ;;

        # ── Zoom — mobilise caméra ET microphone (verrou V1.2) ────────────
        "zoom"|"Zoom")
            if [[ "$VAL" == "On" ]]; then
                # Tentative de verrouillage des deux ressources avant lancement.
                # Si l'une échoue, on libère celle déjà posée et on refuse.
                if command -v lock_resource &>/dev/null; then
                    local holder_cam holder_mic

                    holder_cam=$(lock_resource camera "$ZOOM_LOCK_OWNER")
                    if [ $? -ne 0 ]; then
                        log "REFUS zoom : caméra déjà détenue par '$holder_cam'"
                        send_vpiv_direct "\$E:CamSE:error:*:CAM_LOCKED_${holder_cam}#"
                        send_vpiv_direct "\$I:COM:error:SE:\"Zoom indisponible : caméra utilisée par ${holder_cam}\"#"
                        return 1
                    fi

                    holder_mic=$(lock_resource microphone "$ZOOM_LOCK_OWNER")
                    if [ $? -ne 0 ]; then
                        log "REFUS zoom : microphone déjà détenu par '$holder_mic'"
                        unlock_resource camera "$ZOOM_LOCK_OWNER" > /dev/null
                        send_vpiv_direct "\$I:COM:error:SE:\"Zoom indisponible : microphone utilisé par ${holder_mic}\"#"
                        return 1
                    fi

                    send_vpiv_direct "\$I:LockSE:camera:SE:${ZOOM_LOCK_OWNER}#"
                    send_vpiv_direct "\$I:LockSE:microphone:SE:${ZOOM_LOCK_OWNER}#"
                fi
            elif [[ "$VAL" == "Off" ]]; then
                # Libération des deux verrous (protégée : seulement si "zoom" en est détenteur)
                if command -v unlock_resource &>/dev/null; then
                    unlock_resource camera "$ZOOM_LOCK_OWNER" > /dev/null
                    unlock_resource microphone "$ZOOM_LOCK_OWNER" > /dev/null
                    send_vpiv_direct "\$I:LockSE:camera:SE:libre#"
                    send_vpiv_direct "\$I:LockSE:microphone:SE:libre#"
                fi
            fi
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
    log "Démarrage rz_appli_manager.sh v1.2"
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
