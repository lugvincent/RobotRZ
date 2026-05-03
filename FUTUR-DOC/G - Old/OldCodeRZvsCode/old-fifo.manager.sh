#!/bin/bash
# =============================================================================
# SCRIPT: fifo_manager.sh
# CHEMIN: termux/scripts/utils/fifo_manager.sh
# DESCRIPTION: Gestion centralisée des files FIFO pour la communication entre 
# Tasker et Termux et pour les retours de données. 
# Ce script permet de créer, nettoyer et vérifier les FIFO nécessaires au projet RZ.
# Rôle des FIFO :
# - fifo_tasker_in : pour les commandes envoyées de Tasker vers Termux
# - fifo_termux_in : pour les commandes envoyées de Termux vers Tasker
# - fifo_return : pour les données retournées de Termux vers Tasker
# Version 2024-06 : Ajout de la détection dynamique du chemin 
# auteur: philippeVincent
# =============================================================================

# --- DÉTECTION DYNAMIQUE DU CHEMIN ---
if [ -d "/data/data/com.termux/files" ]; then
    # Chemin Smartphone
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    # Chemin PC (Calculé par rapport à l'emplacement du script)
    BASE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
fi

FIFO_DIR="$BASE_DIR/fifo"
FIFOS=("fifo_tasker_in" "fifo_termux_in" "fifo_return")

# Création des FIFO
create_fifos() {
  echo "Initialisation des FIFOs dans : $FIFO_DIR"
  mkdir -p "$FIFO_DIR"
  for f in "${FIFOS[@]}"; do
    CIBLE="$FIFO_DIR/$f"
    if [ -e "$CIBLE" ]; then
        if [ ! -p "$CIBLE" ]; then
            echo "[!] $f n'est pas une pipe. Correction..."
            rm -f "$CIBLE"
            mkfifo "$CIBLE"
        fi
    else
        echo "[*] Création de la FIFO : $f"
        mkfifo "$CIBLE"
    fi
    chmod 666 "$CIBLE"
  done
}

# Nettoyage des FIFO
clean_fifos() {
  echo "Nettoyage du dossier FIFO..."
  rm -f "$FIFO_DIR"/fifo_*
}

# Vérification des FIFO
check_fifos() {
  for f in "${FIFOS[@]}"; do
    if [ ! -p "$FIFO_DIR/$f" ]; then
      echo "FIFO manquant ou invalide: $f"
      return 1
    fi
  done
  echo "Toutes les FIFOs sont OK."
  return 0
}

# Exécution
case "$1" in
    create) create_fifos ;;
    clean)  clean_fifos ;;
    check)  check_fifos ;;
    *)      echo "Usage: fifo_manager.sh {create|clean|check}" ;;
esac