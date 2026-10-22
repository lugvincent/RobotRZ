#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : corriger_vpiv_ex.py
#  Chemin     : scratchpad/corriger_vpiv_ex.py
#  Projet     : RobotRZ — chantier « ordre VPIV »
#
#  OBJECTIF
#    Auditer puis corriger la colonne VPIV_EX de la Table A, dont 150 cellules
#    sur 246 portent l'ancien ordre $CAT:VAR:INST:PROP:VAL# au lieu de l'ordre
#    normatif actuel $CAT:VAR:PROP:INST:VAL#.
#
#  PRINCIPE DE LA DÉTECTION  (volontairement conservateur)
#    On ne « devine » rien, et on n'utilise QUE la colonne PROP comme juge.
#    Raison : la colonne INSTANCE contient des ÉNUMÉRATIONS de valeurs
#    autorisées (« rear|front », « TGD|THB|BASE », « *|0–11 ») et parfois de
#    simples descripteurs (« inst », « idx ») — la comparer à un champ de
#    trame produit de faux « ambigus ». La colonne PROP, elle, est vérifiée
#    comme toujours mono-valuée (aucune occurrence de « | », aucune vide).
#
#    On découpe l'exemple en 5 champs ($CAT:VAR:c3:c4:VALEUR#) et :
#       c3 == PROP                 -> CORRECT  (PROP en position 3 = norme)
#       c4 == PROP  et c3 != PROP  -> INVERSÉ  (permutable)
#       aucun des deux             -> AMBIGU   (jamais touché, relu à la main)
#
#  CAS PARTICULIER : l'étoile parasite
#    Trois lignes Urg portent SIX champs, avec un « * » surnuméraire :
#       $E:Urg:*:etat:A:active#   au lieu de   $E:Urg:etat:A:active#
#    Elles cumulent l'inversion et un champ en trop. Une simple permutation
#    donnerait $E:Urg:etat:*:A:active#, toujours faux. Elles sont donc
#    traitées à part : on supprime le « * » et on remonte le reste.
#
#  SORTIES
#    - audit_vpiv_ex.csv        : une ligne par exemple, avec son verdict
#    - TableA_VPIV_EX_corrige.csv : la Table A complète, colonne VPIV_EX réparée
#    - un récapitulatif sur la sortie standard
#
#  PRÉCAUTIONS
#    - Le fichier source est en UTF-8 AVEC BOM et sépare par point-virgule :
#      encoding='utf-8-sig' est obligatoire, sinon la 1re colonne s'appelle
#      "﻿SOURCE" et rien ne correspond.
#    - La colonne NOTES contient des retours à la ligne dans des champs
#      entre guillemets : il FAUT le module csv, jamais un split(';').
#    - Le fichier source n'est jamais modifié sur place.
#
#  Version 1.0 — 22/10/2026 — Vincent Philippe / Claude
# =============================================================================

import csv
import re
import sys
from collections import Counter, defaultdict

SRC = sys.argv[1]
DST = sys.argv[2]
AUDIT = sys.argv[3]

# --- Motif d'une trame VPIV complète : $ CAT : VAR : X : Y : VALEUR # ---------
# On capture les 5 champs séparément. La valeur peut contenir « : » (rare mais
# possible dans un intitulé), on la prend donc en dernier, non gourmande jusqu'au #.
MOTIF = re.compile(r'^\$([A-Za-z]+):([^:]+):([^:]*):([^:]*):(.*)#$')

lignes = []
with open(SRC, encoding='utf-8-sig', newline='') as f:
    lecteur = csv.DictReader(f, delimiter=';')
    entetes = lecteur.fieldnames
    for ligne in lecteur:
        lignes.append(ligne)

print(f"Colonnes détectées  : {entetes}")
print(f"Lignes de données   : {len(lignes)}\n")

verdicts = Counter()
par_variable = defaultdict(Counter)
details = []

for i, ligne in enumerate(lignes, start=2):      # start=2 : ligne 1 = entêtes
    var   = (ligne.get('VAR')      or '').strip()
    prop  = (ligne.get('PROP')     or '').strip()
    inst  = (ligne.get('INSTANCE') or '').strip()
    ex    = (ligne.get('VPIV_EX')  or '').strip()

    if not ex:
        verdicts['VIDE'] += 1
        details.append((i, var, prop, inst, ex, '', 'VIDE'))
        continue

    m = MOTIF.match(ex)
    if not m:
        verdicts['NON_CONFORME'] += 1
        details.append((i, var, prop, inst, ex, '', 'NON_CONFORME'))
        continue

    cat, var_ex, champ3, champ4, valeur = m.groups()

    # Repère : la variable de l'exemple doit être celle de la ligne.
    if var_ex.strip() != var:
        verdicts['VAR_DIVERGENTE'] += 1
        details.append((i, var, prop, inst, ex, '', 'VAR_DIVERGENTE'))
        continue

    c3, c4 = champ3.strip(), champ4.strip()

    if c3 == prop:
        # PROP est déjà en position 3 : la trame respecte la norme.
        verdict, corrige = 'CORRECT', ex

    elif c4 == prop:
        # PROP est en position 4 : inversion caractérisée.
        if c3 == '*' and ':' in valeur and valeur.split(':', 1)[0].strip() == inst:
            # Étoile parasite : $E:Urg:*:etat:A:active#  ->  $E:Urg:etat:A:active#
            # La « valeur » commence en réalité par la vraie instance.
            verdict = 'ETOILE_PARASITE'
            corrige = f'${cat}:{var}:{prop}:{valeur}#'
        else:
            verdict = 'INVERSE'
            corrige = f'${cat}:{var}:{prop}:{c3}:{valeur}#'

    else:
        # PROP n'apparaît ni en 3 ni en 4 : l'exemple ne correspond pas à sa
        # ligne. On ne corrige JAMAIS à l'aveugle — relecture humaine.
        verdict, corrige = 'AMBIGU', ex

    verdicts[verdict] += 1
    par_variable[var][verdict] += 1
    details.append((i, var, prop, inst, ex, corrige if verdict in ('INVERSE','ETOILE_PARASITE') else '', verdict))

    if verdict in ('INVERSE', 'ETOILE_PARASITE'):
        ligne['VPIV_EX'] = corrige

# --- Écriture de la Table A réparée ------------------------------------------
with open(DST, 'w', encoding='utf-8-sig', newline='') as f:
    ecriveur = csv.DictWriter(f, fieldnames=entetes, delimiter=';',
                              quoting=csv.QUOTE_MINIMAL, lineterminator='\r\n')
    ecriveur.writeheader()
    ecriveur.writerows(lignes)

# --- Écriture du journal d'audit ---------------------------------------------
with open(AUDIT, 'w', encoding='utf-8-sig', newline='') as f:
    w = csv.writer(f, delimiter=';', lineterminator='\r\n')
    w.writerow(['LIGNE', 'VAR', 'PROP', 'INSTANCE', 'VPIV_EX_AVANT',
                'VPIV_EX_APRES', 'VERDICT'])
    w.writerows(details)

# --- Récapitulatif ------------------------------------------------------------
print("=" * 62)
print("VERDICTS")
print("=" * 62)
for v, n in verdicts.most_common():
    print(f"  {v:<16} {n:>4}")
print(f"  {'TOTAL':<16} {sum(verdicts.values()):>4}")

print("\n" + "=" * 62)
print("VARIABLES CORRIGÉES (nombre d'exemples permutés)")
print("=" * 62)
for var in sorted(par_variable):
    n = par_variable[var]['INVERSE'] + par_variable[var]['ETOILE_PARASITE']
    if n:
        total = sum(par_variable[var].values())
        print(f"  {var:<12} {n:>3} / {total}")

# Les cas qui demandent un œil humain
print("\n" + "=" * 62)
print("CAS AMBIGUS / NON CONFORMES — NON CORRIGÉS, à relire")
print("=" * 62)
rien = True
for (lg, var, prop, inst, ex, _c, v) in details:
    if v in ('AMBIGU', 'NON_CONFORME', 'VAR_DIVERGENTE'):
        rien = False
        print(f"  L{lg:<4} {var:<10} PROP={prop!r:<14} INST={inst!r:<8} -> {ex}")
if rien:
    print("  (aucun)")
