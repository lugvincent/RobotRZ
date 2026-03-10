#!/bin/bash
# =============================================================================
# SCRIPT: fifo_manager.sh
# CHEMIN: termux/utils/fifo_manager.sh
# DESCRIPTION:
#   Gestion centralisée des files FIFO, tourne en boucle
#   pour assurer la communication entre les différents processus (Tasker, Termux, IA).
#   stt_handler.sh envoie les trames VPIV à fifo_termux_in, 
#   qui sont ensuite traitées par le circuit de décision.
#   fifo_termux_in est le point d'entrée pour les commandes vocales, 
#   tandis que fifo_tasker_in reçoit les commandes de Tasker.
#   fifo_return est utilisé pour les retours d'information vers Tasker
#   ou pour la bascule IA : exit 1 = non trouvé, exit 0 = trouvé.
#ARTICULATION :
#   1. Création des FIFO (create) -> fifo_tasker_in, fifo_termux_in, fifo_return
#   2. Nettoyage des FIFO (clean)
#   3. Vérification de l'existence des FIFO (check)
#   4 Ecoute en boucle pour traiter les commandes entrantes et les retours d'information  
#   5. Utilisation dans les scripts de communication (stt_handler.sh, decision_circuit.sh, etc.)
#   
# =============================================================================

# Chemins
FIFO_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux/fifo"  # Sur smartphone
# FIFO_DIR="/tmp/fifo"   # Sur PC si nécessaire

# Création des FIFO
create_fifos() {
  mkdir -p "$FIFO_DIR"
  [ -p "$FIFO_DIR/fifo_tasker_in" ] || mkfifo "$FIFO_DIR/fifo_tasker_in"
  [ -p "$FIFO_DIR/fifo_termux_in" ] || mkfifo "$FIFO_DIR/fifo_termux_in"
  [ -p "$FIFO_DIR/fifo_return" ] || mkfifo "$FIFO_DIR/fifo_return"
  [ -p "$FIFO_DIR/fifo_appli_in" ] || mkfifo "$FIFO_DIR/fifo_appli_in"
}

# Nettoyage des FIFO
clean_fifos() {
  rm -f "$FIFO_DIR"/fifo_*
}

# Vérification des FIFO
check_fifos() {
  for fifo in fifo_tasker_in fifo_termux_in fifo_return fifo_appli_in; do
    if [ ! -p "$FIFO_DIR/$fifo" ]; then
      echo "FIFO manquant: $fifo"
      return 1
    fi
  done
  return 0
}

# Exécution
if [ "$1" = "create" ]; then
  create_fifos
elif [ "$1" = "clean" ]; then
  clean_fifos
elif [ "$1" = "check" ]; then
  check_fifos
else
  echo "Usage: fifo_manager.sh {create|clean|check}"
fi