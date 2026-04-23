#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_ai_conversation.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_ai_conversation.py
#
# OBJECTIF
# --------
# Moteur de conversation Cloud utilisant l'API Google Gemini 1.5 Flash.
# Intervient en bascule quand le catalogue local PocketSphinx ne reconnaît
# pas la phrase (rz_stt_handler.sh retourne exit 1).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Reçoit la phrase reconnue en argument (sys.argv[1]).
# 2. Construit un prompt avec la personnalité de RZ (robot compagnon).
# 3. Interroge Gemini 1.5 Flash via l'API REST (timeout 10s).
# 4. Joue la réponse via termux-tts-speak.
#
# ARTICULATION
# ------------
#   rz_stt_manager.py → (bascule IA, context=Conv/Mixte)
#                     → rz_ai_conversation.py "phrase utilisateur"
#                     → API Gemini → réponse texte → termux-tts-speak
#
# PERSONNALITÉ RZ (SYSTEM PROMPT)
# --------------------------------
# RZ est un robot compagnon amical et intelligent.
# Réponses courtes (1 phrase) pour limiter la durée TTS.
# Pas de trame VPIV générée ici : la conversation est libre,
# sans commande robot déclenchée côté SE.
# Si une commande doit être exécutée, elle doit passer par le catalogue
# local (rz_stt_handler.sh), pas par l'IA Cloud.
#
# CLÉS API
# --------
# La clé Gemini est lue dans config/keys.json :
#   { "GEMINI_API_KEY": "AIza..." }
# Ne jamais mettre la clé en dur dans ce fichier.
#
# PRÉCAUTIONS
# -----------
# - Pas de flux audio envoyé au cloud : seulement le texte déjà reconnu.
# - En cas de timeout ou d'erreur réseau : réponse de fallback locale TTS.
# - Confidentialité : le texte de la phrase est envoyé à Google.
#   Informer l'utilisateur si nécessaire.
#
# DÉPENDANCES
# -----------
#   requests (pip install requests)
#   termux-tts-speak (Termux API)
#
# UTILISATION
# -----------
#   python3 rz_ai_conversation.py "bonjour comment tu vas"
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.2  (headers complets, fallback TTS amélioré, timeout explicite)
# DATE    : 2026-03-05
# =============================================================================

import sys
import json
import requests
import subprocess

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

BASE_DIR    = "/data/data/com.termux/files/home/robotRZ-smartSE/termux"
CONFIG_KEYS = f"{BASE_DIR}/config/keys.json"
LOG_FILE    = f"{BASE_DIR}/logs/ai_conversation.log"

# URL API Gemini 1.5 Flash (rapide, gratuit avec quota)
GEMINI_URL_TEMPLATE = (
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-1.5-flash:generateContent?key={api_key}"
)

# Timeout requête HTTP (secondes)
REQUEST_TIMEOUT = 10

# Réponse de fallback si API inaccessible
FALLBACK_RESPONSE = "J'ai un problème de connexion à mon cerveau cloud."

# =============================================================================
# UTILITAIRES
# =============================================================================

def log_ai(message: str):
    """Journalise dans ai_conversation.log avec horodatage."""
    timestamp = subprocess.getoutput("date '+%Y-%m-%d %H:%M:%S'")
    try:
        with open(LOG_FILE, "a") as f:
            f.write(f"[{timestamp}] {message}\n")
    except OSError:
        pass  # Log non critique


def get_api_key() -> str | None:
    """
    Lit la clé API Gemini depuis config/keys.json.
    Retourne None si absent ou invalide.
    """
    try:
        with open(CONFIG_KEYS, 'r') as f:
            return json.load(f).get("GEMINI_API_KEY")
    except (FileNotFoundError, json.JSONDecodeError, KeyError) as e:
        log_ai(f"Erreur lecture clé API : {e}")
        return None


def speak(text: str):
    """Lance termux-tts-speak avec le texte fourni."""
    subprocess.run(["termux-tts-speak", text])

# =============================================================================
# APPEL API GEMINI
# =============================================================================

def ask_gemini(text_input: str) -> str:
    """
    Envoie la phrase à Gemini 1.5 Flash et retourne la réponse texte.
    Retourne FALLBACK_RESPONSE en cas d'erreur réseau ou API.
    """
    api_key = get_api_key()
    if not api_key:
        log_ai("Clé API absente — impossible d'interroger Gemini")
        return "Désolé, ma clé de communication est absente."

    url = GEMINI_URL_TEMPLATE.format(api_key=api_key)

    # System prompt : personnalité RZ, réponse courte pour TTS
    prompt = (
        "Tu es RZ, un robot compagnon amical et intelligent. "
        "Réponds de manière concise, en une seule phrase courte. "
        f"L'utilisateur dit : {text_input}"
    )

    headers = {'Content-Type': 'application/json'}
    payload = {
        "contents": [{"parts": [{"text": prompt}]}]
    }

    try:
        response = requests.post(
            url,
            headers=headers,
            json=payload,
            timeout=REQUEST_TIMEOUT
        )
        response.raise_for_status()
        data = response.json()

        # Extraction réponse texte
        answer = data['candidates'][0]['content']['parts'][0]['text']
        return answer.strip()

    except requests.exceptions.Timeout:
        log_ai("Timeout API Gemini")
        return "Je mets trop de temps à réfléchir, réessaie."

    except requests.exceptions.ConnectionError:
        log_ai("Pas de connexion réseau")
        return "Je n'ai pas de connexion internet pour l'instant."

    except Exception as e:
        log_ai(f"Erreur API Gemini : {e}")
        return FALLBACK_RESPONSE

# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage : rz_ai_conversation.py 'phrase à traiter'")
        sys.exit(1)

    user_phrase = sys.argv[1]
    log_ai(f"Question : {user_phrase}")

    # Interrogation Gemini
    reponse = ask_gemini(user_phrase)
    log_ai(f"Réponse  : {reponse}")

    # Synthèse vocale
    speak(reponse)


if __name__ == "__main__":
    main()