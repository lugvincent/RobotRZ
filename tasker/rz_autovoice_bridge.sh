#!/bin/bash
# =============================================================================
# SCRIPT  : rz_autovoice_bridge.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_autovoice_bridge.sh
#
# OBJECTIF
# --------
# Pont entre AutoVoice (Tasker) et le handler vocal RZ.
# Reçoit le texte reconnu par AutoVoice via Termux:Tasker,
# le passe à rz_stt_handler.sh qui gère tout le reste
# (catalogue, filtres sécurité, trames VPIV, TTS feedback).
#
# RÉPARTITION DES RESPONSABILITÉS
# --------------------------------
# TASKER/AUTOVOICE gère :
#   - Filtrage wake word "RZ" (Command Filter ^RZ, Trigger Word RZ)
#   - Retrait du préfixe "rz" du texte (%avcommnofilter)
#   - Sélection mode écoute selon %RZsttContext (multi-contextes profils)
#   - Bascule Gemini sur No Match (profil RZ_NoMatch → RZ_GeminiConv)
#   - Gestion échec reconnaissance (profil RZ_RecFailed → RZ_VoiceError)
#   - Écoute continue (Continuous mode AutoVoice)
#
# CE SCRIPT gère :
#   - Normalisation légère du texte (espaces multiples, casse)
#   - Préfixe spéciaux : GEMINI: (bascule directe IA) et ERROR: (erreur recog)
#   - Appel rz_stt_handler.sh (catalogue → VPIV → MQTT)
#   - Logging pour diagnostic
#   - Gestion erreurs fatales (handler introuvable)
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Trois modes selon l'argument reçu :
#
#   1. Texte normal (ex: "stop", "avance", "mode veille")
#      → Normalisé → rz_stt_handler.sh → catalogue → VPIV
#      Note : %avcommnofilter ne contient PAS "rz" — AutoVoice l'a retiré
#
#   2. Préfixe GEMINI: (ex: "GEMINI:rz discute avec moi")
#      → Envoyé directement à rz_ai_conversation.py
#      → Déclenché par profil RZ_NoMatch (Tasker) en contexte Conv/Mixte
#
#   3. Préfixe ERROR: (ex: "ERROR:recognition_failed")
#      → TTS "je n'ai pas compris" + log
#      → Déclenché par profil RZ_RecFailed (Tasker)
#
# PIPELINE COMPLET
# ----------------
#   AutoVoice détecte wake word "RZ" + commande
#   → Tasker profil RZ_EcouteCommandes ou RZ_EcouteConversation
#   → Tâche RZ_VoiceCmd → Termux:Tasker
#   → rz_autovoice_bridge.sh "%avcommnofilter"
#   → rz_stt_handler.sh "commande"
#   → stt_catalog.json → trame VPIV → fifo_termux_in → MQTT → SP
#
#   Si No Match :
#   → Tasker profil RZ_NoMatch → Tâche RZ_GeminiConv
#   → rz_autovoice_bridge.sh "GEMINI:%avcomm"
#   → rz_ai_conversation.py → Gemini API → TTS
#
#   Si Recognition Failed :
#   → Tasker profil RZ_RecFailed → Tâche RZ_VoiceError
#   → rz_autovoice_bridge.sh "ERROR:recognition_failed"
#   → TTS "je n'ai pas compris"
#
# ARTICULATION
# ------------
#   Appelé par : RZ_VoiceCmd, RZ_GeminiConv, RZ_VoiceError (via Termux:Tasker)
#   Appelle    : rz_stt_handler.sh   (catalogue → VPIV)
#   Appelle    : rz_ai_conversation.py (Gemini, mode GEMINI:)
#   Log        : logs/autovoice_bridge.log
#
# PRÉCAUTIONS
# -----------
# - Appelé depuis Tasker — pas depuis rz_start.sh. Pas de PID file.
# - Termux:API doit être actif (lancé par RZ_Init_Tasker au démarrage).
# - Le symlink ~/.termux/tasker/rz_autovoice_bridge.sh doit pointer ici.
# - pip install requests requis pour rz_ai_conversation.py (Gemini).
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (simplification — contexte STT délégué à Tasker/AutoVoice)
# DATE    : 2026-06-04
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
STT_DIR="$BASE_DIR/scripts/sensors/stt"

HANDLER="$STT_DIR/rz_stt_handler.sh"
AI_CONV="$STT_DIR/rz_ai_conversation.py"
LOG_FILE="$BASE_DIR/logs/autovoice_bridge.log"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_av() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [AV_BRIDGE] $1" >> "$LOG_FILE"
}

tts_say() {
    termux-tts-speak "$1" 2>/dev/null &
}

# =============================================================================
# VÉRIFICATION ARGUMENT
# =============================================================================

if [ -z "$1" ]; then
    log_av "ERREUR : aucun texte reçu — appel sans argument"
    exit 1
fi

ARG_RAW="$1"

# =============================================================================
# MODE ERROR: — échec de reconnaissance (profil RZ_RecFailed)
# Tasker appelle ce script avec "ERROR:recognition_failed"
# =============================================================================

if [[ "$ARG_RAW" == ERROR:* ]]; then
    ERROR_TYPE="${ARG_RAW#ERROR:}"
    log_av "Échec reconnaissance : $ERROR_TYPE"
    tts_say "Je n'ai pas compris."
    exit 0
fi

# =============================================================================
# MODE GEMINI: — bascule directe IA (profil RZ_NoMatch)
# Tasker appelle ce script avec "GEMINI:texte complet reconnu"
# Le texte est la phrase complète (%avcomm) car pas de commande matchée
# =============================================================================

if [[ "$ARG_RAW" == GEMINI:* ]]; then
    GEMINI_TEXT="${ARG_RAW#GEMINI:}"
    log_av "Bascule Gemini directe : '$GEMINI_TEXT'"
    if [ -f "$AI_CONV" ]; then
        python3 "$AI_CONV" "$GEMINI_TEXT"
    else
        log_av "WARN : rz_ai_conversation.py introuvable"
        tts_say "Je ne peux pas répondre, module IA absent."
    fi
    exit 0
fi

# =============================================================================
# MODE NORMAL — commande vocale standard
# Reçoit %avcommnofilter : texte SANS le préfixe "rz"
# AutoVoice a déjà retiré le wake word via Command Filter
# Normalisation : minuscules + espaces multiples supprimés
# =============================================================================

# Note : rz_stt_handler.sh fait aussi un sed 's/^rz //g' par sécurité
# — inoffensif si le texte ne commence pas par "rz"
TEXT_NORM=$(echo "$ARG_RAW" | tr '[:upper:]' '[:lower:]' | tr -s ' ' | xargs)

log_av "Commande reçue : '$ARG_RAW' → normalisé : '$TEXT_NORM'"

# Vérification handler
if [ ! -f "$HANDLER" ]; then
    log_av "ERREUR : handler introuvable : $HANDLER"
    tts_say "Erreur système vocal."
    exit 1
fi

# Appel handler → catalogue → VPIV → MQTT
bash "$HANDLER" "$TEXT_NORM"
EXIT_CODE=$?

log_av "Handler exit=$EXIT_CODE pour '$TEXT_NORM'"

# Exit 1 = commande inconnue du catalogue
# En mode normal ce cas est géré par Tasker profil RZ_NoMatch
# qui déclenche RZ_GeminiConv si %RZsttContext = Conv|Mixte
# Ici on se contente de logger — pas de double bascule Gemini
if [ $EXIT_CODE -eq 1 ]; then
    log_av "Commande inconnue '$TEXT_NORM' — RZ_NoMatch Tasker prendra le relais si contexte Conv/Mixte"
fi

exit 0