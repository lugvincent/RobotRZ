#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_build_system.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_build_system.py
#
# OBJECTIF
# --------
# Script de construction du système STT.
# Lit le catalogue CSV (Table D) et génère les deux fichiers nécessaires
# au fonctionnement de la chaîne vocal locale :
#   1. stt_catalog.json : dictionnaire de commandes pour rz_stt_handler.sh
#   2. rz_words.txt     : liste de mots pour rz_build_dict.py (PocketSphinx)
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Pour chaque ligne du CSV :
#   - Extrait les synonymes (Alias) → liste pour PocketSphinx
#   - Construit l'objet commande (id, traitement, vpiv_parts, alias_tts, ...)
#   - Accumule les mots uniques pour le dictionnaire phonétique
#
# ARTICULATION
# ------------
#   table_d_catalogue.csv → rz_build_system.py → stt_catalog.json
#                                              → rz_words.txt
#   rz_words.txt          → rz_build_dict.py  → lib_pocketsphinx/fr.dict
#   stt_catalog.json      → rz_stt_handler.sh  (reconnaissance commandes)
#   lib_pocketsphinx/     → rz_stt_manager.py  (moteur PocketSphinx)
#
# FORMAT CSV ATTENDU (séparateur ;)
# ----------------------------------
# Colonnes obligatoires :
#   ID, Traitement, CAT, Destinataire, moteurActif, Alias, ID_fils,
#   VAR, PROP, INSTANCE, VALEUR, Alias-TTS
#
# FORMAT JSON PRODUIT (stt_catalog.json)
# ---------------------------------------
# {
#   "commandes": [
#     {
#       "id": "MTR_FORWARD",
#       "traitement": "SIMPLE",
#       "cat": "V",
#       "destinataire": "A",
#       "moteurActif": 1,
#       "synonymes": ["avance", "tout droit"],
#       "id_fils": [],
#       "vpiv_parts": { "var": "Mtr", "prop": "cmd", "inst": "*", "val": "50,0,2" },
#       "alias_tts": ["Je fonce !"]
#     }, ...
#   ]
# }
#
# PRÉCAUTIONS
# -----------
# - Le fichier CSV doit être encodé UTF-8 avec BOM (utf-8-sig) ou UTF-8 pur.
# - Les champs vides dans Alias → on utilise l'ID comme synonyme par défaut.
# - Les champs vides dans ID_fils et Alias-TTS → listes vides.
# - Lancer depuis le répertoire contenant table_d_catalogue.csv,
#   ou adapter les constantes FILE_CSV / FILE_JSON / FILE_WORDS.
#
# DÉPENDANCES
# -----------
#   Python stdlib uniquement : csv, json, os
#
# UTILISATION
# -----------
#   python3 rz_build_system.py
#   → génère stt_catalog.json et rz_words.txt dans le répertoire courant
#   → enchaîner avec : python3 rz_build_dict.py
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.1  (correction bug sorted() + headers complets)
# DATE    : 2026-03-05
# =============================================================================

import csv
import json
import os

# =============================================================================
# CONFIGURATION
# =============================================================================

# Fichier CSV source (Table D)
FILE_CSV   = 'table_d_catalogue.csv'

# Fichiers de sortie
FILE_JSON  = 'stt_catalog.json'    # Catalogue JSON pour rz_stt_handler.sh
FILE_WORDS = 'rz_words.txt'        # Mots pour rz_build_dict.py

# =============================================================================
# CONVERSION CSV → JSON + WORDLIST
# =============================================================================

def run_converter():
    """
    Lit le CSV, construit stt_catalog.json et rz_words.txt.
    """

    commandes_json    = []
    liste_mots_sphinx = set()

    # Le mot-clé wake word est toujours requis dans le dictionnaire
    liste_mots_sphinx.add("rz")

    # --- Vérification présence CSV ---
    if not os.path.exists(FILE_CSV):
        print(f"[!] Erreur : fichier '{FILE_CSV}' introuvable.")
        print("    Placer table_d_catalogue.csv dans le répertoire courant.")
        return

    # --- Lecture CSV ---
    with open(FILE_CSV, mode='r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f, delimiter=';')

        for row in reader:

            # ------------------------------------------------------------------
            # Gestion des synonymes (colonne Alias)
            # Si vide → on utilise l'ID en minuscules comme seul synonyme
            # ------------------------------------------------------------------
            raw_alias = row.get('Alias', '').strip()
            if not raw_alias:
                synonymes = [row['ID'].lower()]
            else:
                synonymes = [s.strip().lower() for s in raw_alias.split(',')]

            # Ajout des mots au vocabulaire Sphinx (on découpe les phrases)
            for s in synonymes:
                for mot in s.split():
                    liste_mots_sphinx.add(mot)

            # ------------------------------------------------------------------
            # Construction de l'objet commande
            # ------------------------------------------------------------------
            cmd = {
                "id":          row['ID'],
                "traitement":  row['Traitement'].upper(),
                "cat":         row['CAT'].upper(),
                "destinataire": row['Destinataire'],
                "moteurActif": int(row.get('moteurActif', 0) or 0),
                "synonymes":   synonymes,
                # id_fils : liste vide si colonne absente ou vide
                "id_fils": (
                    [i.strip() for i in row['ID_fils'].split(',')]
                    if row.get('ID_fils', '').strip()
                    else []
                ),
                # vpiv_parts : les 4 champs de la trame VPIV
                "vpiv_parts": {
                    "var":  row['VAR'],
                    "prop": row['PROP'],
                    "inst": row['INSTANCE'],
                    "val":  row['VALEUR']
                },
                # alias_tts : phrases de feedback vocal (séparateur |)
                "alias_tts": (
                    [t.strip() for t in row['Alias-TTS'].split('|')]
                    if row.get('Alias-TTS', '').strip()
                    else []
                )
            }
            commandes_json.append(cmd)

    # --- Sauvegarde stt_catalog.json ---
    with open(FILE_JSON, 'w', encoding='utf-8') as f:
        json.dump({"commandes": commandes_json}, f, indent=4, ensure_ascii=False)

    # --- Sauvegarde rz_words.txt ---
    # CORRECTION v2.1 : sorted() retourne une liste, pas un context manager.
    # Ancienne version buggée : `with sorted(liste_mots_sphinx) as mots_tries:`
    mots_tries = sorted(liste_mots_sphinx)      # ← liste triée alphabétiquement
    with open(FILE_WORDS, 'w', encoding='utf-8') as f:
        for mot in mots_tries:
            f.write(f"{mot}\n")

    # --- Résumé ---
    print("--- GÉNÉRATION RÉUSSIE ---")
    print(f"JSON  : {len(commandes_json)} commandes → {FILE_JSON}")
    print(f"DICT  : {len(liste_mots_sphinx)} mots uniques → {FILE_WORDS}")
    print("Étape suivante : python3 rz_build_dict.py")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    run_converter()