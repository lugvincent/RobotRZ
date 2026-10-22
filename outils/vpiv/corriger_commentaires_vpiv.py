#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : corriger_commentaires_vpiv.py
#  Chemin     : scratchpad/corriger_commentaires_vpiv.py
#  Projet     : RobotRZ — chantier « ordre VPIV »
#
#  OBJECTIF
#    Remettre dans l'ordre normatif $CAT:VAR:PROP:INST:VAL# les trames
#    d'EXEMPLE écrites dans les COMMENTAIRES du firmware Arduino, qui ont
#    été recopiées depuis la colonne VPIV_EX de la Table A avant sa migration.
#
#  POURQUOI C'EST DÉLICAT
#    Une trame VPIV peut apparaître à deux endroits très différents :
#      - dans un commentaire  -> c'est un exemple, documentaire, on corrige ;
#      - dans une chaîne de code (sprintf, strcmp, Serial.print) -> c'est un
#        COMPORTEMENT. Y toucher changerait ce que la carte émet.
#    Ce script ne modifie QUE les commentaires. Toute trame rencontrée hors
#    commentaire est signalée pour relecture humaine et laissée intacte.
#
#  COMMENT LE VERDICT EST ÉTABLI
#    La Table A corrigée fournit, pour chaque variable, l'ensemble de ses
#    noms de propriétés. Pour une trame $CAT:VAR:c3:c4:VAL# :
#      c3 ∈ propriétés(VAR)                 -> CORRECT
#      c4 ∈ propriétés(VAR) et c3 ∉          -> INVERSÉ, on permute
#      les deux, ou aucun                   -> AMBIGU, on ne touche pas
#
#  DÉTECTION DES COMMENTAIRES
#    Automate à états parcourant le fichier caractère par caractère, qui
#    suit : code / commentaire-ligne (//) / commentaire-bloc (/* */) /
#    chaîne ("...") / caractère ('...'), en tenant compte des échappements.
#    C'est plus lourd qu'une expression régulière, mais une regex se fait
#    piéger par un « // » à l'intérieur d'une chaîne (ex. "http://…").
#
#  PRÉCAUTIONS
#    - Mode --dry-run par défaut : rien n'est écrit sans --appliquer.
#    - Les fichiers sont lus et réécrits en UTF-8, fins de ligne préservées.
#
#  Version 1.0 — 22/10/2026 — Vincent Philippe / Claude
# =============================================================================

import csv
import re
import sys
from collections import defaultdict, Counter

TABLE_A = sys.argv[1]
RACINE = sys.argv[2]
APPLIQUER = '--appliquer' in sys.argv

# --- 1. Référentiel : variable -> ensemble de ses propriétés ------------------
props_par_var = defaultdict(set)
with open(TABLE_A, encoding='utf-8-sig', newline='') as f:
    for ligne in csv.DictReader(f, delimiter=';'):
        v = (ligne.get('VAR') or '').strip()
        p = (ligne.get('PROP') or '').strip()
        if v and p:
            props_par_var[v].add(p)

print(f"Référentiel : {len(props_par_var)} variables, "
      f"{sum(len(s) for s in props_par_var.values())} propriétés\n")


def zones_commentaires(texte):
    """Renvoie un tableau de booléens : vrai si le caractère i est dans un
    commentaire. Automate code / // / /* */ / "..." / '...'."""
    n = len(texte)
    dans_com = [False] * n
    etat = 'CODE'
    i = 0
    while i < n:
        c = texte[i]
        suiv = texte[i + 1] if i + 1 < n else ''

        if etat == 'CODE':
            if c == '/' and suiv == '/':
                etat = 'LIGNE'; dans_com[i] = dans_com[i + 1] = True; i += 2; continue
            if c == '/' and suiv == '*':
                etat = 'BLOC'; dans_com[i] = dans_com[i + 1] = True; i += 2; continue
            if c == '"':
                etat = 'CHAINE'
            elif c == "'":
                etat = 'CARAC'

        elif etat == 'LIGNE':
            if c == '\n':
                etat = 'CODE'
            else:
                dans_com[i] = True

        elif etat == 'BLOC':
            dans_com[i] = True
            if c == '*' and suiv == '/':
                dans_com[i + 1] = True; etat = 'CODE'; i += 2; continue

        elif etat == 'CHAINE':
            if c == '\\':
                i += 2; continue
            if c == '"':
                etat = 'CODE'

        elif etat == 'CARAC':
            if c == '\\':
                i += 2; continue
            if c == "'":
                etat = 'CODE'

        i += 1
    return dans_com


# Une trame : $CAT:VAR:c3:c4:valeur#  — la valeur peut contenir « : »,
# on s'arrête au premier « # ».
MOTIF = re.compile(r'\$([A-Za-z]+):([A-Za-z_][A-Za-z0-9_]*):([^:#]*):([^:#]*):([^#]*)#')

stats = Counter()
hors_commentaire = []
modifications = []

import os
for dossier, _sd, fichiers in os.walk(RACINE):
    if '.git' in dossier.split(os.sep):
        continue
    for nom in sorted(fichiers):
        if not nom.endswith(('.cpp', '.h', '.ino', '.c')):
            continue
        chemin = os.path.join(dossier, nom)
        with open(chemin, encoding='utf-8', errors='surrogateescape') as f:
            texte = f.read()

        masque = zones_commentaires(texte)
        remplacements = []       # (debut, fin, nouveau)

        for m in MOTIF.finditer(texte):
            cat, var, c3, c4, val = m.groups()
            if var not in props_par_var:
                stats['VAR_INCONNUE'] += 1
                continue

            propres = props_par_var[var]
            c3s, c4s = c3.strip(), c4.strip()
            est_com = masque[m.start()]

            if c3s in propres:
                stats['CORRECT'] += 1
                continue
            if c4s not in propres:
                stats['AMBIGU'] += 1
                continue

            # Ici : c4 est une propriété, c3 non -> trame inversée.
            if not est_com:
                stats['INVERSE_HORS_COMMENTAIRE'] += 1
                ligne_no = texte[:m.start()].count('\n') + 1
                hors_commentaire.append((chemin, ligne_no, m.group(0)))
                continue

            stats['INVERSE_CORRIGE'] += 1
            nouveau = f'${cat}:{var}:{c4s}:{c3s}:{val}#'
            ligne_no = texte[:m.start()].count('\n') + 1
            remplacements.append((m.start(), m.end(), nouveau))
            modifications.append((chemin, ligne_no, m.group(0), nouveau))

        if remplacements and APPLIQUER:
            for debut, fin, nouveau in reversed(remplacements):
                texte = texte[:debut] + nouveau + texte[fin:]
            with open(chemin, 'w', encoding='utf-8', errors='surrogateescape') as f:
                f.write(texte)

# --- Rapport ------------------------------------------------------------------
print("=" * 70)
print("BILAN" + ("  (APPLIQUÉ)" if APPLIQUER else "  (SIMULATION — rien n'est écrit)"))
print("=" * 70)
for k, v in stats.most_common():
    print(f"  {k:<28} {v:>4}")

if hors_commentaire:
    print("\n" + "=" * 70)
    print("🔴 TRAMES INVERSÉES HORS COMMENTAIRE — NON TOUCHÉES, à examiner")
    print("=" * 70)
    for ch, lg, tr in hors_commentaire:
        print(f"  {ch}:{lg}\n      {tr}")

print("\n" + "=" * 70)
print("MODIFICATIONS PAR FICHIER")
print("=" * 70)
par_fichier = Counter(ch for ch, _l, _a, _n in modifications)
for ch, n in sorted(par_fichier.items()):
    print(f"  {n:>3}  {ch}")

print("\n" + "=" * 70)
print("DÉTAIL DES 20 PREMIÈRES")
print("=" * 70)
for ch, lg, avant, apres in modifications[:20]:
    print(f"  {os.path.basename(ch)}:{lg}")
    print(f"      {avant}\n   -> {apres}")
