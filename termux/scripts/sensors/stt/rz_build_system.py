#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER: rz_build_system.py
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_build_system.py
# DESCRIPTION:
#    Script de conversion qui prend le catalogue CSV et génère à la fois le JSON
#    pour le handler de commandes et la liste de mots pour le dictionnaire de
#    reconnaissance vocale PocketSphinx.
# FONCTIONS PRINCIPALES:
#    - run_converter: Lit le CSV, construit les structures de données, et sauvegarde
#      les fichiers de sortie:  
#stt_catalog.json : Le cerveau pour ton Handler Bash.
#rz_words.txt : La liste de tous les mots que Sphinx devra apprendre (ton dictionnaire brut).stt_catalog.json : Le cerveau pour ton Handler Bash.
# ===============================================================
import csv
import json
import os

# CONFIGURATION
FILE_CSV = 'table_d_catalogue.csv'
FILE_JSON = 'stt_catalog.json'
FILE_WORDS = 'rz_words.txt'

def run_converter():
    commandes_json = []
    liste_mots_sphinx = set()
    liste_mots_sphinx.add("rz") # Le mot-clé est toujours requis

    if not os.path.exists(FILE_CSV):
        print(f"Erreur : Le fichier {FILE_CSV} est introuvable.")
        return

    with open(FILE_CSV, mode='r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f, delimiter=';')
        
        for row in reader:
            # Gestion des Alias vides : si vide, on utilise l'ID
            raw_alias = row.get('Alias', '').strip()
            if not raw_alias:
                synonymes = [row['ID'].lower()]
            else:
                synonymes = [s.strip().lower() for s in raw_alias.split(',')]

            # On ajoute les synonymes à la liste pour Sphinx
            for s in synonymes:
                for mot in s.split(): # On découpe si l'alias est une phrase
                    liste_mots_sphinx.add(mot)

            # Construction de l'objet de commande
            cmd = {
                "id": row['ID'],
                "traitement": row['Traitement'].upper(),
                "cat": row['CAT'].upper(),
                "destinataire": row['Destinataire'],
                "moteurActif": int(row.get('moteurActif', 0) or 0),
                "synonymes": synonymes,
                "id_fils": [i.strip() for i in row['ID_fils'].split(',')] if row['ID_fils'] else [],
                "vpiv_parts": {
                    "var": row['VAR'],
                    "prop": row['PROP'],
                    "inst": row['INSTANCE'],
                    "val": row['VALEUR']
                },
                "alias_tts": [t.strip() for t in row['Alias-TTS'].split('|')] if row['Alias-TTS'] else []
            }
            commandes_json.append(cmd)

    # 1. Sauvegarde du JSON pour le Handler
    with open(FILE_JSON, 'w', encoding='utf-8') as f:
        json.dump({"commandes": commandes_json}, f, indent=4, ensure_ascii=False)
    
    # 2. Sauvegarde de la liste de mots pour le dictionnaire Sphinx
    with sorted(liste_mots_sphinx) as mots_tries:
        with open(FILE_WORDS, 'w', encoding='utf-8') as f:
            for mot in mots_tries:
                f.write(f"{mot}\n")

    print(f"--- GÉNÉRATION RÉUSSIE ---")
    print(f"JSON : {len(commandes_json)} commandes intégrées.")
    print(f"DICT : {len(liste_mots_sphinx)} mots uniques à apprendre pour Sphinx.")

if __name__ == "__main__":
    run_converter()