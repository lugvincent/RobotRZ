#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_stt_manager.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_stt_manager.py
#
# OBJECTIF
# --------
# Daemon principal du module vocal côté SE.
# Mode KWS (Keyword Spotting) : détecte "rz" puis capture la commande.
# JSGF non supporté par PocketSphinx 5.1.0 — KWS confirmé fonctionnel.
#
# USAGE
# -----
# Dire "rz [commande]" d'une seule traite sans pause.
# Exemple : "rz stop" / "rz avance" / "rz mode veille"
#
# CONTEXTES STT (calculés par rz_state-manager.sh dans global.json)
# -----------------------------------------------------------------
# Inactif : modeRZ=0/5 ou typePtge incompatible → ignoré
# Cmde    : robot en mouvement → catalogue local uniquement
# Conv    : modeRZ=1 veille → Gemini prioritaire
# Mixte   : modeRZ=2/3/4 à l'arrêt → catalogue puis Gemini si inconnu
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.5  (retour KWS, debug phrase, fix path stt_config, lm=false)
# DATE    : 2026-05-27
# =============================================================================

import os
import json
import subprocess
import sys

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

BASE_DIR = "/data/data/com.termux/files/home/robotRZ-smartSE/termux"
STT_DIR  = os.path.join(BASE_DIR, "scripts/sensors/stt")

# stt_config.json généré par check_config.sh dans config/sensors/
CONFIG_PATH = os.path.join(BASE_DIR, "config/sensors/stt_config.json")
LIB_PATH    = os.path.join(STT_DIR,  "lib_pocketsphinx")
GLOBAL_JSON = os.path.join(BASE_DIR, "config/global.json")
FIFO_IN     = os.path.join(BASE_DIR, "fifo/fifo_termux_in")
HANDLER_PATH = os.path.join(STT_DIR, "rz_stt_handler.sh")
AI_CONV_PATH = os.path.join(STT_DIR, "rz_ai_conversation.py")
HMM_PATH    = os.path.join(LIB_PATH, "model-fr")
DICT_PATH   = os.path.join(LIB_PATH, "fr.dict")

# =============================================================================
# UTILITAIRES
# =============================================================================

def send_vpiv(trame: str):
    try:
        with open(FIFO_IN, 'w') as f:
            f.write(trame + '\n')
    except (OSError, IOError):
        pass


def get_stt_context() -> str:
    try:
        with open(GLOBAL_JSON, 'r') as f:
            data = json.load(f)
            return data.get("STT", {}).get("context", "Inactif")
    except (FileNotFoundError, json.JSONDecodeError):
        return "Inactif"


def load_config() -> dict:
    defaults = {
        "threshold": 1e-20,
        "keyphrase": "rz",
        "lang": "fr"
    }
    try:
        with open(CONFIG_PATH, 'r') as f:
            cfg = json.load(f)
            # stt_config.json est encapsulé dans {"stt": {...}}
            return {**defaults, **cfg.get("stt", cfg)}
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"[!] stt_config.json absent ({e}) — défauts appliqués")
        return defaults

# =============================================================================
# APPELS AUX SOUS-SYSTÈMES
# =============================================================================

def call_local_handler(text: str) -> int:
    if not os.path.exists(HANDLER_PATH):
        print(f"[!] Handler introuvable : {HANDLER_PATH}")
        return 1
    result = subprocess.run(["bash", HANDLER_PATH, text])
    return result.returncode


def call_cloud_conversation(text: str):
    if not os.path.exists(AI_CONV_PATH):
        print(f"[!] Script IA introuvable : {AI_CONV_PATH}")
        return
    print(f"[@] BASCULE IA Cloud : '{text}' → Gemini ...")
    subprocess.run(["python3", AI_CONV_PATH, text])

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================

def run_listener():

    # Vérification prérequis
    if not os.path.exists(HMM_PATH) or not os.path.exists(DICT_PATH):
        print("[!] ERREUR : modèles PocketSphinx introuvables.")
        print(f"    Attendus : {HMM_PATH} et {DICT_PATH}")
        sys.exit(1)

    try:
        from pocketsphinx import LiveSpeech
    except ImportError:
        print("[!] ERREUR : pocketsphinx non installé.")
        sys.exit(1)

    cfg = load_config()
    print(f"[@] Config STT : keyphrase='{cfg['keyphrase']}' threshold={cfg['threshold']}")

    speech = LiveSpeech(
        lm=False, # désactive le modèle de langage (pas de phrases hors grammaire et l'anglais par défaut)
        sampling_rate=16000,
        buffer_size=2048,
        hmm=HMM_PATH,
        dict=DICT_PATH,
        kws_threshold=cfg['threshold'],
        keyphrase=cfg['keyphrase']
    )

    print("[@] L'OREILLE RZ : en veille (mode KWS actif) ...")
    print("[@] Dites 'rz [commande]' d'une seule traite — ex: 'rz stop'")

    send_vpiv("$I:STT:state:*:1#")

    try:
        for phrase in speech:
            detected_text = str(phrase).lower().strip()

            # DEBUG : log brut pour comprendre ce que PocketSphinx retourne
            print(f"[@] DEBUG brut : '{detected_text}' (longueur={len(detected_text)})")

            if not detected_text:
                continue

            context = get_stt_context()
            print(f"[@] Reconnu : '{detected_text}' | Contexte : {context}")

            if context == "Inactif":
                print("[#] STT ignoré (contexte Inactif)")
                continue

            if context == "Cmde":
                call_local_handler(detected_text)

            elif context == "Conv":
                exit_code = call_local_handler(detected_text)
                if exit_code != 0:
                    call_cloud_conversation(detected_text)

            elif context == "Mixte":
                exit_code = call_local_handler(detected_text)
                if exit_code != 0:
                    print("[?] Commande inconnue → tentative IA Cloud ...")
                    call_cloud_conversation(detected_text)

            else:
                print(f"[!] Contexte inconnu '{context}' — ignoré")

    except KeyboardInterrupt:
        print("\n[@] Arrêt demandé (Ctrl+C)")

    finally:
        send_vpiv("$I:STT:state:*:0#")
        print("[@] Module STT arrêté.")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    run_listener()