#!/bin/bash
# =============================================================================
# SCRIPT: fifo_manager.sh
# CHEMIN: termux/utils/fifo_manager.sh
# DESCRIPTION:
#   Gestion centralisée des files FIFO
# =============================================================================

# Chemins
FIFO_DIR="/data/data/com.termux/files/home/scripts_RZ_SEfifo"  # Sur smartphone
# FIFO_DIR="/tmp/fifo"   # Sur PC si nécessaire

# Création des FIFO
create_fifos() {
  mkdir -p "$FIFO_DIR"
  [ -p "$FIFO_DIR/fifo_tasker_in" ] || mkfifo "$FIFO_DIR/fifo_tasker_in"
  [ -p "$FIFO_DIR/fifo_termux_in" ] || mkfifo "$FIFO_DIR/fifo_termux_in"
  [ -p "$FIFO_DIR/fifo_return" ] || mkfifo "$FIFO_DIR/fifo_return"
}

# Nettoyage des FIFO
clean_fifos() {
  rm -f "$FIFO_DIR"/fifo_*
}

# Vérification des FIFO
check_fifos() {
  for fifo in fifo_tasker_in fifo_termux_in fifo_return; do
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
