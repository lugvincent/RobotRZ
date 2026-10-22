#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier : audit_valeurs.py       (scratchpad — campagne « ordre VPIV »)
#  Objectif : second controle de coherence de la colonne VPIV_EX de la Table A.
#
#  Le premier controle portait sur l'ORDRE des champs. Celui-ci porte sur la
#  VALEUR : l'exemple respecte-t-il le FORMAT_VALUE declare sur sa propre ligne ?
#  C'est ainsi qu'a ete trouvee l'anomalie CfgS.boot ($I:CfgS:boot:*:OK# alors
#  que la ligne declare TYPE=boolean, FORMAT_VALUE=1|true).
#
#  PRUDENCE — on ne controle QUE les lignes ou le verdict est mecanique :
#    - FORMAT_VALUE est une enumeration de litteraux courts separes par « | »
#    - aucun token ne ressemble a un gabarit (<...>, ..., chiffres-plages)
#  Tout le reste est declare NON_CONTROLABLE et laisse de cote : mieux vaut
#  ne rien dire que produire du bruit.
#
#  Version 1.0 — 22/10/2026 — Vincent Philippe / Claude
# =============================================================================

import csv, re, sys
from collections import Counter

SRC = sys.argv[1]
MOTIF = re.compile(r'^\$([A-Za-z]+):([^:]+):([^:]*):([^:]*):(.*)#$')

# Un token « litteral » : lettres/chiffres/_/-/., pas de gabarit ni d'espace.
LITTERAL = re.compile(r'^[A-Za-z0-9_.\-]+$')

stats = Counter()
ecarts = []

with open(SRC, encoding='utf-8-sig', newline='') as f:
    for n, l in enumerate(csv.DictReader(f, delimiter=';'), start=2):
        ex = (l['VPIV_EX'] or '').strip()
        fv = (l['FORMAT_VALUE'] or '').strip()
        if not ex or not fv or fv == '_':
            stats['SANS_OBJET'] += 1
            continue
        m = MOTIF.match(ex)
        if not m:
            stats['SANS_OBJET'] += 1
            continue
        valeur = m.group(5).strip()

        tokens = [t.strip() for t in fv.split('|')]
        # L'enumeration doit etre entierement faite de litteraux courts.
        if not tokens or not all(LITTERAL.match(t) and len(t) <= 24 for t in tokens):
            stats['NON_CONTROLABLE'] += 1
            continue
        # Une valeur elle-meme gabarit (<val>, ...) n'est pas controlable.
        if not LITTERAL.match(valeur):
            stats['NON_CONTROLABLE'] += 1
            continue

        if valeur in tokens:
            stats['CONFORME'] += 1
        else:
            stats['ECART'] += 1
            ecarts.append((n, l['VAR'].strip(), l['PROP'].strip(),
                           l['TYPE'].strip(), fv, valeur, ex))

print("=" * 78)
print("COHERENCE  VPIV_EX (valeur)  vs  FORMAT_VALUE  —  Table A")
print("=" * 78)
for k, v in stats.most_common():
    print(f"  {k:<18} {v:>4}")

print("\n" + "=" * 78)
print("ECARTS — la valeur de l'exemple n'est pas dans le format declare")
print("=" * 78)
if not ecarts:
    print("  (aucun)")
for n, var, prop, typ, fv, val, ex in ecarts:
    print(f"  L{n:<4} {var}.{prop}")
    print(f"        TYPE={typ}   FORMAT_VALUE={fv}")
    print(f"        exemple -> {ex}        (valeur « {val} »)")
