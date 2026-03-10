#!/bin/bash
# =============================================================================
# SCRIPT  : safety_stop.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/core/safety_stop.sh
#
# OBJECTIF
# --------
# Signalement d'urgence immédiat vers SP lors d'un état "danger".
# SE ne donne jamais d'ordre direct à Arduino — seul SP décide et orchestre.
#
# DESCRIPTION
# -----------
# Ce script :
#   1. Émet le signal d'urgence vers SP ($E:Urg:source) via FIFO.
#   2. Envoie un message d'information COM:error pour l'interface SP.
#   3. Met à jour global.json localement pour cohérence des états SE.
#   4. Logue l'événement dans emergency.log.
#
# CE QUE CE SCRIPT NE FAIT PAS
# -----------------------------
# ✗ N'envoie PAS d'ordre à Arduino ($V:MotA, $V:CamSE...).
#   → C'est SP qui décide de l'arrêt Arduino après réception de l'urgence.
# ✗ N'appelle PAS mosquitto_pub directement.
#   → Tout passe par fifo_termux_in → rz_vpiv_parser.sh → MQTT.
#
# INTERACTIONS
# ------------
#   Appelé par : rz_vpiv_parser.sh (dès réception de $E:Urg:etat:*:danger#)
#   Écrit dans : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#   Met à jour : config/global.json (Urg.etat, Urg.source)
#
# VPIV ENVOYÉS
# ------------
#   $E:Urg:source:SE:URG_DANGER#                        Signal urgence → SP décide
#   $I:COM:error:SE:"SÉCURITÉ ACTIVE : arrêt demandé"#  Feedback interface
#
# PRÉCAUTIONS
# -----------
# ⚠️ Les trames VPIV commencent par \$ (dollar échappé, PAS une variable shell).
#    $V serait une variable shell vide → trame corrompue → bug silencieux.
# - Ce script doit s'exécuter rapidement (urgence critique, < 500ms).
# - emergency.log est toujours ajouté (>>), jamais écrasé.
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (Architecture corrigée : SE signale, SP décide)
# DATE    : 2026-03-04
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS (détection automatique)
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

FIFO_TERMUX="$BASE_DIR/fifo/fifo_termux_in"
GLOBAL_FILE="$BASE_DIR/config/global.json"
LOG_EMERGENCY="$BASE_DIR/logs/emergency.log"

# =============================================================================
# INITIALISATION
# =============================================================================

mkdir -p "$BASE_DIR/logs"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] SAFETY STOP TRIGGERED" >> "$LOG_EMERGENCY"

# =============================================================================
# VÉRIFICATION FIFO
# =============================================================================

if [ ! -p "$FIFO_TERMUX" ]; then
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] SAFETY STOP : FIFO absente — signal urgence impossible" >> "$LOG_EMERGENCY"
    exit 1
fi

# =============================================================================
# 1. SIGNAL URGENCE VERS SP
# SE signale l'urgence. SP décide de tout (arrêt Arduino, etc.).
# ⚠️ \$ obligatoire — $E serait une variable shell vide (trame corrompue)
# =============================================================================

echo "\$E:Urg:source:SE:URG_DANGER#" > "$FIFO_TERMUX"

# =============================================================================
# 2. FEEDBACK INTERFACE (COM:error pour affichage côté SP/Node-RED)
# =============================================================================

echo "\$I:COM:error:SE:\"SÉCURITÉ ACTIVE : arrêt demandé au SP.\"#" > "$FIFO_TERMUX"

# =============================================================================
# 3. MISE À JOUR GLOBAL.JSON (cohérence état local SE)
# SP est la seule autorité pour le clear, mais SE doit refléter son état réel.
# =============================================================================

if [ -f "$GLOBAL_FILE" ] && command -v jq &>/dev/null; then
    jq '.Urg.etat = "active" | .Urg.source = "URG_DANGER"' \
       "$GLOBAL_FILE" > "${GLOBAL_FILE}.tmp" \
    && mv "${GLOBAL_FILE}.tmp" "$GLOBAL_FILE"
fi

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Signal envoyé. SP en attente." >> "$LOG_EMERGENCY"