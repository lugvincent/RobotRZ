#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER: rz_build_dict.py
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_build_dict.py
# DESCRIPTION:
#    Ce script va lire le fichier rz_words.txt (généré précédemment
#    par rz_build_system.py et créer le fichier fr.dict
#    que PocketSphinx attend 
# FONCTIONS PRINCIPALES:
#    - generate_phonetics: Charge le lexique de référence,
#      lit les mots personnalisés, 
#      et génère le dictionnaire final en associant les phonétiques.
# ===============================================================

import os

# CONFIGURATION DES CHEMINS
STT_DIR = "/data/data/com.termux/files/home/scripts_RZ_SE/termux/scripts/sensors/stt"
LIB_PATH = os.path.join(STT_DIR, "lib_pocketsphinx")
WORDS_FILE = os.path.join(STT_DIR, "rz_words.txt")
REF_DICT = os.path.join(LIB_PATH, "lexique_referent.dict")
FINAL_DICT = os.path.join(LIB_PATH, "fr.dict")

def generate_phonetics():
    if not os.path.exists(WORDS_FILE):
        print(f"Erreur : {WORDS_FILE} n'existe pas. Lance d'abord le convertisseur CSV.")
        return

    # 1. Charger le dictionnaire de référence en mémoire
    print("Chargement du lexique de référence...")
    lexique = {}
    with open(REF_DICT, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split(maxsplit=1)
            if len(parts) == 2:
                lexique[parts[0].lower()] = parts[1]

    # 2. Lire tes mots personnalisés
    with open(WORDS_FILE, 'r', encoding='utf-8') as f:
        mes_mots = [line.strip().lower() for line in f if line.strip()]

    # 3. Créer le dictionnaire final
    mots_trouves = 0
    mots_manquants = []

    with open(FINAL_DICT, 'w', encoding='utf-8') as f:
        for mot in sorted(mes_mots):
            if mot in lexique:
                f.write(f"{mot} {lexique[mot]}\n")
                mots_trouves += 1
            else:
                mots_manquants.append(mot)
                # Option par défaut : phonétique basique (très approximatif)
                f.write(f"{mot} T B D\n") 

    print(f"--- FIN DE GÉNÉRATION ---")
    print(f"Mots intégrés avec succès : {mots_trouves}")
    if mots_manquants:
        print(f"ATTENTION : {len(mots_manquants)} mots n'ont pas été trouvés dans le lexique :")
        for m in mots_manquants:
            print(f" - {m}")
        print(f"Édite manuellement {FINAL_DICT} pour corriger les 'T B D'.")

if __name__ == "__main__":
    generate_phonetics()