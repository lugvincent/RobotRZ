#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_build_system.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_build_system.py
#
# OBJECTIF
# --------
# Script de construction du système STT.
# Lit le catalogue CSV (Table D) et génère les deux fichiers nécessaires
# au fonctionnement de la chaîne vocale locale :
#   1. stt_catalog.json : dictionnaire de commandes pour rz_stt_handler.sh
#   2. rz_words.txt     : liste de mots pour rz_build_dict.py (PocketSphinx)
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Pour chaque ligne du CSV :
#   - Filtre les lignes séparateurs (ID vide ou débutant par ##)
#   - Filtre les lignes Statut=⛔ (erreur connue, ne pas déployer)
#   - Extrait les synonymes (Alias) → liste pour PocketSphinx
#   - Construit l'objet commande (id, traitement, vpiv_parts, encodage, alias_tts, ...)
#   - Accumule les mots uniques pour le dictionnaire phonétique
#
# FILTRES APPLIQUÉS
# -----------------
# - Lignes séparateurs : ID vide ou commençant par ## → ignorées silencieusement
# - Statut=⛔ : entrée en erreur connue → ignorée avec avertissement
# - Champs manquants : gérés avec .get() + valeur par défaut ""
#
# ARTICULATION
# ------------
#   tableD_catalogueV2.csv → rz_build_system.py → stt_catalog.json
#                                              → rz_words.txt
#   rz_words.txt          → rz_build_dict.py  → lib_pocketsphinx/fr.dict
#   stt_catalog.json      → rz_stt_handler.sh  (reconnaissance commandes)
#   lib_pocketsphinx/     → rz_stt_manager.py  (moteur PocketSphinx)
#
# FORMAT CSV ATTENDU (séparateur ;)
# ----------------------------------
# Colonnes obligatoires :
#   ID, Traitement, CAT, Destinataire, moteurActif, Alias, ID_fils,
#   VAR, PROP, INSTANCE, VALEUR, Prepa_Encodage (ou Encodage), Alias-TTS
# Colonnes optionnelles :
#   Statut  (OK / Pro / ⛔) — les lignes ⛔ sont ignorées
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
#       "vpiv_parts": {
#           "var": "Mtr", "prop": "cmd", "inst": "*",
#           "val": "50,0,2", "encodage": ""
#       },
#       "alias_tts": ["Je fonce !"]
#     }, ...
#   ]
# }
#
# NOTE COLONNE ENCODAGE
# ---------------------
# La colonne Prepa_Encodage (ou Encodage si renommée) contient les instructions
# d'extraction pour les commandes PLAN :
#   angle->NUM_DIR : extraire un angle + direction (gauche/droite)
#   speed->NUM     : extraire une vitesse numérique
#   angle+speed    : extraire angle ET vitesse
# Stockée dans vpiv_parts.encodage → utilisée par process_plan() dans le handler.
#
# PRÉCAUTIONS
# -----------
# - Le fichier CSV doit être encodé UTF-8 avec BOM (utf-8-sig) ou UTF-8 pur.
# - Alias : séparateur | entre les variantes (ex: avance|vas-y|tout droit).
# - Les champs vides dans ID_fils et Alias-TTS → listes vides.
# - Lancer depuis le répertoire contenant tableD_catalogueV2.csv,
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
# VERSION : 2.2  (filtres séparateurs + Statut⛔ + colonne Encodage)
# DATE    : 2026-05-25
# =============================================================================

import csv
import json
import os

# =============================================================================
# CONFIGURATION
# =============================================================================

# Fichier CSV source (Table D)
FILE_CSV   = 'tableD_catalogueV2.csv'

# Fichiers de sortie
FILE_JSON  = 'stt_catalog.json'    # Catalogue JSON pour rz_stt_handler.sh
FILE_WORDS = 'rz_words.txt'        # Mots pour rz_build_dict.py

# Valeur Statut bloquante — la ligne est ignorée si Statut contient cette chaîne
STATUT_BLOQUE = '⛔'

# =============================================================================
# CONVERSION CSV → JSON + WORDLIST
# =============================================================================

def est_ligne_separateur(row: dict) -> bool:
    """
    Retourne True si la ligne est un séparateur/commentaire à ignorer.
    Critères : ID vide, ou ID commençant par ## (convention future).
    """
    id_val = row.get('ID', '').strip()
    return id_val == '' or id_val.startswith('##')


def est_ligne_bloquee(row: dict) -> bool:
    """
    Retourne True si la ligne est marquée ⛔ dans la colonne Statut.
    Ignorée silencieusement si la colonne Statut est absente du CSV.
    """
    statut = row.get('Statut', '').strip()
    return STATUT_BLOQUE in statut


def lire_encodage(row: dict) -> str:
    """
    Lit la colonne d'encodage PLAN.
    Accepte 'Prepa_Encodage' (nom actuel CSV) ou 'Encodage' (nom proposé).
    Retourne une chaîne vide si la colonne est absente.
    """
    return (
        row.get('Prepa_Encodage', '').strip()
        or row.get('Encodage', '').strip()
    )


def run_converter():
    """
    Lit le CSV, construit stt_catalog.json et rz_words.txt.
    """

    commandes_json    = []
    liste_mots_sphinx = set()

    # Compteurs pour le résumé final
    nb_separateurs = 0
    nb_bloques     = 0
    ids_bloques    = []

    # Le wake word est toujours requis dans le dictionnaire phonétique
    liste_mots_sphinx.add("rz")

    # --- Vérification présence CSV ---
    if not os.path.exists(FILE_CSV):
        print(f"[!] Erreur : fichier '{FILE_CSV}' introuvable.")
        print("    Placer tableD_catalogueV2.csv dans le répertoire courant.")
        return

    # --- Lecture CSV ---
    with open(FILE_CSV, mode='r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f, delimiter=';')

        for row in reader:

            # ------------------------------------------------------------------
            # FILTRE 1 — Lignes séparateurs (ID vide ou ##)
            # Ces lignes structurent le CSV visuellement mais ne sont pas
            # des commandes — on les ignore silencieusement.
            # ------------------------------------------------------------------
            if est_ligne_separateur(row):
                nb_separateurs += 1
                continue

            # ------------------------------------------------------------------
            # FILTRE 2 — Lignes bloquées (Statut=⛔)
            # Entrée connue comme incorrecte — ne pas déployer.
            # ------------------------------------------------------------------
            if est_ligne_bloquee(row):
                nb_bloques += 1
                ids_bloques.append(row.get('ID', '?'))
                continue

            # ------------------------------------------------------------------
            # Gestion des synonymes (colonne Alias)
            # Si vide → on utilise l'ID en minuscules comme seul synonyme
            # ------------------------------------------------------------------
            raw_alias = row.get('Alias', '').strip()
            if not raw_alias:
                synonymes = [row['ID'].lower()]
            else:
                synonymes = [s.strip().lower() for s in raw_alias.split('|')]

            # Ajout des mots au vocabulaire Sphinx (on découpe les phrases)
            for s in synonymes:
                for mot in s.split():
                    liste_mots_sphinx.add(mot)

            # ------------------------------------------------------------------
            # Construction de l'objet commande
            # ------------------------------------------------------------------
            cmd = {
                "id":           row.get('ID', '').strip(),
                "traitement":   row.get('Traitement', '').strip().upper(),
                "cat":          row.get('CAT', '').strip().upper(),
                "destinataire": row.get('Destinataire', '').strip(),
                "moteurActif":  int(row.get('moteurActif', 0) or 0),
                "synonymes":    synonymes,

                # id_fils : liste vide si colonne absente ou vide
                # Utilisé par les commandes COMPLEXE (macros)
                "id_fils": (
                    [i.strip() for i in row['ID_fils'].split(',')]
                    if row.get('ID_fils', '').strip()
                    else []
                ),

                # vpiv_parts : les 4 champs de la trame VPIV
                # + encodage : instruction d'extraction pour les commandes PLAN
                #   ex: "angle->NUM_DIR", "speed->NUM", "angle+speed"
                #   Vide pour SIMPLE, LOCAL et COMPLEXE.
                "vpiv_parts": {
                    "var":      row.get('VAR',      '').strip(),
                    "prop":     row.get('PROP',     '').strip(),
                    "inst":     row.get('INSTANCE', '').strip(),
                    "val":      row.get('VALEUR',   '').strip(),
                    "encodage": lire_encodage(row)
                },

                # alias_tts : phrases de feedback vocal (séparateur |)
                # Le handler choisit aléatoirement une variante à chaque exécution.
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
    mots_tries = sorted(liste_mots_sphinx)
    with open(FILE_WORDS, 'w', encoding='utf-8') as f:
        for mot in mots_tries:
            f.write(f"{mot}\n")

    # --- Résumé ---
    print("--- GÉNÉRATION RÉUSSIE ---")
    print(f"Commandes générées : {len(commandes_json)} → {FILE_JSON}")
    print(f"Mots PocketSphinx  : {len(liste_mots_sphinx)} → {FILE_WORDS}")
    print(f"Séparateurs ignorés : {nb_separateurs}")
    if nb_bloques:
        print(f"Lignes ⛔ ignorées  : {nb_bloques} ({', '.join(ids_bloques)})")
    print("Étape suivante : python3 rz_build_dict.py")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    run_converter()