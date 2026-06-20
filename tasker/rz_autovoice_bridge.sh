#!/bin/bash
# =============================================================================
# SCRIPT  : rz_autovoice_bridge.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_autovoice_bridge.sh
#
# OBJECTIF
# --------
# Pont entre AutoVoice (Tasker) et le handler vocal RZ.
# Recoit le texte reconnu par AutoVoice via Termux:Tasker (deja debarrasse
# du prefixe "rz" par Rz_Traite_Ecoute), et route selon STT.context (lu
# directement dans global.json) vers le catalogue (rz_stt_handler.sh) et/ou
# la conversation IA (rz_ai_conversation.py).
#
# LOGIQUE DE ROUTAGE -- OPTION B (un seul mot-cle "rz", juin 2026)
# -------------------------------------------------------------------
#   STT.context = Inactif
#     -> TTS "contexte vocal non actif" + log. Rien d'autre.
#
#   STT.context = Cmde
#     -> catalogue uniquement (rz_stt_handler.sh)
#     -> exit=1 (commande inconnue) -> TTS "je n'ai pas compris"
#     -> PAS de bascule Gemini (securite + rapidite en deplacement)
#
#   STT.context = Conv
#     -> Gemini direct (rz_ai_conversation.py), SAUF :
#        exception "stop"/"top" -- routees vers le catalogue (URGENCY_STOP)
#        meme en conversation, l'arret d'urgence doit toujours fonctionner.
#
#   STT.context = Mixte
#     -> catalogue en priorite (rz_stt_handler.sh)
#     -> exit=1 (commande inconnue) -> bascule SILENCIEUSE vers Gemini
#        (pas de TTS d'erreur intermediaire -- transition transparente)
#
#   STT.context vide ou absent (demarrage avant 1er calcul SP/SE)
#     -> TTS demandant a l'utilisateur de preciser le contexte
#        (rz commande / rz conversation / rz mixte)
#
# MODES SPECIAUX (compatibilite, appel direct sans passer par le contexte)
# ---------------------------------------------------------------------------
#   GEMINI:texte   -> bascule directe IA, quel que soit STT.context
#                      (reserve a un appel explicite externe, ex: RZ_IA_Conv)
#   ERROR:type     -> echec de reconnaissance AutoVoice -> TTS "pas compris"
#
# ARTICULATION
# ------------
#   Appele par : Rz_Traite_Ecoute (task62, Tasker) via Termux:Tasker
#   Lit        : config/global.json              (STT.context)
#   Appelle    : rz_stt_handler.sh                (catalogue -> VPIV)
#   Appelle    : rz_ai_conversation.py            (Gemini)
#   Log        : logs/autovoice_bridge.log
#
# PRECAUTIONS
# -----------
# - Appele depuis Tasker -- pas depuis rz_start.sh. Pas de PID file.
# - Termux:API doit etre actif (lance par RZ_Init_Tasker au demarrage).
# - Doit etre un fichier REGULIER dans tasker/ (jamais un symlink --
#   Termux:Tasker refuse les symlinks apres reboot Android).
# - pip install requests requis pour rz_ai_conversation.py (Gemini).
# - L'exception stop/top en contexte Conv ne couvre QUE ces deux mots exacts
#   (apres normalisation minuscule) -- pas de regex large, pour eviter qu'une
#   phrase de conversation legitime contenant "stop" soit mal routee
#   (ex: "raconte-moi l'histoire du stop and go" -- cas limite accepte).
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (Option B : routage par STT.context unique mot-cle "rz",
#                  exception stop/top en Conv, bascule Mixte->Gemini silencieuse)
# DATE    : 2026-06-19
# =============================================================================

# =============================================================================
# CONFIGURATION
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
STT_DIR="$BASE_DIR/scripts/sensors/stt"

HANDLER="$STT_DIR/rz_stt_handler.sh"
AI_CONV="$STT_DIR/rz_ai_conversation.py"
GLOBAL_JSON="$BASE_DIR/config/global.json"
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

call_handler() {
    local text="$1"
    if [ ! -f "$HANDLER" ]; then
        log_av "ERREUR : handler introuvable : $HANDLER"
        tts_say "Erreur systeme vocal."
        return 1
    fi
    bash "$HANDLER" "$text"
    return $?
}

call_gemini() {
    local text="$1"
    log_av "Appel Gemini : '$text'"
    if [ -f "$AI_CONV" ]; then
        python3 "$AI_CONV" "$text"
    else
        log_av "WARN : rz_ai_conversation.py introuvable"
        tts_say "Je ne peux pas repondre, module IA absent."
    fi
}

# =============================================================================
# VERIFICATION ARGUMENT
# =============================================================================

if [ -z "$1" ]; then
    log_av "ERREUR : aucun texte recu -- appel sans argument"
    exit 1
fi

ARG_RAW="$1"

# =============================================================================
# MODE ERROR: -- echec de reconnaissance
# =============================================================================

if [[ "$ARG_RAW" == ERROR:* ]]; then
    ERROR_TYPE="${ARG_RAW#ERROR:}"
    log_av "Echec reconnaissance : $ERROR_TYPE"
    tts_say "Je n'ai pas compris."
    exit 0
fi

# =============================================================================
# MODE GEMINI: -- bascule directe IA explicite (appel externe, ex: RZ_IA_Conv)
# Reste disponible independamment de STT.context pour des usages futurs.
# =============================================================================

if [[ "$ARG_RAW" == GEMINI:* ]]; then
    GEMINI_TEXT="${ARG_RAW#GEMINI:}"
    log_av "Bascule Gemini directe (appel explicite) : '$GEMINI_TEXT'"
    call_gemini "$GEMINI_TEXT"
    exit 0
fi

# =============================================================================
# MODE NORMAL -- routage selon STT.context
# =============================================================================

TEXT_NORM=$(echo "$ARG_RAW" | tr '[:upper:]' '[:lower:]' | tr -s ' ' | xargs)

log_av "Texte recu : '$ARG_RAW' -> normalise : '$TEXT_NORM'"

# --- Lecture de STT.context depuis global.json ---
STT_CONTEXT=""
if [ -f "$GLOBAL_JSON" ]; then
    STT_CONTEXT=$(jq -r '.STT.context // empty' "$GLOBAL_JSON" 2>/dev/null)
fi

log_av "STT.context lu : '$STT_CONTEXT'"

# =============================================================================
# CAS : CONTEXTE VIDE OU ABSENT (avant 1er calcul SP/SE)
# =============================================================================

if [ -z "$STT_CONTEXT" ]; then
    log_av "STT.context vide -- demande de precision a l'utilisateur"
    tts_say "Contexte vocal non défini. Dites rz commande, rz conversation, ou rz mixte."
    exit 0
fi

# =============================================================================
# CAS : INACTIF
# =============================================================================

if [ "$STT_CONTEXT" = "Inactif" ]; then
    log_av "STT.context=Inactif -- aucune action"
    exit 0
fi

# =============================================================================
# CAS : CMDE -- catalogue uniquement, pas de bascule Gemini
# =============================================================================

if [ "$STT_CONTEXT" = "Cmde" ]; then
    call_handler "$TEXT_NORM"
    EXIT_CODE=$?
    log_av "Contexte Cmde -- handler exit=$EXIT_CODE pour '$TEXT_NORM'"
    if [ $EXIT_CODE -eq 1 ]; then
        log_av "Commande inconnue en contexte Cmde -- TTS erreur (pas de Gemini)"
        tts_say "Je n'ai pas compris."
    fi
    exit 0
fi

# =============================================================================
# CAS : CONV -- Gemini direct, sauf exception stop/top (securite)
# =============================================================================

if [ "$STT_CONTEXT" = "Conv" ]; then
    if [ "$TEXT_NORM" = "stop" ] || [ "$TEXT_NORM" = "top" ]; then
        log_av "Contexte Conv -- exception securite '$TEXT_NORM' -- route vers catalogue"
        call_handler "$TEXT_NORM"
        exit 0
    fi
    log_av "Contexte Conv -- route direct vers Gemini"
    call_gemini "$TEXT_NORM"
    exit 0
fi

# =============================================================================
# CAS : MIXTE -- catalogue priorite, bascule Gemini silencieuse sur exit=1
# =============================================================================

if [ "$STT_CONTEXT" = "Mixte" ]; then
    call_handler "$TEXT_NORM"
    EXIT_CODE=$?
    log_av "Contexte Mixte -- handler exit=$EXIT_CODE pour '$TEXT_NORM'"
    if [ $EXIT_CODE -eq 1 ]; then
        log_av "Commande inconnue en contexte Mixte -- bascule Gemini silencieuse"
        call_gemini "$TEXT_NORM"
    fi
    exit 0
fi

# =============================================================================
# CAS : VALEUR INATTENDUE
# =============================================================================

log_av "WARN : STT.context inattendu : '$STT_CONTEXT' -- aucune action"
exit 0
