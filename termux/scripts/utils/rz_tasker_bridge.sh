#!/bin/bash
# =============================================================================
# SCRIPT  : rz_tasker_bridge.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/utils/rz_tasker_bridge.sh
#
# OBJECTIF
# --------
# Pont de communication entre les scripts Termux (SE) et Tasker (Android).
# Déclenche une tâche Tasker nommée en écrivant un fichier trigger JSON
# dans un emplacement accessible à Tasker (/storage/emulated/0/Tasker/RobotRZ/).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script est appelé par rz_vpiv_parser.sh lorsqu'une trame VPIV CAT=A
# est reçue depuis SP. Il encode les paramètres en JSON et écrit le fichier
# trigger.txt. Un profil Tasker (Événement → Fichier modifié) surveille ce
# fichier et déclenche la tâche RZ_Dispatch, qui lit le JSON et appelle
# la tâche Tasker correspondante.
#
# ARCHITECTURE DU DÉCLENCHEMENT
# ------------------------------
#   rz_vpiv_parser.sh
#       → fifo_tasker_in
#           → rz_tasker_bridge.sh (ce script)
#               → /storage/emulated/0/Tasker/RobotRZ/trigger.txt
#                   → Profil Tasker (Fichier modifié)
#                       → RZ_Dispatch (lit JSON, appelle %RZtask)
#                           → RZ_CamStart / RZ_CamStop / RZ_Expression / ...
#
# ARGUMENTS
# ---------
#   $1 = NOM_TACHE  : nom exact de la tâche Tasker à déclencher
#                     (ex: RZ_CamStart, RZ_CamStop, RZ_Expression, RZ_TorchOn)
#   $2 = PARAMETRE  : paramètre optionnel transmis à la tâche
#                     (ex: rear, front, sourire, on)
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
#   Appelé par  : rz_vpiv_parser.sh (via fifo_tasker_in)
#   Écrit dans  : /storage/emulated/0/Tasker/RobotRZ/trigger.txt
#   Déclenche   : Profil Tasker "Fichier modifié" → RZ_Dispatch
#   Log dans    : ~/robotRZ-smartSE/termux/logs/tasker_bridge.log
#
# PRÉCAUTIONS
# -----------
# - Tasker doit être actif en arrière-plan (pas tué, pas optimisé batterie)
# - Le profil Tasker doit pointer sur /storage/emulated/0/Tasker/RobotRZ/trigger.txt
# - allow-external-apps = true doit être dans ~/.termux/termux.properties
# - La tâche Tasker doit exister avec le nom exact transmis en $1
#
# DÉPENDANCES
# -----------
#   printf  : écriture JSON sans \n final
#   mkdir   : création du dossier trigger si absent
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0
# DATE    : 2026-05-03
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

# Chemin absolu du fichier trigger — accessible à Tasker ET à Termux
TRIGGER_FILE="/storage/emulated/0/Tasker/RobotRZ/trigger.txt"

# Chemin des logs
BASE_DIR="$HOME/robotRZ-smartSE/termux"
LOG_FILE="$BASE_DIR/logs/tasker_bridge.log"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_bridge() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [TASKER_BRIDGE] $1" >> "$LOG_FILE"
}

# =============================================================================
# VALIDATION DES ARGUMENTS
# =============================================================================

TASK_NAME="$1"
PARAM="${2:-}"   # Paramètre optionnel — vide si absent

if [ -z "$TASK_NAME" ]; then
    echo "Usage: rz_tasker_bridge.sh NOM_TACHE [PARAMETRE]"
    echo "Exemple: rz_tasker_bridge.sh RZ_CamStart rear"
    log_bridge "ERREUR : appel sans NOM_TACHE"
    exit 1
fi

# =============================================================================
# CRÉATION DU DOSSIER TRIGGER SI ABSENT
# =============================================================================

TRIGGER_DIR=$(dirname "$TRIGGER_FILE")

if [ ! -d "$TRIGGER_DIR" ]; then
    if ! mkdir -p "$TRIGGER_DIR"; then
        log_bridge "ERREUR : impossible de créer $TRIGGER_DIR"
        exit 1
    fi
    log_bridge "Dossier trigger créé : $TRIGGER_DIR"
fi

# =============================================================================
# ÉCRITURE DU TRIGGER JSON
# printf utilisé à la place de echo pour éviter le \n final
# Le \n final dans trigger.txt fait échouer JSON.parse() dans le JavaScriplet Tasker
# Le timestamp garantit que le fichier est toujours modifié → Tasker se déclenche
# =============================================================================

TS=$(date +%s)

if ! printf '{"task":"%s","param":"%s","ts":"%s"}' \
    "$TASK_NAME" \
    "$PARAM" \
    "$TS" \
    > "$TRIGGER_FILE"; then
    log_bridge "ERREUR : écriture trigger échouée pour $TASK_NAME"
    exit 1
fi

log_bridge "Trigger envoyé → tâche=$TASK_NAME param=$PARAM ts=$TS"

exit 0