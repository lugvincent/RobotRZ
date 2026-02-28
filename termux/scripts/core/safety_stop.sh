#!/bin/bash
# =============================================================================
# SCRIPT: safety_stop.sh
# CHEMIN: termux/scripts/core/safety_stop.sh
# OBJECTIFS: 
#    - Mise en sécurité immédiate du robot lors d'un état "danger".
#    - Interruption des flux gourmands et arrêt des moteurs.
# INTERACTIONS:
#    - Appelé par rz_vpiv_parser.sh (Point C) dès réception de Urg:etat:danger.
#    - Envoie des ordres prioritaires au FIFO_TERMUX_IN pour les managers.
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
FIFO_TERMUX="$BASE_DIR/fifo/fifo_termux_in"

# 1. ARRÊT DES FLUX ET MOTEURS
# On utilise la catégorie 'V' (Consigne) pour forcer l'arrêt
echo "$V:MotA:state:*:Stop#" > "$FIFO_TERMUX"      # Ordre direct Arduino
echo "$V:CamSE:active:*:Off#" > "$FIFO_TERMUX"     # Coupure streaming
echo "$V:TorchSE:power:*:0#" > "$FIFO_TERMUX"      # Extinction lumière

# 2. FEEDBACK SYSTÈME
echo "$I:COM:error:SE:\"SÉCURITÉ ACTIVE : Système verrouillé.\"" > "$FIFO_TERMUX"

# 3. LOG DE L'ÉVÉNEMENT
echo "[$(date '+%H:%M:%S')] SAFETY STOP TRIGGERED" >> "$BASE_DIR/logs/emergency.log"