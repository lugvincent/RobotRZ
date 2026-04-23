#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_stt_manager.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_stt_manager.py
#
# OBJECTIF
# --------
# Daemon principal du module vocal côté SE.
# Écoute en continu le microphone via PocketSphinx (offline, mode KWS).
# Bascule vers le handler local (Sphinx) ou l'IA Cloud (Gemini) selon
# le contexte calculé par rz_state-manager.sh.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Lecture du contexte STT depuis global.json (mis à jour par state-manager).
# 2. Boucle PocketSphinx : détection du mot-clé "rz" (Wake Word).
# 3. Selon le contexte :
#    - Inactif : ignore (robot arrêté ou mode incompatible)
#    - Cmde    : appel rz_stt_handler.sh uniquement (robot en mouvement)
#    - Conv    : appel rz_ai_conversation.py uniquement (veille immobile)
#    - Mixte   : Sphinx en priorité, bascule IA si exit 1 (inconnu)
# 4. Signalement VPIV du démarrage/arrêt STT via fifo_termux_in.
#
# BASCULE LOCALE / CLOUD
# ----------------------
# Local  (rz_stt_handler.sh) :
#   - Reconnaît les commandes du catalogue (stt_catalog.json)
#   - Retourne exit 0 si trouvé, exit 1 si inconnu
#   - Envoie capsule VPIV $A:STT:intent:*:{...}# dans fifo_termux_in
# Cloud  (rz_ai_conversation.py) :
#   - Interroge Gemini 1.5 Flash
#   - Réponse TTS via termux-tts-speak
#   - Pas de trame VPIV générée (conversation libre)
#
# INTERACTIONS
# ------------
#   Lit       : config/global.json        (STT.context — mis à jour par state-manager)
#   Lit       : scripts/sensors/stt/stt_config.json   (threshold, keyphrase)
#   Lit       : scripts/sensors/stt/lib_pocketsphinx/ (modèles fr)
#   Écrit     : fifo/fifo_termux_in       (VPIV état STT : on/off)
#   Appelle   : rz_stt_handler.sh         (catalogue local Sphinx)
#   Appelle   : rz_ai_conversation.py     (IA Cloud Gemini)
#
# PRÉREQUIS
# ---------
#   pip install pocketsphinx  (ou pkg install python-pocketsphinx sous Termux)
#   Modèles français dans lib_pocketsphinx/ : model-fr/, fr.dict
#   Clé API Gemini dans config/keys.json (pour mode Conv/Mixte)
#
# PRÉCAUTIONS
# -----------
# - Ne jamais envoyer de flux audio continu vers le cloud (confidentialité).
#   PocketSphinx tourne en local pour la détection du wake word.
# - Si les modèles sont absents (développement PC), le script s'arrête
#   proprement avec un message explicite.
# - global.json peut être absent au démarrage : contexte = "Inactif" par défaut.
#
# DÉPENDANCES
# -----------
#   pocketsphinx, rz_stt_handler.sh, rz_ai_conversation.py
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.1  (headers complets, gestion absence modèles, signalement VPIV)
# DATE    : 2026-03-05
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

# Fichiers de configuration
CONFIG_PATH  = os.path.join(STT_DIR,  "stt_config.json")
LIB_PATH     = os.path.join(STT_DIR,  "lib_pocketsphinx")
GLOBAL_JSON  = os.path.join(BASE_DIR, "config/global.json")

# FIFO pour signalement système
FIFO_IN = os.path.join(BASE_DIR, "fifo/fifo_termux_in")

# Scripts associés
HANDLER_PATH  = os.path.join(STT_DIR, "rz_stt_handler.sh")
AI_CONV_PATH  = os.path.join(STT_DIR, "rz_ai_conversation.py")

# Fichiers modèles PocketSphinx
HMM_PATH      = os.path.join(LIB_PATH, "model-fr")
DICT_PATH     = os.path.join(LIB_PATH, "fr.dict")

# =============================================================================
# UTILITAIRES
# =============================================================================

def send_vpiv(trame: str):
    """
    Envoie une trame VPIV dans fifo_termux_in.
    Non-bloquant : si le FIFO est absent ou plein, on ignore silencieusement.
    """
    try:
        with open(FIFO_IN, 'w') as f:
            f.write(trame + '\n')
    except (OSError, IOError):
        pass  # FIFO absent ou lecteur non prêt — ignoré


def get_stt_context() -> str:
    """
    Lit le contexte STT calculé par rz_state-manager.sh dans global.json.
    Valeurs possibles : Inactif | Cmde | Conv | Mixte
    Retourne 'Inactif' en cas d'absence ou d'erreur de lecture.
    """
    try:
        with open(GLOBAL_JSON, 'r') as f:
            data = json.load(f)
            return data.get("STT", {}).get("context", "Inactif")
    except (FileNotFoundError, json.JSONDecodeError):
        return "Inactif"


def load_config() -> dict:
    """
    Charge stt_config.json.
    Retourne la config par défaut si absent.
    """
    defaults = {
        "threshold": 1e-20,
        "keyphrase": "rz",
        "lang": "fr"
    }
    try:
        with open(CONFIG_PATH, 'r') as f:
            cfg = json.load(f)
            # Fusion avec les défauts pour les clés manquantes
            return {**defaults, **cfg}
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"[!] stt_config.json absent ou invalide ({e}) — défauts appliqués")
        return defaults

# =============================================================================
# APPELS AUX SOUS-SYSTÈMES
# =============================================================================

def call_local_handler(text: str) -> int:
    """
    Appelle rz_stt_handler.sh avec le texte reconnu.
    Retourne le code de sortie :
      0 = commande trouvée et traitée
      1 = commande inconnue → bascule IA possible
    """
    if not os.path.exists(HANDLER_PATH):
        print(f"[!] Handler introuvable : {HANDLER_PATH}")
        return 1  # Traiter comme inconnu pour permettre la bascule IA

    result = subprocess.run(["bash", HANDLER_PATH, text])
    return result.returncode


def call_cloud_conversation(text: str):
    """
    Appelle rz_ai_conversation.py pour traiter la phrase via Gemini.
    La réponse est jouée directement par TTS (géré dans le script Python).
    """
    if not os.path.exists(AI_CONV_PATH):
        print(f"[!] Script IA introuvable : {AI_CONV_PATH}")
        return

    print(f"[@] BASCULE IA Cloud : traitement de '{text}' via Gemini ...")
    subprocess.run(["python3", AI_CONV_PATH, text])

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================

def run_listener():
    """
    Démarre la boucle d'écoute PocketSphinx.
    Traite chaque phrase détectée selon le contexte courant.
    """

    # --- Vérification présence des modèles de langue ---
    if not os.path.exists(HMM_PATH) or not os.path.exists(DICT_PATH):
        print("[!] ERREUR : Modèles PocketSphinx introuvables.")
        print(f"    Attendus dans : {LIB_PATH}")
        print("    Vérifier : model-fr/ et fr.dict")
        sys.exit(1)

    # --- Import tardif (évite crash si non installé sur PC de développement) ---
    try:
        from pocketsphinx import LiveSpeech
    except ImportError:
        print("[!] ERREUR : pocketsphinx non installé.")
        print("    Termux : pip install pocketsphinx")
        sys.exit(1)

    # --- Chargement configuration ---
    cfg = load_config()
    print(f"[@] Config STT : keyphrase='{cfg['keyphrase']}' "
          f"threshold={cfg['threshold']} lang={cfg['lang']}")

    # --- Initialisation PocketSphinx ---
    speech = LiveSpeech(
        verbose=False,
        sampling_rate=16000,
        buffer_size=2048,
        hmm=HMM_PATH,
        dict=DICT_PATH,
        kws_threshold=cfg['threshold'],
        keyphrase=cfg['keyphrase']
    )

    print("[@] L'OREILLE RZ : en veille (mode KWS actif) ...")

    # --- Signalement VPIV : STT démarré ---
    send_vpiv("$I:STT:state:*:1#")

    try:
        for phrase in speech:
            detected_text = str(phrase).lower().strip()

            # Lecture du contexte à chaque détection (peut changer en cours d'exécution)
            context = get_stt_context()

            print(f"[@] Entendu : '{detected_text}' | Contexte : {context}")

            # --- FILTRE CONTEXTE ---
            if context == "Inactif":
                # Robot arrêté ou pilotage non vocal : on ignore
                print("[#] STT ignoré (contexte Inactif)")
                continue

            # --- AIGUILLAGE SELON CONTEXTE ---

            if context == "Cmde":
                # Robot en mouvement : catalogue local uniquement (pas d'IA)
                call_local_handler(detected_text)

            elif context == "Conv":
                # Veille immobile : conversation IA uniquement (pas de catalogue)
                call_cloud_conversation(detected_text)

            elif context == "Mixte":
                # Autonome / Fixe stoppé : Sphinx en priorité, IA si inconnu
                exit_code = call_local_handler(detected_text)
                if exit_code != 0:
                    # exit 1 = commande inconnue du catalogue → bascule IA
                    print("[?] Commande inconnue localement → tentative IA Cloud ...")
                    call_cloud_conversation(detected_text)

            else:
                # Contexte inattendu : log et ignore
                print(f"[!] Contexte inconnu '{context}' — trame ignorée")

    except KeyboardInterrupt:
        print("\n[@] Arrêt demandé (Ctrl+C)")

    finally:
        # --- Signalement VPIV : STT arrêté ---
        send_vpiv("$I:STT:state:*:0#")
        print("[@] Module STT arrêté.")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    run_listener()