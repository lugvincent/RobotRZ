#!/bin/bash
# =============================================================================
# SCRIPT: rz_tasker_bridge.sh
# DESCRIPTION: Fait le lien entre le système RZ et Tasker.
# DEPENDANCES: Plugin Termux:Tasker installé.
# =============================================================================

TASK_NAME="$1"  # Nom de la tâche Tasker
PARAM_1="$2"    # Paramètre optionnel (ex: valeur, niveau, couleur)

# Chemin vers les scripts autorisés par le plugin Termux:Tasker
# Note: Tasker ne peut exécuter que des scripts situés dans ce dossier spécifique
TASKER_DIR="$HOME/.termux/tasker"
mkdir -p "$TASKER_DIR"

log_tasker() {
    echo "[$(date '+%H:%M:%S')] [TASKER] Appel de $TASK_NAME avec $PARAM_1" >> ~/scripts_RZ_SE/termux/logs/tasker_bridge.log
}

if [ -z "$TASK_NAME" ]; then
    echo "Usage: rz_tasker_bridge.sh NOM_TACHE [PARAMETRE]"
    exit 1
fi

log_tasker

# EXÉCUTION
# On utilise l'outil de bridge fourni par l'installation de Termux:Tasker
# La tâche dans Tasker doit porter exactement le même nom.
termux-tasker "$TASK_NAME" "$PARAM_1"