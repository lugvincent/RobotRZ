#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : audit_ordre_global.py
#  Chemin     : outils/vpiv/audit_ordre_global.py
#  Projet     : RobotRZ — surveillance permanente de l'ordre VPIV
#
#  OBJECTIF
#    Balayer TOUT le depot — code, commentaires, markdown, pages HTML, flux
#    Node-RED, CSV — et signaler toute trame $CAT:VAR:PROP:INST:VAL# dont la
#    propriete n'est pas en position 3.
#
#  POURQUOI IL EXISTE
#    Le 22/10/2026, un premier balayage a corrige 89 commentaires du firmware
#    et annonce « zero trame inversee restante ». C'etait FAUX de quatre :
#    son motif, non borne a la ligne, traversait les entetes de bloc et
#    classait en « ambigu » quatre trames bien inversees. Le 23/10, quatre
#    autres ont ete trouvees dans des zones jamais auditees : les pages HTML
#    et les scripts de configuration Termux.
#    La lecon : un audit partiel donne une fausse tranquillite. Celui-ci
#    balaye tout, et liste ses propres exceptions au lieu de les taire.
#
#  CE QU'IL SAIT DISTINGUER
#    - Les CITATIONS VOLONTAIRES : des trames a l'ancien ordre laissees expres
#      dans docs/ comme pieces a conviction du defaut. Elles sont comptees a
#      part, pas signalees comme fautes.
#    - Les PIECES D'AUDIT : outils/vpiv/audit_vpiv_ex.csv contient par
#      construction la colonne « avant », donc 150 trames inversees. Normal.
#    - node-red/ : volontairement NON corrige tant que le chantier SP est
#      ouvert. Compte a part, avec un rappel.
#
#  PRECAUTION QUI FAIT TOUT
#    Le motif est borne a la ligne ([^\n]) et refuse < et > dans les champs.
#    Sans cela il traverse les commentaires multi-lignes et produit des faux
#    « ambigus » — l'erreur exacte du 22/10.
#
#  USAGE
#    python3 outils/vpiv/audit_ordre_global.py .
#    Code de retour 0 si rien d'inattendu, 1 sinon — utilisable en controle.
#
#  Version 1.0 — 23/10/2026 — Vincent Philippe / Claude
# =============================================================================

import csv, re, os, sys
from collections import defaultdict, Counter

RACINE = sys.argv[1] if len(sys.argv) > 1 else '.'
TABLE = os.path.join(RACINE, 'outils/vpiv/TableA_VPIV_EX_corrige.csv')

# --- Referentiel : variable -> ses proprietes --------------------------------
props = defaultdict(set)
with open(TABLE, encoding='utf-8-sig', newline='') as f:
    for l in csv.DictReader(f, delimiter=';'):
        v, p = (l['VAR'] or '').strip(), (l['PROP'] or '').strip()
        if v and p:
            props[v].add(p)

# --- Motif borne a la ligne — c'est la precaution centrale -------------------
MOTIF = re.compile(
    r'\$([A-Za-z]+):([A-Za-z_][A-Za-z0-9_]*):([^:#<>\n]{0,24}):([^:#<>\n]{0,24}):([^#<>\n]{0,60})#')

# --- Zones exemptees, avec leur raison ---------------------------------------
EXEMPT = {
    'outils/vpiv/audit_vpiv_ex.csv':
        "journal d'audit : contient la colonne « avant » par construction",
    'node-red/':
        "chantier SP en cours — volontairement non corrige, voir audit_nodered_ordre_VPIV.md",
}
# Fichiers de docs/ ou des citations volontaires sont attendues
CITATIONS = {
    'docs/convention_VPIV_ordre.md', 'docs/audit_nodered_ordre_VPIV.md',
    'docs/synthese_etat.md', 'docs/reference_firmware_arduino.md',
    'docs/travaux_robot_assemble.md', 'docs/schemas_SE.md',
    # Les scripts d'outillage citent la trame AVANT correction dans leur
    # entete, pour expliquer ce qu'ils reparent. Legitime.
    'outils/vpiv/corriger_vpiv_ex.py', 'outils/vpiv/genere_classeur_tables.py',
    'outils/vpiv/corriger_commentaires_vpiv.py', 'outils/vpiv/audit_ordre_global.py',
}

IGNORE_DIR = {'.git', '__pycache__', 'node_modules'}

def exempte(rel):
    for pref, _r in EXEMPT.items():
        if rel == pref or rel.startswith(pref):
            return pref
    return None

fautes, citations, exemptees, bonnes = [], Counter(), Counter(), 0

for d, sousd, fichiers in os.walk(RACINE):
    sousd[:] = [x for x in sousd if x not in IGNORE_DIR]
    for n in sorted(fichiers):
        chemin = os.path.join(d, n)
        rel = os.path.relpath(chemin, RACINE).replace(os.sep, '/')
        try:
            texte = open(chemin, encoding='utf-8', errors='replace').read()
        except Exception:
            continue
        for m in MOTIF.finditer(texte):
            cat, var, c3, c4, val = m.groups()
            if var not in props:
                continue
            c3, c4 = c3.strip(), c4.strip()
            if c3 in props[var]:
                bonnes += 1
                continue
            if c4 not in props[var]:
                continue                      # indecidable, on ne juge pas
            # Trame inversee : ou se trouve-t-elle ?
            z = exempte(rel)
            if z:
                exemptees[z] += 1
            elif rel in CITATIONS:
                citations[rel] += 1
            else:
                lg = texte[:m.start()].count('\n') + 1
                fautes.append((rel, lg, m.group(0), f'${cat}:{var}:{c4}:{c3}:{val}#'))

# --- Restitution --------------------------------------------------------------
print("=" * 72)
print("AUDIT GLOBAL DE L'ORDRE VPIV")
print("=" * 72)
print(f"  Trames au bon ordre                      {bonnes:>5}")
print(f"  Citations volontaires dans docs/         {sum(citations.values()):>5}")
print(f"  Dans les zones exemptees                 {sum(exemptees.values()):>5}")
print(f"  🔴 FAUTES                                {len(fautes):>5}")

if citations:
    print("\n  Citations volontaires — pieces a conviction, a NE PAS corriger :")
    for f, n in citations.most_common():
        print(f"      {n:>3}  {f}")

if exemptees:
    print("\n  Zones exemptees :")
    for z, n in exemptees.most_common():
        print(f"      {n:>3}  {z}")
        print(f"           {EXEMPT[z]}")

if fautes:
    print("\n" + "=" * 72)
    print("🔴 A CORRIGER")
    print("=" * 72)
    for rel, lg, avant, apres in fautes:
        print(f"  {rel}:{lg}")
        print(f"      {avant}")
        print(f"   -> {apres}")
    sys.exit(1)

print("\n✅ Aucune trame inversee inattendue.")
sys.exit(0)
