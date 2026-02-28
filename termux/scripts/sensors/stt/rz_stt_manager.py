#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_stt_manager.py
# CHEMIN : ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_stt_manager.py
# ARTICULATION : 
#   - Lit global.json (mis à jour par rz_state-manager.sh) pour le contexte.
#   - Appelle rz_stt_handler.sh pour les commandes locales (Sphinx).
#   - Bascule vers rz_ai_conversation.py pour le mode conversation.
# =============================================================================

import os
import json
import subprocess
import sys
from pocketsphinx import LiveSpeech

# --- CONFIGURATION DES CHEMINS ---
BASE_DIR = "/data/data/com.termux/files/home/scripts_RZ_SE/termux"
STT_DIR = os.path.join(BASE_DIR, "scripts/sensors/stt")
CONFIG_PATH = os.path.join(STT_DIR, "stt_config.json")
LIB_PATH = os.path.join(STT_DIR, "lib_pocketsphinx")
GLOBAL_JSON = os.path.join(BASE_DIR, "config/global.json")
FIFO_IN = os.path.join(BASE_DIR, "fifo/fifo_termux_in")

def send_vpiv(trame):
    """Envoie une trame VPIV directement dans le circuit de données"""
    try:
        with open(FIFO_IN, 'w') as f:
            f.write(trame + '\n')
    except:
        pass

def get_stt_context():
    """Récupère le contexte calculé par le State-Manager"""
    try:
        with open(GLOBAL_JSON, 'r') as f:
            data = json.load(f)
            return data.get("STT", {}).get("context", "Inactif")
    except (FileNotFoundError, json.JSONDecodeError):
        return "Inactif"

def call_local_handler(text):
    """Appelle le handler Bash pour traiter la commande via le catalogue"""
    handler_path = os.path.join(STT_DIR, "rz_stt_handler.sh")
    # On récupère le code de sortie du script Bash
    result = subprocess.run(["bash", handler_path, text])
    return result.returncode # 0 = Succès (Trouvé), 1 = Inconnu

def call_cloud_conversation(text):
    """Placeholder pour l'envoi vers l'IA Cloud (Conversation)"""
    print(f"[@] BASCULE IA : Traitement de '{text}' via Cloud...")
    # Ici, tu appelleras ton futur script de conversation IA
    # Appel du script Python de conversation
    subprocess.run(["python3", "/data/data/com.termux/files/home/scripts_RZ_SE/termux/scripts/sensors/stt/rz_ai_conversation.py", text])

def run_listener():
    # 1. Vérification des fichiers de langue (Sécurité PC vs Smartphone)
    if not os.path.exists(os.path.join(LIB_PATH, "fr.dict")):
        print("[!] ERREUR : Modèles de langue introuvables. Mode simulation activé.")
        # Sur ton PC, tu peux simuler une détection ici si tu veux tester la logique

    # 2. Chargement config seuil/mot-clé
    try:
        with open(CONFIG_PATH, 'r') as f:
            cfg = json.load(f)
    except:
        cfg = {'threshold': 1e-20, 'keyphrase': 'rz'}

    # 3. Initialisation PocketSphinx
    speech = LiveSpeech(
        verbose=False,
        sampling_rate=16000,
        buffer_size=2048,
        hmm=os.path.join(LIB_PATH, "model-fr"),
        dict=os.path.join(LIB_PATH, "fr.dict"),
        kws_threshold=cfg['threshold'],
        keyphrase=cfg['keyphrase']
    )

    print(f"[@] L'OREILLE RZ : En veille (Mode contextuel actif)")

    # Signalement système : STT est prêt et écoute
    send_vpiv("$I:STT:state:*:1#")

    try:
        for phrase in speech:
            detected_text = str(phrase).lower()
            context = get_stt_context()
            
            print(f"[@] Entendu: '{detected_text}' | Contexte actuel: {context}")

            if context == "Inactif":
                print("[#] STT ignoré (Robot Inactif/Arrêt)")
                continue

            # LOGIQUE DE BASCULE (Selon ton Flowchart)
            
            if context == "Cmde":
                # Mode mouvement : Uniquement Sphinx local
                call_local_handler(detected_text)

            elif context == "Conv":
                # Mode veille immobile : Uniquement Conversation IA
                call_cloud_conversation(detected_text)

            elif context == "Mixte":
                # Mode Autonome/Fixe : Sphinx en premier, si échec -> IA
                status = call_local_handler(detected_text)
                if status != 0: # Si rz_stt_handler.sh retourne 1 (non trouvé)
                    print("[?] Commande locale inconnue, tentative via IA...")
                    call_cloud_conversation(detected_text)
    
    finally:
        # Signalement système : STT s'arrête
        send_vpiv("$I:STT:state:*:0#")

if __name__ == "__main__":
    run_listener()