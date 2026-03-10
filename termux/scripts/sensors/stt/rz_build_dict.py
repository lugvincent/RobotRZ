#!/usr/bin/env python3
# =============================================================================
# NOM DU FICHIER : rz_build_dict.py
# CHEMIN         : ~/scripts_RZ_SE/termux/scripts/sensors/stt/rz_build_dict.py
#
# OBJECTIF
# --------
# Génère le fichier fr.dict (dictionnaire phonétique) attendu par PocketSphinx,
# en associant les mots de rz_words.txt à leurs phonèmes depuis un lexique
# de référence français.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Charge lexique_referent.dict (lexique phonétique de référence français)
#    en mémoire sous forme de dictionnaire Python {mot: phonèmes}.
# 2. Lit rz_words.txt (généré par rz_build_system.py).
# 3. Pour chaque mot :
#    - Trouvé dans le lexique → écrit "mot phonèmes" dans fr.dict  ✓
#    - Absent               → écrit "mot T B D" (To Be Defined) ⚠
# 4. Affiche la liste des mots manquants pour correction manuelle.
#
# ARTICULATION
# ------------
#   rz_build_system.py → rz_words.txt → (ici) → lib_pocketsphinx/fr.dict
#   lib_pocketsphinx/fr.dict → rz_stt_manager.py (PocketSphinx)
#
# CORRECTION DES "T B D"
# ----------------------
# Les mots marqués "T B D" doivent être corrigés manuellement dans fr.dict.
# Format : "mot P H O N E M E S"  (phonèmes séparés par espaces)
# Exemple : "robotz R O B O T Z"
# Le lexique de référence utilisé est le Lexique3 français ou équivalent.
#
# PRÉCAUTIONS
# -----------
# - Lancer APRÈS rz_build_system.py (rz_words.txt doit exister).
# - lexique_referent.dict doit être présent dans lib_pocketsphinx/.
# - Les "T B D" non corrigés feront échouer la reconnaissance de ces mots.
#
# DÉPENDANCES
# -----------
#   Python stdlib uniquement : os
#
# UTILISATION
# -----------
#   python3 rz_build_dict.py
#   → génère lib_pocketsphinx/fr.dict
#   → corriger manuellement les "T B D" signalés
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.2  (headers complets, messages d'erreur améliorés)
# DATE    : 2026-03-05
# =============================================================================

import os

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

STT_DIR    = "/data/data/com.termux/files/home/scripts_RZ_SE/termux/scripts/sensors/stt"
LIB_PATH   = os.path.join(STT_DIR, "lib_pocketsphinx")
WORDS_FILE = os.path.join(STT_DIR,  "rz_words.txt")
REF_DICT   = os.path.join(LIB_PATH, "lexique_referent.dict")
FINAL_DICT = os.path.join(LIB_PATH, "fr.dict")

# =============================================================================
# GÉNÉRATION DU DICTIONNAIRE PHONÉTIQUE
# =============================================================================

def generate_phonetics():
    """
    Associe chaque mot de rz_words.txt à ses phonèmes depuis lexique_referent.dict.
    Produit lib_pocketsphinx/fr.dict utilisé par PocketSphinx.
    """

    # --- Vérifications préalables ---
    if not os.path.exists(WORDS_FILE):
        print(f"[!] Erreur : {WORDS_FILE} introuvable.")
        print("    Lancer d'abord : python3 rz_build_system.py")
        return

    if not os.path.exists(REF_DICT):
        print(f"[!] Erreur : lexique de référence introuvable : {REF_DICT}")
        print("    Placer lexique_referent.dict dans lib_pocketsphinx/")
        return

    # --- Chargement du lexique de référence en mémoire ---
    print("Chargement du lexique de référence ...")
    lexique = {}
    with open(REF_DICT, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split(maxsplit=1)
            if len(parts) == 2:
                lexique[parts[0].lower()] = parts[1]

    print(f"  {len(lexique)} entrées chargées.")

    # --- Lecture des mots personnalisés ---
    with open(WORDS_FILE, 'r', encoding='utf-8') as f:
        mes_mots = [line.strip().lower() for line in f if line.strip()]

    # --- Génération du dictionnaire final ---
    mots_trouves   = 0
    mots_manquants = []

    os.makedirs(LIB_PATH, exist_ok=True)

    with open(FINAL_DICT, 'w', encoding='utf-8') as f:
        for mot in sorted(mes_mots):
            if mot in lexique:
                f.write(f"{mot} {lexique[mot]}\n")
                mots_trouves += 1
            else:
                # Phonétique temporaire "T B D" = To Be Defined
                # À corriger manuellement dans fr.dict
                mots_manquants.append(mot)
                f.write(f"{mot} T B D\n")

    # --- Résumé ---
    print("--- FIN DE GÉNÉRATION ---")
    print(f"Mots avec phonétique   : {mots_trouves}")
    print(f"Mots manquants (T B D) : {len(mots_manquants)}")

    if mots_manquants:
        print("\n⚠ Mots à corriger manuellement dans fr.dict :")
        for m in mots_manquants:
            print(f"  - {m}")
        print(f"\n  Fichier à éditer : {FINAL_DICT}")
        print("  Format : 'mot P H O N E M E S'")
    else:
        print("✓ Tous les mots ont été trouvés dans le lexique.")


# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

if __name__ == "__main__":
    generate_phonetics()