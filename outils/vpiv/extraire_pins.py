#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : extraire_pins.py
#  Chemin     : outils/vpiv/extraire_pins.py
#  Projet     : RobotRZ
#
#  OBJECTIF
#    Produire la table des broches (pins) de la Mega et de l'UNO EN LA LISANT
#    DANS LE CODE, jamais en la recopiant. Sort du Markdown et du CSV.
#
#  POURQUOI CE SCRIPT EXISTE
#    Le chantier des 20-22/10/2026 a montré ce que coûte une table recopiée à
#    la main puis désynchronisée : la colonne VPIV_EX de la Table A, non migrée,
#    avait contaminé 89 commentaires du firmware, deux légendes, des documents
#    archivés et jusqu'au code de réception de la Mega. Une table de brochage
#    recopiée vieillirait exactement pareil. Celle-ci se régénère.
#
#  CE QU'IL LIT
#    config/config.h    les #define de broches simples (servo, leds, micros,
#                       capteur de force, infrarouge, encodeurs)
#    config/config.cpp  les tableaux usTrigPins[], usEchoPins[], usNames[]
#    config/config.h    MAIN_SERIAL et MOTEURS_SERIAL, d'où se déduisent les
#                       broches série de la Mega
#    src/uno/main.cpp   MEGA_SERIAL et SERIAL_BAUD côté UNO
#
#  PRÉCAUTIONS
#    - Ne lit QUE des #define et des initialisations de tableaux littéraux.
#      Si une broche venait à être calculée à l'exécution, elle échapperait à
#      ce script : il le signalerait en ne la trouvant pas, pas en inventant.
#    - Les broches du bouclier moteur Pololu ne sont PAS ici : la bibliothèque
#      n'est pas versionnée dans le dépôt (lib_deps la télécharge). Elles sont
#      signalées comme à confirmer.
#    - Ne modifie aucun fichier source.
#
#  Version 1.0 — 23/10/2026 — Vincent Philippe / Claude
# =============================================================================

import re, sys, io, os

RACINE = sys.argv[1] if len(sys.argv) > 1 else '.'
CFG_H = os.path.join(RACINE, 'lib/RZlibrairiesPersoNew/src/config/config.h')
CFG_C = os.path.join(RACINE, 'lib/RZlibrairiesPersoNew/src/config/config.cpp')
UNO_C = os.path.join(RACINE, 'src/uno/main.cpp')

lire = lambda p: io.open(p, encoding='utf-8').read()

# --- 1. Les #define de broches simples ---------------------------------------
# On ne retient que les constantes dont la valeur est une broche plausible :
# un entier, ou Ax (entrée analogique). Tout le reste (vitesses, seuils,
# durées) est ecarté par cette seule condition.
DEF = re.compile(r'^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(A?\d{1,2})\s*(?:/[/*](.*))?$',
                 re.MULTILINE)

MODULE = [  # (fragment du nom de la constante, module, libellé)
    ('FS_',     'fs',     'capteur de force HX711'),
    ('MVT_IR',  'mvt_ir', 'détecteur de mouvement infrarouge'),
    ('LRING',   'lring',  'anneau de LED'),
    ('LRUB',    'lrub',   'ruban de LED'),
    ('MIC_',    'mic',    'micro analogique'),
    ('ODO_',    'odom',   'encodeur de roue'),
    ('SERVO_',  'srv',    'servomoteur'),
]

def module_de(nom):
    for frag, mod, lib in MODULE:
        if frag in nom:
            return mod, lib
    return '?', ''

texte_h = lire(CFG_H)
simples = []
for m in DEF.finditer(texte_h):
    nom, val, com = m.group(1), m.group(2), (m.group(3) or '').strip()
    if 'ZERO' in nom or 'BAUD' in nom:        # angles neutres, vitesses série
        continue
    if not nom.endswith('_PIN'):
        continue
    mod, lib = module_de(nom)
    simples.append((val, nom, mod, lib, com.strip('* ')))

# --- 2. Les tableaux ultrasons ------------------------------------------------
texte_c = lire(CFG_C)

def tableau(nom, texte):
    m = re.search(rf'{nom}\s*\[[^\]]*\]\s*=\s*\{{([^}}]*)\}}', texte, re.S)
    if not m:
        return []
    return [x.strip().strip('"') for x in m.group(1).split(',') if x.strip()]

trigs = tableau('usTrigPins', texte_c)
echos = tableau('usEchoPins', texte_c)
noms = tableau('usNames', texte_c)

# --- 3. Les liaisons série ----------------------------------------------------
# Sur une Mega 2560 : Serial = 0/1, Serial1 = 19/18, Serial2 = 17/16, Serial3 = 15/14
MEGA_SERIE = {'Serial': ('0', '1'), 'Serial1': ('19', '18'),
              'Serial2': ('17', '16'), 'Serial3': ('15', '14')}  # (RX, TX)

def defvaleur(nom, texte):
    m = re.search(rf'^\s*#define\s+{nom}\s+(\S+)', texte, re.MULTILINE)
    return m.group(1) if m else None

main_serial = defvaleur('MAIN_SERIAL', texte_h)
mot_serial = defvaleur('MOTEURS_SERIAL', texte_h)
main_baud = defvaleur('MAIN_BAUD', texte_h)
mot_baud = defvaleur('MOTEURS_BAUD', texte_h)

texte_uno = lire(UNO_C)
uno_serial = defvaleur('MEGA_SERIAL', texte_uno)
m = re.search(r'SERIAL_BAUD\s*=\s*(\d+)', texte_uno)
uno_baud = m.group(1) if m else '?'

# --- 4. Restitution ------------------------------------------------------------
def cle(v):
    """Trie les broches : numériques d'abord, puis A0, A1…"""
    return (1, int(v[1:])) if v.startswith('A') else (0, int(v))

lignes = []
for val, nom, mod, lib, com in simples:
    lignes.append((val, nom, mod, lib, com))
for i, (t, e) in enumerate(zip(trigs, echos)):
    n = noms[i] if i < len(noms) else f'#{i}'
    lignes.append((t, 'usTrigPins[%d]' % i, 'us', f'ultrason {n} — trigger', ''))
    lignes.append((e, 'usEchoPins[%d]' % i, 'us', f'ultrason {n} — echo', ''))
if main_serial in MEGA_SERIE:
    rx, tx = MEGA_SERIE[main_serial]
    lignes.append((rx, f'{main_serial} RX', 'com', f'liaison USB vers le Doro (SE) @ {main_baud}', ''))
    lignes.append((tx, f'{main_serial} TX', 'com', f'liaison USB vers le Doro (SE) @ {main_baud}', ''))
if mot_serial in MEGA_SERIE:
    rx, tx = MEGA_SERIE[mot_serial]
    lignes.append((rx, f'{mot_serial} RX', 'com', f'liaison vers l\'UNO moteurs @ {mot_baud}', ''))
    lignes.append((tx, f'{mot_serial} TX', 'com', f'liaison vers l\'UNO moteurs @ {mot_baud}', ''))

lignes.sort(key=lambda l: cle(l[0]))

print("| Broche | Constante dans le code | Module | Rôle |")
print("|---|---|---|---|")
for val, nom, mod, lib, com in lignes:
    r = lib or com
    print(f"| **{val}** | `{nom}` | `{mod}` | {r} |")

print()
print(f"Total : {len(lignes)} broches utilisées sur la Mega.")
print(f"Liaison Doro  : {main_serial} @ {main_baud} bauds (port USB de la carte)")
print(f"Liaison UNO   : Mega {mot_serial} @ {mot_baud}  <->  UNO {uno_serial} @ {uno_baud}")

# --- 5. Broches libres ---------------------------------------------------------
occupees = {l[0] for l in lignes}
num = {str(i) for i in range(0, 54)}
ana = {f'A{i}' for i in range(0, 16)}
libres = sorted((num | ana) - occupees, key=cle)
print(f"\nBroches libres sur la Mega ({len(libres)}) :")
print('  ' + ' '.join(libres))
