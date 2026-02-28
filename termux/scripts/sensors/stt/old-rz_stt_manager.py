#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER: rz_stt_manager.py
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_stt_manager.py
# DESCRIPTION:
#    Moteur de veille vocale basé sur PocketSphinx. Écoute en continu le mot-clé
#    défini dans stt_config.json. Lorsqu'il est détecté, il transmet la phrase
#    au processeur de commandes.
# FONCTIONS PRINCIPALES:
#    - load_config: Charge les paramètres dynamiques (seuil, mot-clé).
#    - run_listener: Boucle infinie d'écoute acoustique.
#    - l'erreur signalé sur pocketsphinx car je ne suis pas sur le smartphone,
#       mais sur mon PC, et je n'ai pas les modèles de langue français installés. 
#       Cependant, le code est conçu pour fonctionner sur un smartphone avec 
#       les modèles appropriés.Donc pas de pb de ce côté là.
# =============================================================================

import os
import json
import subprocess
from pocketsphinx import LiveSpeech

# --- CONFIGURATION DES CHEMINS ---
STT_DIR = "/data/data/com.termux/files/home/scripts_RZ_SE/termux/scripts/sensors/stt"
CONFIG_PATH = os.path.join(STT_DIR, "stt_config.json")
LIB_PATH = os.path.join(STT_DIR, "lib_pocketsphinx")

def load_config():
    """Charge la configuration générée par original_init.sh"""
    with open(CONFIG_PATH, 'r') as f:
        return json.load(f)

def run_listener():
    cfg = load_config()
    
    # Initialisation de l'écouteur
    # Note : hmm, dict et keyphrase sont critiques pour la performance sur Doro 8080
    speech = LiveSpeech(
        verbose=False,
        sampling_rate=16000,
        buffer_size=2048,
        hmm=os.path.join(LIB_PATH, "model-fr"),
        dict=os.path.join(LIB_PATH, "fr.dict"),
        kws_threshold=cfg['threshold'],
        keyphrase=cfg['keyphrase']
    )

    print(f"[@] RZ_STT_MANAGER: En veille (Mot-clé: '{cfg['keyphrase']}')")

    for phrase in speech:
        detected_text = str(phrase).lower()
        print(f"[@] Détecté: {detected_text}")
        
        # --- APPEL DU PROCESSEUR ---
        # On passe la phrase au script shell qui gère le catalogue JSON
        try:
            subprocess.run(["bash", os.path.join(STT_DIR, "rz_stt_handler.sh"), detected_text])
        except Exception as e:
            print(f"[!] Erreur lors de l'appel du processeur: {e}")

if __name__ == "__main__":
    # TODO: Ajouter ici une vérification de l'existence des fichiers modèles avant lancement
    run_listener()