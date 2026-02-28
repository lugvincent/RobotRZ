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

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
FIFO_DIR="$BASE_DIR/fifo"
LOG_FILE="$BASE_DIR/logs/fifo_manager.log"
INTERPRETER="$BASE_DIR/scripts/utils/rz_tasker_interpreter.sh"

log() { echo "[$(date '+%H:%M:%S')] [FIFO_MGR] $1" >> "$LOG_FILE"; }

create_fifos() {
    mkdir -p "$FIFO_DIR"
    for f in fifo_tasker_in fifo_termux_in fifo_return; do
        [ -p "$FIFO_DIR/$f" ] || mkfifo "$FIFO_DIR/$f"
    done
    log "FIFOs créés ou vérifiés dans $FIFO_DIR"
}

# --- LA FONCTION MANQUANTE : L'ÉCOUTE ACTIVE ---
listen_and_route() {
    create_fifos
    log "Démarrage de l'écoute active..."

    # Boucle infinie pour maintenir le pipe ouvert
    while true; do
        # On lit le pipe ligne par ligne
        if read -r trame < "$FIFO_DIR/fifo_termux_in"; then
            if [[ -n "$trame" ]]; then
                log "Trame reçue : $trame"
                
                # AIGUILLAGE : 
                # Si c'est une commande Tasker (Catégorie A ou E dans tes VPIV)
                # Ou si c'est une commande moteur (V) à envoyer au robot
                bash "$INTERPRETER" "$trame"
            fi
        fi
    done
}

clean_fifos() {
    rm -f "$FIFO_DIR"/fifo_*
    log "FIFOs supprimés."
}

# --- EXÉCUTION ---
case "$1" in
    "create") create_fifos ;;
    "clean")  clean_fifos ;;
    "listen") listen_and_route ;; # <--- Le mode principal à lancer au boot
    *) echo "Usage: $0 {create|clean|listen}" ;;
esac
