#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : audit_commentaires.py
#  Chemin     : outils/vpiv/audit_commentaires.py
#  Projet     : RobotRZ — concordance des commentaires avec le code
#
#  OBJECTIF
#    Chercher, dans les commentaires du firmware, les affirmations que le code
#    contredit. Le firmware est abondamment commenté — c'est une force, mais
#    un commentaire faux est pire qu'un commentaire absent : on le croit.
#
#  CE QUI A MOTIVE CET OUTIL
#    Trois cas trouves a la main les 22-24/10/2026 :
#      - « LOOP_MAX_MS = 80ms » dans main.cpp, alors que config.h definit un
#        LOOP_MAX_TIME_MS a 100 qui n'est JAMAIS utilise. Qui irait regler le
#        seuil a l'endroit naturel ne changerait rien.
#      - « Recoit via Serial1 » dans src/uno/main.cpp, alors qu'un UNO n'a pas
#        de Serial1 et que la ligne 36 definit MEGA_SERIAL sur Serial.
#      - « $I:System:boot:*:OK# » dans src/mega/main.cpp, alors que le code
#        emet sendInfo("CfgS","boot","*","1").
#    Aucun n'etait detectable par le compilateur : un commentaire ne compile
#    pas. D'ou ce script.
#
#  CE QU'IL SAIT TROUVER  (mecaniquement, sans interpretation)
#    1. CONSTANTE CITEE AVEC UNE VALEUR  — « LOOP_MAX_MS = 80ms » confronte au
#       #define reel. Signale si les deux chiffres different.
#    2. IDENTIFIANT CITE QUI N'EXISTE PAS — un nom en forme de fonction ou de
#       macro, mentionne dans un commentaire, introuvable dans les sources.
#    3. FICHIER REFERENCE ABSENT — « voir xxx.cpp » quand xxx.cpp n'existe pas.
#    4. CONSTANTE DEFINIE JAMAIS UTILISEE — le piege LOOP_MAX_TIME_MS.
#
#  CE QU'IL NE SAIT PAS FAIRE
#    Juger le SENS d'une phrase. « Ce module est rapide » n'est pas verifiable.
#    L'outil ne remplace pas la lecture : il trouve les contradictions
#    chiffrees et nominatives, qui sont les plus nombreuses et les plus
#    trompeuses.
#
#  PRECAUTION
#    Il ne modifie RIEN. Il imprime et sort en code 1 s'il trouve quelque chose.
#
#  USAGE
#    python3 outils/vpiv/audit_commentaires.py .
#
#  Version 1.0 — 24/10/2026 — Vincent Philippe / Claude
# =============================================================================

import os, re, sys
from collections import defaultdict

RACINE = sys.argv[1] if len(sys.argv) > 1 else '.'
ZONES = ['lib/RZlibrairiesPersoNew/src', 'src']
EXT = ('.cpp', '.h', '.ino', '.c')

# --- Les mots qu'on ne prend jamais pour des identifiants du projet ----------
BANALS = {
    'TODO', 'FIXME', 'NOTE', 'OK', 'KO', 'VPIV', 'MQTT', 'USB', 'UART', 'LED',
    'PWM', 'GND', 'RX', 'TX', 'CPU', 'RAM', 'JSON', 'CSV', 'HTML', 'ACK',
    'ARRET', 'VEILLE', 'FIXE', 'DEPLACEMENT', 'AUTONOME', 'ERREUR', 'STOP',
    'ROLE', 'FICHIER', 'OBJECTIF', 'VERSION', 'AUTEUR', 'DATE', 'CHEMIN',
    'PRECAUTION', 'PRECAUTIONS', 'ARTICULATION', 'SOURCES', 'USAGE', 'II',
    'III', 'IV', 'SP', 'SE', 'RZ', 'HTTP', 'API', 'IDE', 'AVR', 'PIN', 'PINS',
}


def zones_commentaires(texte):
    """Rend la liste des (ligne, contenu) de chaque commentaire.

    Automate code / // / /* */ / "..." / '...'. Une expression reguliere se
    ferait piéger par le // d'une URL dans une chaine — meme precaution que
    pour corriger_commentaires_vpiv.py.
    """
    out, etat, i, n = [], 'CODE', 0, len(texte)
    debut, tampon = 1, []
    ligne = 1
    while i < n:
        c = texte[i]
        s = texte[i + 1] if i + 1 < n else ''
        if c == '\n':
            ligne += 1
        if etat == 'CODE':
            if c == '/' and s == '/':
                etat, debut, tampon = 'LIGNE', ligne, []
                i += 2
                continue
            if c == '/' and s == '*':
                etat, debut, tampon = 'BLOC', ligne, []
                i += 2
                continue
            if c == '"':
                etat = 'CHAINE'
            elif c == "'":
                etat = 'CARAC'
        elif etat == 'LIGNE':
            if c == '\n':
                out.append((debut, ''.join(tampon)))
                etat = 'CODE'
            else:
                tampon.append(c)
        elif etat == 'BLOC':
            if c == '*' and s == '/':
                out.append((debut, ''.join(tampon)))
                etat = 'CODE'
                i += 2
                continue
            tampon.append(c)
        elif etat == 'CHAINE':
            if c == '\\':
                i += 2
                continue
            if c == '"':
                etat = 'CODE'
        elif etat == 'CARAC':
            if c == '\\':
                i += 2
                continue
            if c == "'":
                etat = 'CODE'
        i += 1
    return out


# --- Collecte ----------------------------------------------------------------
fichiers = []
for z in ZONES:
    for d, sd, fs in os.walk(os.path.join(RACINE, z)):
        sd[:] = [x for x in sd if x not in ('.git', 'archive')]
        for f in sorted(fs):
            if f.endswith(EXT):
                fichiers.append(os.path.join(d, f))

sources = {}
for p in fichiers:
    sources[p] = open(p, encoding='utf-8', errors='replace').read()

tout_le_code = '\n'.join(sources.values())
noms_fichiers = {os.path.basename(p) for p in fichiers}

# Les #define avec une valeur numerique
DEFS = {}
for p, t in sources.items():
    for m in re.finditer(r'^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(-?\d+)\b', t, re.M):
        DEFS[m.group(1)] = (int(m.group(2)), os.path.basename(p),
                            t[:m.start()].count('\n') + 1)
# Les constantes C++ avec une valeur numerique
for p, t in sources.items():
    for m in re.finditer(r'^\s*(?:static\s+)?const\s+\w[\w\s]*?\s([A-Z][A-Z0-9_]*)\s*=\s*(-?\d+)',
                         t, re.M):
        DEFS.setdefault(m.group(1), (int(m.group(2)), os.path.basename(p),
                                     t[:m.start()].count('\n') + 1))

# Tous les identifiants presents dans le code
IDENTS = set(re.findall(r'\b([A-Za-z_][A-Za-z0-9_]{3,})\b', tout_le_code))

# --- Analyse ------------------------------------------------------------------
val_contredite, ident_absent, fichier_absent = [], [], []
cites = defaultdict(int)

MOTIF_VAL = re.compile(r'\b([A-Z][A-Z0-9_]{3,})\s*(?:=|vaut|:)\s*(\d+)\s*(?:ms|s|cm|mm|µs|us)?\b')
MOTIF_FCT = re.compile(r'\b([a-z][A-Za-z0-9_]*_[A-Za-z0-9_]+)\s*\(')
MOTIF_FIC = re.compile(r'\b([a-z_][a-z0-9_]{2,})\.(cpp|h|ino)\b')

for p, t in sources.items():
    base = os.path.basename(p)
    for lg, com in zones_commentaires(t):
        for m in MOTIF_VAL.finditer(com):
            nom, val = m.group(1), int(m.group(2))
            if nom in DEFS:
                cites[nom] += 1
                reel, ou, ligne_def = DEFS[nom]
                if reel != val:
                    val_contredite.append((base, lg, nom, val, reel, ou, ligne_def))
        for m in MOTIF_FCT.finditer(com):
            nom = m.group(1)
            if nom not in IDENTS and nom.upper() not in BANALS:
                ident_absent.append((base, lg, nom))
        for m in MOTIF_FIC.finditer(com):
            nf = m.group(0)
            if nf not in noms_fichiers:
                fichier_absent.append((base, lg, nf))

# Constantes definies mais jamais employees dans du code
jamais = []
for nom, (val, ou, ligne) in sorted(DEFS.items()):
    hors_def = len(re.findall(r'\b' + re.escape(nom) + r'\b', tout_le_code))
    if hors_def <= 1:
        jamais.append((nom, val, ou, ligne))

# --- Restitution ---------------------------------------------------------------
print("=" * 74)
print("CONCORDANCE DES COMMENTAIRES AVEC LE CODE")
print("=" * 74)
print(f"  Fichiers analyses          {len(fichiers):>4}")
print(f"  Constantes numeriques      {len(DEFS):>4}")
print(f"  🔴 Valeurs contredites      {len(val_contredite):>4}")
print(f"  🔴 Identifiants inexistants {len(ident_absent):>4}")
print(f"  🔴 Fichiers references absents {len(fichier_absent):>1}")
print(f"  ⚠️  Constantes jamais employees {len(jamais):>3}")

if val_contredite:
    print("\n" + "=" * 74)
    print("🔴 UN COMMENTAIRE DONNE UNE VALEUR, LE CODE EN DONNE UNE AUTRE")
    print("=" * 74)
    for base, lg, nom, dit, reel, ou, ld in val_contredite:
        print(f"  {base}:{lg}")
        print(f"      le commentaire dit  {nom} = {dit}")
        print(f"      le code definit     {nom} = {reel}   ({ou}:{ld})")

if ident_absent:
    print("\n" + "=" * 74)
    print("🔴 UN COMMENTAIRE CITE UNE FONCTION QUI N'EXISTE PAS")
    print("=" * 74)
    vus = set()
    for base, lg, nom in ident_absent:
        if (base, nom) in vus:
            continue
        vus.add((base, nom))
        print(f"  {base}:{lg}   {nom}()")

if fichier_absent:
    print("\n" + "=" * 74)
    print("🔴 UN COMMENTAIRE RENVOIE A UN FICHIER ABSENT")
    print("=" * 74)
    for base, lg, nf in fichier_absent:
        print(f"  {base}:{lg}   -> {nf}")

if jamais:
    print("\n" + "=" * 74)
    print("⚠️  DEFINIES MAIS JAMAIS EMPLOYEES — le piege du faux reglage")
    print("=" * 74)
    print("   Qui irait regler la valeur ici ne changerait rien.")
    for nom, val, ou, ligne in jamais:
        print(f"      {nom:<32} = {val:<8} {ou}:{ligne}")

total = len(val_contredite) + len(ident_absent) + len(fichier_absent)
print()
if total == 0 and not jamais:
    print("✅ Aucune discordance trouvee.")
    sys.exit(0)
sys.exit(1)
