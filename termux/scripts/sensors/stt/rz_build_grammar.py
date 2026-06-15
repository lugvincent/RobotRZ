#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_build_grammar.py
# CHEMIN         : ~/robotRZ-smartSE/termux/scripts/sensors/stt/rz_build_grammar.py
#
# OBJECTIF
# --------
# Génère le fichier de grammaire JSGF utilisé par PocketSphinx pour
# la reconnaissance des commandes vocales RZ.
# Chaque commande valide commence par "rz" suivi d'un alias du catalogue.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Lit tableD_catalogueV2.csv (Table D).
# 2. Extrait tous les alias des commandes Statut=OK (pas Pro, pas ⛔).
# 3. Génère rz_grammar.jsgf : grammaire JSGF avec toutes les alternatives.
# 4. Les mots avec apostrophe sont découpés en deux tokens
#    (ex: "l'anneau" → "l anneau") pour compatibilité PocketSphinx.
#
# ARTICULATION
# ------------
#   tableD_catalogueV2.csv → rz_build_grammar.py → lib_pocketsphinx/rz_grammar.jsgf
#   rz_grammar.jsgf + fr.dict → rz_stt_manager.py (Phase 2 reconnaissance)
#
# GRAMMAIRE PRODUITE
# ------------------
# #JSGF V1.0 UTF-8 fr;
# grammar rz_commands;
# public <command> = rz <action>;
# <action> = stop | arrête | tout droit | avance | ... ;
#
# PRÉCAUTIONS
# -----------
# - Tous les mots des alias doivent être présents dans fr.dict.
#   Si un mot est absent → PocketSphinx l'ignore ou plante.
#   Lancer rz_build_dict.py AVANT rz_build_grammar.py pour garantir
#   que fr.dict est à jour.
# - Les alias avec apostrophe sont découpés : "l'anneau" → "l anneau"
# - Les alias avec tiret sont conservés tels quels si dans fr.dict
#   (ex: "vas-y", "active-toi"). Sinon les ajouter manuellement à fr.dict.
# - Seules les entrées Statut=OK sont incluses. Les entrées Pro ne sont
#   pas encore déployées.
#
# DÉPENDANCES
# -----------
#   Python stdlib uniquement : csv, os
#
# UTILISATION
# -----------
#   python3 rz_build_grammar.py
#   → génère lib_pocketsphinx/rz_grammar.jsgf
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.1  (fix apostrophes — tokens conservés comme dans fr.dict)
# DATE    : 2026-05-27
# =============================================================================

import csv
import os

# =============================================================================
# CONFIGURATION
# =============================================================================

FILE_CSV  = 'tableD_catalogueV2.csv'
FILE_JSGF = os.path.join('lib_pocketsphinx', 'rz_grammar.jsgf')

# Statuts à inclure dans la grammaire (Pro = V2, pas encore déployé)
STATUTS_OK = {'OK'}

# =============================================================================
# NETTOYAGE DES ALIAS POUR JSGF
# =============================================================================

def nettoyer_alias(alias: str) -> str:
    """
    Prépare un alias pour l'insertion dans la grammaire JSGF.
    - Découpe les contractions avec apostrophe : "l'anneau" → "l anneau"
    - Conserve les tirets (correspondent aux entrées de fr.dict)
    - Met en minuscules
    """
    # Conserver apostrophes et tirets : tokens identiques à fr.dict
    # "l'anneau" reste "l'anneau", "vas-y" reste "vas-y"
    alias = " ".join(alias.split())
    return alias.lower().strip()

# =============================================================================
# GÉNÉRATION JSGF
# =============================================================================

def generate_grammar():
    """
    Lit le CSV, extrait les alias OK, génère rz_grammar.jsgf.
    """

    if not os.path.exists(FILE_CSV):
        print(f"[!] Erreur : {FILE_CSV} introuvable.")
        return

    aliases = []           # liste ordonnée (évite doublons via set intermédiaire)
    aliases_set = set()
    nb_ignores = 0

    with open(FILE_CSV, 'r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f, delimiter=';')

        for row in reader:
            id_val = row.get('ID', '').strip()

            # Filtrer séparateurs
            if not id_val or id_val.startswith('##'):
                continue

            # Filtrer selon Statut
            statut = row.get('Statut', 'OK').strip()
            if statut not in STATUTS_OK:
                nb_ignores += 1
                continue

            raw_alias = row.get('Alias', '').strip()
            if not raw_alias:
                continue

            for a in raw_alias.split('|'):
                a_clean = nettoyer_alias(a)
                if a_clean and a_clean not in aliases_set:
                    aliases.append(a_clean)
                    aliases_set.add(a_clean)

    if not aliases:
        print("[!] Aucun alias trouvé — vérifier le CSV.")
        return

    # Tri : les plus longs en premier (priorité dans la reconnaissance)
    aliases_sorted = sorted(aliases, key=lambda x: (-len(x), x))

    # --- Écriture JSGF ---
    os.makedirs(os.path.dirname(FILE_JSGF), exist_ok=True)

    with open(FILE_JSGF, 'w', encoding='utf-8') as f:
        f.write('#JSGF V1.0 UTF-8 fr;\n')
        f.write('// Grammaire générée automatiquement par rz_build_grammar.py\n')
        f.write('// Source : tableD_catalogueV2.csv\n')
        f.write('// Ne pas modifier manuellement — relancer rz_build_grammar.py\n\n')
        f.write('grammar rz_commands;\n\n')
        f.write('// Toute commande commence par le wake word "rz"\n')
        f.write('public <command> = rz <action>;\n\n')
        f.write('<action> =\n')

        for i, alias in enumerate(aliases_sorted):
            virgule = ' |' if i < len(aliases_sorted) - 1 else ' ;'
            f.write(f'    {alias}{virgule}\n')

    # --- Résumé ---
    print("--- GÉNÉRATION RÉUSSIE ---")
    print(f"Aliases inclus  : {len(aliases_sorted)} → {FILE_JSGF}")
    print(f"Entrées ignorées (Pro/⛔) : {nb_ignores}")
    print("Étape suivante : vérifier que tous les mots sont dans fr.dict")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    generate_grammar()
