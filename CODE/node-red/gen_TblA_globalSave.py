#!/usr/bin/env python3
"""
=============================================================================
Fichier    : gen_globalSave.py
Chemin     : /home/claude/gen_globalSave.py
Objectif   : Générer globalSave.json enrichi depuis la tableVarSynthese (table A)
Description:
  - Source structurelle : New_globalSave.json (liste arrêtée des variables
    et propriétés) - Les valeurs manuelles existantes sont PRESERVEES
  - Source données       : TableVarSyntheseV2_sans_doublons.csv (table A)
    - TYPE       → typeP
    - FORMAT_VALUE (enum) → optionsP
    - VPIV_EX (Ini+P)     → valeur initiale pour '*' si pas de valeur existante
    - NOTES               → descriptionP si non remplie
  - Logique flow : inspirée du nœud Node-RED SauvResultatActions
    (conservation des valeurs existantes, structure enrichedVars)
  - Table d'alias : correspondance nomP globalSave ↔ PROP table A
    (quand les noms diffèrent, ex: Gyro.mode → modeGyro)
Articulation:
  - Entrées  : New_globalSave.json, TableVarSyntheseV2_sans_doublons.csv
  - Sortie   : globalSave_final.json (prêt pour Termux Node-RED)
Précautions:
  - Les valeurs déjà saisies (non null) dans globalSave sont PRÉSERVÉES
  - Les propriétés introuvables dans table A sont conservées à l'identique
  - optionsP générés UNIQUEMENT pour TYPE=enum dans table A
Auteur     : Projet RZ - Générateur automatique
Version    : 1.0 — 27/03/2026
=============================================================================
"""
import json, pandas as pd, re
from datetime import datetime, timezone

# ============================================================
# 1.  CHARGEMENT DES SOURCES
# ============================================================
SRC_GS  = '/mnt/user-data/uploads/New_globalSave.json'
SRC_CSV = '/mnt/user-data/uploads/TableVarSyntheseV2_sans_doublons.csv'
OUT     = '/home/claude/globalSave_final.json'

with open(SRC_GS) as f:
    gs = json.load(f)

df_raw = pd.read_csv(SRC_CSV)
COLS = ['SOURCE','VAR','PROP','CAT','TYPE','FORMAT_VALUE','CODAGE',
        'DIRECTION','INSTANCE','COMPLEXE','CYCLE','NATURE','INTERFACE',
        'VPIV_EX','NOTES']
df = df_raw[COLS].fillna('').copy()

# ============================================================
# 2.  TABLE D'ALIAS  nomP_globalSave → PROP_tableA
#     Quand le nom dans globalSave diffère du nom dans table A
# ============================================================
ALIAS = {
    'Lring'   : {'lumin'    : 'int'},
    'Lrub'    : {'rgb'      : 'col'},
    'FS'      : {'val'      : 'force'},
    'IRmvt'   : {'val'      : 'value'},
    'Srv'     : {'ang'      : 'angle'},
    'Gyro'    : {'val'      : 'dataContGyro',  'mode': 'modeGyro'},
    'Mag'     : {'val'      : 'DataContMg',    'mode': 'modeMg'},
    'MicroSE' : {'rms'      : 'dataContRMS',   'dir' : 'direction'},
    'TorchSE' : {'act'      : 'active'},
    'Sys'     : {'batt'     : 'dataContBatt',
                 'temp'     : 'dataContThermal',
                 'cpu'      : 'dataContCpu'},
}

# ============================================================
# 3.  FONCTIONS UTILITAIRES
# ============================================================

def lookup(varName, nomP):
    """Retourne (rows, tableP) — rows=lignes table A pour VAR+PROP (alias inclus)."""
    tableP = ALIAS.get(varName, {}).get(nomP, nomP)
    rows = df[(df['VAR'] == varName) & (df['PROP'] == tableP)]
    return rows, tableP


def best_row(rows):
    """
    Priorité de sélection :
      1. SOURCE=P + CYCLE=Ini + DIRECTION contient 'SP→A'
      2. SOURCE=P + CYCLE=Ini
      3. SOURCE=P  (tout cycle)
      4. Première ligne disponible (autre source)
    """
    p = rows[rows['SOURCE'] == 'P']
    if not p.empty:
        ini = p[p['CYCLE'] == 'Ini']
        if not ini.empty:
            spa = ini[ini['DIRECTION'].str.contains('SP→A', na=False)]
            return spa.iloc[0] if not spa.empty else ini.iloc[0]
        return p.iloc[0]
    return rows.iloc[0] if not rows.empty else None


def parse_init_value(vpiv_ex_str):
    """
    Extrait VAL depuis $CAT:VAR:INST:PROP:VAL#
    Convention projet : INST avant PROP (légende table A)
    Retourne None si absent ou invalide.
    """
    s = str(vpiv_ex_str).strip()
    if not s or s in ('nan', '_', ''):
        return None
    # Dernière valeur avant '#'
    m = re.search(r':([^:#]+)#', s)
    if m:
        v = m.group(1).strip()
        return None if v.lower() in ('null', 'none', '') else v
    return None


def parse_options(format_value):
    """
    Construit optionsP depuis FORMAT_VALUE (UNIQUEMENT pour TYPE=enum).
    Formats acceptés :
      - '0=ARRET|1=VEILLE|...'   → [{value:'0', label:'ARRET'}, ...]
      - 'val1|val2|val3'         → [{value:'val1', label:'val1'}, ...]
    Rejette les plages numériques (≥ 0, 0–100) et les libellés génériques.
    """
    fv = str(format_value).strip()
    if not fv or fv in ('nan', '_', 'voir COMPLEXE'):
        return []
    # Rejeter plages numériques et autres non-enums
    if re.match(r'^[\d.]+[–\-][\d.]+$', fv):   return []
    if re.match(r'^[≥>≤<]\s*[\d]+', fv):         return []
    if fv.lower() in ('texte libre', 'url http', 'voir complexe', '≠ 0'):
        return []
    
    parts = fv.split('|')
    opts = []
    for part in parts:
        part = part.strip()
        if not part:
            continue
        if '=' in part:
            val, label = part.split('=', 1)
            opts.append({'value': val.strip(), 'label': label.strip()})
        else:
            opts.append({'value': part, 'label': part})
    return opts


def build_valeurs(instances, init_val, cycle, existing):
    """
    Construit le dict valeurs pour toutes les instances de la variable.
    - Si existing a des valeurs non-null  → les CONSERVER (valeurs utilisateur)
    - Sinon si cycle=Ini/Cste et init_val → affecter init_val à l'instance '*'
    - Sinon                               → null pour toutes les instances
    """
    # Vérifier si des valeurs utilisateur existent
    has_user_values = any(
        v is not None and v != '' for v in existing.values()
    )
    if has_user_values:
        # Conserver l'existant, juste s'assurer que toutes les instances ont une clé
        result = dict(existing)
        for inst in instances:
            if inst not in result:
                result[inst] = None
        return result, True   # (dict, was_preserved)

    # Construire depuis table A
    result = {}
    for inst in instances:
        if inst == '*' and cycle in ('Ini', 'Cste') and init_val is not None:
            result[inst] = init_val
        else:
            result[inst] = None
    return result, False      # (dict, was_preserved)

# ============================================================
# 4.  GÉNÉRATION
# ============================================================
NOW = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.000Z')

enriched_new = {}
rapport = []
not_found = []

for varName, varData in gs['enrichedVars'].items():
    instances  = varData.get('instances', ['*'])
    
    new_var = {
        'nomVarG'        : varName,
        'nomCpletVarG'   : varData.get('nomCpletVarG', ''),
        'instances'      : instances,
        'descriptionVarG': varData.get('descriptionVarG', ''),
        'dateUpdateVarG' : NOW,
        'proprietes'     : [],
    }

    for prop_gs in varData.get('proprietes', []):
        nomP = prop_gs['nomP']

        # ── Recherche table A ────────────────────────────────────
        rows, tableP = lookup(varName, nomP)
        is_alias     = (tableP != nomP)
        row          = best_row(rows)

        # ── Données de départ (existant dans globalSave) ─────────
        type_gs   = prop_gs.get('typeP', '')
        descr_gs  = prop_gs.get('descriptionP', '')
        valeurs_gs = prop_gs.get('valeurs', {'*': None})
        opts_gs   = prop_gs.get('optionsP', [])
        nomCpletP = prop_gs.get('nomCpletP', '')

        if row is not None:
            # ── Extraction table A ───────────────────────────────
            type_A    = str(row['TYPE']).strip()
            fv        = str(row['FORMAT_VALUE']).strip()
            cycle     = str(row['CYCLE']).strip()
            nature    = str(row['NATURE']).strip()
            notes     = str(row['NOTES']).strip()
            vpiv_ex   = str(row['VPIV_EX']).strip()

            # Si best n'est pas Ini, chercher aussi une ligne Ini pour la valeur
            if cycle != 'Ini':
                ini_p = rows[(rows['SOURCE']=='P') & (rows['CYCLE']=='Ini')]
                if not ini_p.empty:
                    vpiv_ini = str(ini_p.iloc[0]['VPIV_EX']).strip()
                    cycle    = 'Ini'
                    vpiv_ex  = vpiv_ini if vpiv_ini else vpiv_ex

            # Type : priorité table A (sauf types sémantiques custom = 'event')
            if type_gs == 'event':
                typeP = 'event'       # conserver le type sémantique custom
            elif type_A not in ('', 'nan'):
                typeP = type_A
            else:
                typeP = type_gs or 'string'

            # Description : conserver si remplie et non triviale
            if descr_gs and descr_gs.strip() not in ('', 'nan'):
                descriptionP = descr_gs
            elif notes and notes.strip() not in ('_', 'nan', ''):
                descriptionP = notes.strip()
            else:
                MAP_NATURE = {
                    'Config':'Config', 'ACK':'ACK', 'Etat':'État',
                    'Event':'Event',   'param':'Paramètre',
                    'fusion':'Fusion', 'Intent':'Intent', '_':'',
                }
                lab = MAP_NATURE.get(nature, nature)
                descriptionP = nomP + (f' — {lab}' if lab else '')

            # optionsP : UNIQUEMENT pour type=enum et si pas déjà défini
            if opts_gs:
                optionsP = opts_gs           # conserver options existantes
            elif type_A == 'enum':
                optionsP = parse_options(fv)
            else:
                optionsP = []

            # Valeur initiale
            init_val = parse_init_value(vpiv_ex)

            tag = f'[tableA:{tableP}]' + (' ⤷alias' if is_alias else '')

        else:
            # ── Propriété non trouvée dans table A ──────────────
            typeP        = type_gs or 'string'
            descriptionP = descr_gs
            optionsP     = opts_gs
            init_val     = None
            cycle        = 'Dyn'
            not_found.append(f'{varName}.{nomP}')
            tag = '⚠ NON TROUVÉ tableA'

        # ── Valeurs ──────────────────────────────────────────────
        valeurs, preserved = build_valeurs(instances, init_val, cycle, valeurs_gs)

        # ── Assemblage propriété ──────────────────────────────────
        new_prop = {
            'nomP'        : nomP,
            'nomCpletP'   : nomCpletP,
            'descriptionP': descriptionP,
            'typeP'       : typeP,
            'valeurs'     : valeurs,
        }
        if optionsP:
            new_prop['optionsP'] = optionsP

        new_var['proprietes'].append(new_prop)
        
        init_str = str(valeurs.get('*')) if valeurs.get('*') is not None else 'null'
        flag = ' [CONSERVÉ]' if preserved else ''
        rapport.append(
            f"  {varName}.{nomP:15s} {tag:30s} | "
            f"type={typeP:8s} | init={init_str:12s} | "
            f"opts={len(optionsP)}{flag}"
        )

    enriched_new[varName] = new_var

# ============================================================
# 5.  SAUVEGARDE
# ============================================================
output = {
    'enrichedVars': enriched_new,
    'lastUpdate'  : NOW,
    '_meta': {
        'generatedBy' : 'gen_globalSave.py v1.0',
        'generatedAt' : NOW,
        'sources'     : [SRC_GS, SRC_CSV],
    }
}

with open(OUT, 'w', encoding='utf-8') as f:
    json.dump(output, f, indent=4, ensure_ascii=False)

# ============================================================
# 6.  RAPPORT
# ============================================================
print("=" * 80)
print("RAPPORT DE GÉNÉRATION — globalSave.json enrichi")
print("=" * 80)
for line in rapport:
    print(line)

if not_found:
    print()
    print("── Propriétés NON TROUVÉES dans table A (conservées à l'identique) ──")
    for nf in not_found:
        print(f"  ⚠  {nf}")

print()
print(f"✅  Fichier généré : {OUT}")
print(f"    Variables      : {len(enriched_new)}")
total_p = sum(len(v['proprietes']) for v in enriched_new.values())
print(f"    Propriétés     : {total_p}")
enriched_with_opts = sum(
    1 for v in enriched_new.values()
    for p in v['proprietes'] if p.get('optionsP')
)
print(f"    Props avec opts: {enriched_with_opts}")
