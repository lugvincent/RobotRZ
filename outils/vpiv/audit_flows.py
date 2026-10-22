#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier : audit_flows.py        (scratchpad — campagne « ordre VPIV »)
#
#  Objectif : trouver, dans un fichier de flux Node-RED, les trames VPIV a
#  l'ancien ordre, ET DIRE DANS QUEL TYPE DE NŒUD elles se trouvent.
#
#  Pourquoi ce dernier point decide de tout :
#    - dans un nœud « comment » ou le champ « info » d'un nœud -> documentation,
#      sans effet ;
#    - dans un nœud « inject », « function », « template », « change » ->
#      c'est du comportement : appuyer sur le bouton envoie une trame fausse.
#
#  Le script NE MODIFIE RIEN. Node-RED est le chantier en cours ; cet audit
#  sert a preparer le travail, pas a le faire.
#
#  Version 1.0 — 22/10/2026 — Vincent Philippe / Claude
# =============================================================================

import json, re, sys, csv
from collections import defaultdict, Counter

FLOWS = sys.argv[1]
TABLE_A = sys.argv[2]

# Referentiel VAR -> proprietes, depuis la Table A corrigee
props = defaultdict(set)
with open(TABLE_A, encoding='utf-8-sig', newline='') as f:
    for l in csv.DictReader(f, delimiter=';'):
        v, p = (l['VAR'] or '').strip(), (l['PROP'] or '').strip()
        if v and p:
            props[v].add(p)

MOTIF = re.compile(r'\$([A-Za-z]+):([A-Za-z_][A-Za-z0-9_]*):([^:#]*):([^:#]*):([^#]*)#')

noeuds = json.load(open(FLOWS, encoding='utf-8'))
print(f"Fichier : {FLOWS}")
print(f"Nœuds   : {len(noeuds)}\n")

# Champs d'un nœud qui sont de la DOCUMENTATION et non du comportement.
CHAMPS_DOC = {'info', 'name', 'l', 'label'}
TYPES_DOC = {'comment', 'group', 'tab'}

resultats = []
par_type = Counter()

def explorer(valeur, chemin, noeud):
    """Descend dans la structure d'un nœud et teste chaque chaine."""
    if isinstance(valeur, str):
        for m in MOTIF.finditer(valeur):
            cat, var, c3, c4, val = m.groups()
            if var not in props:
                continue
            c3s, c4s = c3.strip(), c4.strip()
            if c3s in props[var]:
                continue                      # ordre correct
            if c4s not in props[var]:
                continue                      # indecidable
            # Inversee. Documentation ou comportement ?
            champ = chemin.split('.')[0] if chemin else ''
            doc = (noeud.get('type') in TYPES_DOC) or (champ in CHAMPS_DOC)
            resultats.append({
                'type': noeud.get('type', '?'),
                'nom': (noeud.get('name') or '')[:38],
                'champ': chemin,
                'doc': doc,
                'trame': m.group(0),
                'corrige': f'${cat}:{var}:{c4s}:{c3s}:{val}#',
            })
    elif isinstance(valeur, dict):
        for k, v in valeur.items():
            explorer(v, f"{chemin}.{k}" if chemin else k, noeud)
    elif isinstance(valeur, list):
        for i, v in enumerate(valeur):
            explorer(v, f"{chemin}[{i}]", noeud)

for n in noeuds:
    if not isinstance(n, dict):
        continue
    par_type[n.get('type', '?')] += 1
    for k, v in n.items():
        if k in ('id', 'z', 'x', 'y', 'wires'):
            continue
        explorer(v, k, n)

comportement = [r for r in resultats if not r['doc']]
documentation = [r for r in resultats if r['doc']]

print("=" * 78)
print(f"TRAMES INVERSEES : {len(resultats)}")
print(f"   dont COMPORTEMENT (effet reel) : {len(comportement)}")
print(f"   dont documentation (sans effet): {len(documentation)}")
print("=" * 78)

if comportement:
    print("\n🔴 DANS DU COMPORTEMENT — a corriger lors du chantier SP")
    for r in comportement:
        print(f"\n  nœud « {r['nom']} »  type={r['type']}  champ={r['champ']}")
        print(f"      {r['trame']}")
        print(f"   -> {r['corrige']}")
else:
    print("\n✅ Aucune trame inversee dans du comportement.")

if documentation:
    print(f"\n📄 DANS DE LA DOCUMENTATION — {len(documentation)} occurrence(s), sans effet")
    vus = Counter()
    for r in documentation:
        vus[(r['type'], r['trame'])] += 1
    for (t, tr), n in vus.most_common(20):
        print(f"      [{t}] {tr}" + (f"   ×{n}" if n > 1 else ""))
