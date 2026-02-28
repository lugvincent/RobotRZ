#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_ai_conversation.py
# CHEMIN : ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_ai_conversation.py
# DESCRIPTION : Moteur de conversation "Cloud" utilisant Google Gemini.
#               Intervient quand le catalogue local (Sphinx) ne comprend pas.
# =============================================================================

import sys
import json
import requests
import subprocess

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR = "/data/data/com.termux/files/home/scripts_RZ_SE/termux"
# Correction ici : on pointe vers le bon fichier de config
CONFIG_KEYS = f"{BASE_DIR}/config/keys.json"
LOG_FILE = f"{BASE_DIR}/logs/ai_conversation.log"

def log_ai(message):
    with open(LOG_FILE, "a") as f:
        f.write(f"[{subprocess.getoutput('date +%H:%M:%S')}] {message}\n")

def get_api_key():
    try:
        with open(CONFIG_KEYS, 'r') as f:
            return json.load(f).get("GEMINI_API_KEY")
    except Exception as e:
        log_ai(f"Erreur lecture clé API : {e}")
        return None

def ask_gemini(text_input):
    api_key = get_api_key()
    if not api_key:
        return "Désolé, ma clé de communication est absente."

    # URL pour le modèle Gemini 1.5 Flash (rapide et gratuit)
    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={api_key}"
    
    # SYSTEM PROMPT : On définit la personnalité de RZ
    # On lui demande d'être bref car le TTS (synthèse vocale) peut être long sinon.
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
        response = requests.post(url, headers=headers, json=payload, timeout=10)
        response.raise_for_status()
        data = response.json()
        
        # Extraction de la réponse texte
        answer = data['candidates'][0]['content']['parts'][0]['text']
        return answer.strip()
    except Exception as e:
        log_ai(f"Erreur API Gemini : {e}")
        return "J'ai un problème de connexion à mon cerveau cloud."

def main():
    if len(sys.argv) < 2:
        print("Usage: rz_ai_conversation.py 'phrase à traiter'")
        sys.exit(1)

    user_phrase = sys.argv[1]
    log_ai(f"Question reçue : {user_phrase}")

    # 1. On interroge l'IA
    reponse = ask_gemini(user_phrase)
    log_ai(f"Réponse IA : {reponse}")

    # 2. On fait parler le robot via Termux-TTS
    # On utilise subprocess pour appeler la commande système
    subprocess.run(["termux-tts-speak", reponse])

if __name__ == "__main__":
    main()
    