#!/usr/bin/env python3
# =============================================================================
# NOM      : genere_classeur.py
# CHEMIN   : /home/claude/tables/genere_classeur.py
# OBJECTIF : produire TablesRZ_reference.ods, le classeur unique qui rassemble
#            les quatre tables normatives du projet RobotRZ.
#
# DESCRIPTION
# -----------
#   Cinq feuilles, dans cet ordre :
#     _Version   metadonnees, journal des versions, role de chaque feuille
#     TableA     reference normative des variables VPIV (253 lignes, 15 col.)
#     TableC     modele de persistance, aplati depuis TableC.json
#     TableD     catalogue des commandes vocales (72 commandes, 16 col.)
#     Legende    signification de chaque valeur de chaque champ
#
#   La Table B ne figure PAS dans ce classeur, et c'est volontaire : elle est
#   generee a l'execution par Node-RED depuis la Table C. L'inscrire ici
#   creerait une troisieme copie concurrente.
#
# SOURCES  (celles qui font foi, cf. LISEZ-MOI du dossier documentaire :
#           le .ods l'emporte sur le .csv qui l'accompagne)
#   TableA   TableA_updatedV3_corrige.ods   10/08/2026
#   TableC   TableC.json                    18/04/2026
#   TableD   tableD_catalogueV3_corrige.ods 06/08/2026
#   Legende  TableVarSyntheseLegendeTableA-Unifiee.csv  04/07/2026
#
# APLATISSEMENT DE LA TABLE C
#   TableC.json est arborescent : variable -> proprietes -> valeurs par
#   instance. Un tableur est plat. On produit donc UNE LIGNE PAR PROPRIETE,
#   en repetant le contexte de la variable. Les valeurs par instance et les
#   options d'enumeration, qui sont des structures, sont rendues en texte
#   lisible ("inst=valeur" separes par des sauts de ligne).
#   /!\ Ce depliage est a SENS UNIQUE. Le fichier qui fait foi pour la
#   Table C reste TableC.json : la feuille sert a lire et a comparer,
#   pas a regenerer le JSON.
#
# PRECAUTION
#   Le nom du fichier ne porte pas de numero de version : la version vit
#   dans la feuille _Version. Meme regle que pour synthese_etat.md (son
#   paragraphe 0) et pour node-red/functions/. Un nom stable evite les
#   copies concurrentes dont on ne sait plus laquelle fait foi.
#
# VERSION  : 1.0 - 22/10/2026 - Vincent Philippe / Claude
# =============================================================================

import sys, json, csv, datetime
sys.path.insert(0, "/home/claude/tables")
from lire_ods import lire

from odf.opendocument import OpenDocumentSpreadsheet
from odf.style import (Style, TextProperties, TableColumnProperties,
                       TableCellProperties, ParagraphProperties)
from odf.text import P
from odf.table import Table, TableColumn, TableRow, TableCell

U = "/root/.claude/uploads/aff57350-474a-5e94-8f23-f6be06c0884a/"
SORTIE = "/home/claude/tables/TablesRZ_reference.ods"
AUJ = "22/10/2026"

doc = OpenDocumentSpreadsheet()

# ---------------------------------------------------------------------------
# 1. STYLES
# ---------------------------------------------------------------------------
def cell_style(nom, fond=None, gras=False, couleur="#000000",
               taille="9pt", retour=True, haut=False):
    st = Style(name=nom, family="table-cell")
    props = {"border": "0.5pt solid #b0b8c4", "wrapoption": "wrap" if retour else "no-wrap"}
    if fond:
        props["backgroundcolor"] = fond
    if haut:
        props["verticalalign"] = "top"
    st.addElement(TableCellProperties(**props))
    st.addElement(TextProperties(fontsize=taille, fontweight="bold" if gras else "normal",
                                 color=couleur))
    doc.automaticstyles.addElement(st)
    return st

ST_ENTETE  = cell_style("Entete",  fond="#1a3d6d", gras=True, couleur="#ffffff", taille="9pt")
ST_CORPS   = cell_style("Corps",   haut=True)
ST_ALT     = cell_style("Alt",     fond="#f0f3f8", haut=True)
ST_TITRE   = cell_style("TitreDoc", gras=True, taille="16pt", couleur="#1a3d6d", retour=False)
ST_SOUSTIT = cell_style("SousTit", gras=True, taille="11pt", couleur="#1a3d6d")
ST_ALERTE  = cell_style("Alerte",  fond="#fdeaea", gras=True, couleur="#8a1f1f")
ST_NEUTRE  = cell_style("Neutre",  haut=True)

def colonne(largeur):
    st = Style(name=f"co{abs(hash(largeur))%99999}", family="table-column")
    st.addElement(TableColumnProperties(columnwidth=largeur))
    doc.automaticstyles.addElement(st)
    return st

def ajoute_ligne(table, valeurs, style):
    r = TableRow()
    for v in valeurs:
        c = TableCell(stylename=style, valuetype="string")
        # Les sauts de ligne internes deviennent des paragraphes distincts,
        # sinon Calc les affiche collés.
        for morceau in str(v).split("\n"):
            c.addElement(P(text=morceau))
        r.addElement(c)
    table.addElement(r)

def feuille(nom, entetes, lignes, largeurs):
    """Une feuille complete : colonnes dimensionnees, en-tete, corps zebre."""
    t = Table(name=nom)
    for l in largeurs:
        t.addElement(TableColumn(stylename=colonne(l)))
    ajoute_ligne(t, entetes, ST_ENTETE)
    for i, lg in enumerate(lignes):
        lg = list(lg) + [""] * (len(entetes) - len(lg))
        ajoute_ligne(t, lg[:len(entetes)], ST_ALT if i % 2 else ST_CORPS)
    doc.spreadsheet.addElement(t)
    return len(lignes)

# ---------------------------------------------------------------------------
# 2. LECTURE DES SOURCES
# ---------------------------------------------------------------------------
print("Lecture des sources...")

ta = lire(U + "9e4949ce-TableA_updatedV3_corrige.ods")["TableA_updatedV3_corrige"]
ENT_A, LIG_A = ta[0], ta[1:]
ENT_A[0] = "SOURCE"          # l'ODS ecrit « Srce », le CSV « SOURCE » : on unifie
print(f"   TableA : {len(LIG_A)} lignes")

# ---------------------------------------------------------------------------
# 2bis. CORRECTION DE LA COLONNE VPIV_EX           (ajout v1.1 — 22/10/2026)
# ---------------------------------------------------------------------------
#  La colonne VPIV_EX n'a jamais ete migree lors du passage de la convention
#  $CAT:VAR:INST:PROP:VAL#  a  $CAT:VAR:PROP:INST:VAL#. 150 de ses 251 exemples
#  non vides portaient encore l'ancien ordre — et c'est de cette colonne que
#  les commentaires du firmware, les documents archives et jusqu'au code de
#  reception de la Mega avaient recopie. Cause racine de tout le chantier des
#  20-22/10. Historique complet : docs/convention_VPIV_ordre.md.
#
#  REGLE DE DECISION — seule la colonne PROP sert de juge.
#  Raison : la colonne INSTANCE contient des ENUMERATIONS (« rear|front »,
#  « TGD|THB|BASE », « *|0-11 ») voire de simples descripteurs (« inst »,
#  « idx »). La comparer a un champ de trame produit de faux positifs. PROP,
#  elle, est verifiee mono-valuee : aucun « | », aucune cellule vide.
#      champ3 == PROP                      -> deja correct, on ne touche pas
#      champ4 == PROP et champ3 != PROP    -> inverse, on permute
#      ni l'un ni l'autre                  -> ambigu, on NE TOUCHE PAS
# ---------------------------------------------------------------------------
import re as _re
_MOTIF = _re.compile(r'^\$([A-Za-z]+):([^:]+):([^:]*):([^:]*):(.*)#$')
_iP, _iI, _iE = ENT_A.index("PROP"), ENT_A.index("INSTANCE"), ENT_A.index("VPIV_EX")
_n_perm = _n_etoile = _n_ok = _n_amb = 0

for _lg in LIG_A:
    _prop = (_lg[_iP] or "").strip()
    _inst = (_lg[_iI] or "").strip()
    _ex   = (_lg[_iE] or "").strip()
    if not _ex:
        continue
    _m = _MOTIF.match(_ex)
    if not _m:
        continue
    _cat, _var, _c3, _c4, _val = _m.groups()
    _c3, _c4 = _c3.strip(), _c4.strip()

    if _c3 == _prop:
        _n_ok += 1
    elif _c4 == _prop:
        # Cas particulier : trois trames Urg portent SIX champs, avec un « * »
        # surnumeraire — $E:Urg:*:etat:A:active#. Une simple permutation
        # donnerait $E:Urg:etat:*:A:active#, toujours faux. On retire l'etoile.
        if _c3 == "*" and ":" in _val and _val.split(":", 1)[0].strip() == _inst:
            _lg[_iE] = f"${_cat}:{_var}:{_prop}:{_val}#"
            _n_etoile += 1
        else:
            _lg[_iE] = f"${_cat}:{_var}:{_prop}:{_c3}:{_val}#"
            _n_perm += 1
    else:
        _n_amb += 1

print(f"   VPIV_EX : {_n_perm} permutees + {_n_etoile} etoiles parasites "
      f"= {_n_perm + _n_etoile} corrigees | {_n_ok} deja bonnes | {_n_amb} ambigues")
assert _n_amb == 0, "Cas ambigu detecte : relecture humaine obligatoire avant generation"

td = lire(U + "d0880137-tableD_catalogueV3_corrige.ods")["TableD_Catalogue"]
ENT_D, LIG_D = td[0], td[1:]
print(f"   TableD : {len(LIG_D)} commandes")

leg = list(csv.reader(open(U + "815aa455-TableVarSyntheseLegendeTableA-Unifiee.csv",
                          encoding="utf-8-sig"), delimiter=";"))
# Les deux premieres lignes sont un titre et un sous-titre, pas des donnees.
TITRE_LEG = " — ".join(x[0] for x in leg[:2] if x and x[0])
ENT_L, LIG_L = leg[2], [x for x in leg[3:] if any(c.strip() for c in x)]
print(f"   Legende : {len(LIG_L)} entrees")

tc = json.load(open(U + "2ba74e38-TableC.json", encoding="utf-8"))

# --- aplatissement de la Table C : une ligne par propriete
ENT_C = ["VAR", "Nom complet VAR", "Description VAR", "Instances",
         "Mise à jour", "PROP", "Nom complet PROP", "Description PROP",
         "Type", "Valeurs par instance", "Options (enum)"]
LIG_C = []
for nom, v in tc.items():
    inst = v.get("instances", "")
    if isinstance(inst, list):
        inst = ", ".join(str(x) for x in inst)
    props = v.get("proprietes") or []
    if not props:                                   # variable sans propriete
        LIG_C.append([nom, v.get("nomCpletVarG",""), v.get("descriptionVarG",""),
                      inst, v.get("dateUpdateVarG",""), "", "", "", "", "", ""])
        continue
    for p in props:
        vals = p.get("valeurs") or {}
        if isinstance(vals, dict):
            vals = "\n".join(f"{k} = {x}" for k, x in vals.items())
        opts = p.get("optionsP") or []
        if isinstance(opts, list):
            opts = "\n".join(
                f"{o.get('value','')} = {o.get('label','')}" if isinstance(o, dict) else str(o)
                for o in opts)
        LIG_C.append([nom, v.get("nomCpletVarG",""), v.get("descriptionVarG",""),
                      inst, v.get("dateUpdateVarG",""),
                      p.get("nomP",""), p.get("nomCpletP",""), p.get("descriptionP",""),
                      p.get("typeP",""), vals, opts])
print(f"   TableC : {len(tc)} variables aplaties en {len(LIG_C)} lignes")

# ---------------------------------------------------------------------------
# 3. FEUILLE _Version
# ---------------------------------------------------------------------------
t = Table(name="_Version")
for l in ("38mm", "30mm", "30mm", "95mm"):
    t.addElement(TableColumn(stylename=colonne(l)))

def L(vals, style=ST_NEUTRE):
    ajoute_ligne(t, vals, style)

L(["TablesRZ_reference.ods", "", "", ""], ST_TITRE)
L(["Classeur unique des tables normatives — projet RobotRZ", "", "", ""], ST_SOUSTIT)
L(["", "", "", ""])
L(["Version", "1.1", "", ""])
L(["Date", AUJ, "", ""])
L(["Auteur", "Vincent Philippe", "", ""])
L(["Emplacement",
   "AA-Doc-horsGit\\A - Architecture_Tables_Schemas\\Tables\\", "", ""])
L(["", "", "", ""])
L(["⚠ Le nom du fichier ne porte PAS de version. La version est ci-dessus, et nulle part ailleurs.",
   "", "", ""], ST_ALERTE)
L(["⚠ Depuis la v1.1, la colonne VPIV_EX de la feuille TableA est CORRIGEE. Toute copie anterieure au 22/10/2026 est fausse a 61 % sur cette colonne : ne plus s'y referer.",
   "", "", ""], ST_ALERTE)
L(["Même règle que pour synthese_etat.md (§0) et node-red/functions/ : un chemin stable évite les copies concurrentes dont on ne sait plus laquelle fait foi.",
   "", "", ""])
L(["", "", "", ""])

L(["LES FEUILLES", "", "", ""], ST_SOUSTIT)
L(["Feuille", "Lignes", "Source qui fait foi", "Rôle"], ST_ENTETE)
L(["TableA", str(len(LIG_A)), "cette feuille",
   "Référence normative de toutes les variables VPIV. Jamais chargée à l'exécution : c'est un document de conception."], ST_CORPS)
L(["TableC", str(len(LIG_C)), "TableC.json",
   "Modèle de persistance et d'initialisation (enrichedVars). Maître sur SP, édité par l'onglet ui_VarG. Dépliage à sens unique — voir l'avertissement plus bas."], ST_ALT)
L(["TableD", str(len(LIG_D)), "cette feuille",
   "Catalogue des commandes vocales reconnues. v3, 16 colonnes dont ID_traitement."], ST_CORPS)
L(["_Audit", "21 + 1", "produit le 22/10/2026",
   "Écarts entre la Table C et la Table A : 16 renommages, 5 propriétés absentes, 1 variable manquante. Chiffre le lot F de la synthèse d état."], ST_CORPS)
L(["Legende", str(len(LIG_L)), "cette feuille",
   "Signification de chaque valeur de chaque champ de la Table A. Seule légende valide."], ST_ALT)
L(["", "", "", ""])

L(["POURQUOI PAS DE TABLE B", "", "", ""], ST_SOUSTIT)
L(["La Table B (globalSave.json) est générée à l'exécution par Node-RED depuis la Table C. L'inscrire ici créerait une troisième copie concurrente d'une même information. La chaîne réelle est :",
   "", "", ""])
L(["TableC.json  →  globalSave.json (Table B)  →  courant_init.json (projection vers SE)",
   "", "", ""], ST_SOUSTIT)
L(["", "", "", ""])

L(["⚠ LE DÉPLIAGE DE LA TABLE C EST À SENS UNIQUE", "", "", ""], ST_ALERTE)
L(["TableC.json est arborescent, un tableur est plat. La feuille TableC donne une ligne par propriété, en répétant le contexte de la variable ; les valeurs par instance et les options d'énumération sont rendues en texte. On peut donc la lire et la comparer, mais PAS en régénérer le JSON. Le fichier qui fait foi pour la Table C reste TableC.json, et il s'édite par ui_VarG.",
   "", "", ""])
L(["", "", "", ""])

L(["RÈGLES À NE PAS ENFREINDRE", "", "", ""], ST_SOUSTIT)
L(["1", "VPIV", "", "$CAT:VAR:PROP:INST:VAL#  —  PROP en position 3, INST en position 4. Ne jamais intervertir."], ST_CORPS)
L(["2", "Flottants", "", "Jamais de nombre à virgule sur le fil : entiers ×10, ×100 ou ×1000 selon la précision."], ST_ALT)
L(["3", "Longueur", "", "VPIV_MAX_MSG_LEN = 128 (config/config.h, ligne 18)."], ST_CORPS)
L(["4", "Autorité", "", "Le PC est l'atelier de conception. Le RPi porte l'autorité d'exécution des tables B, C et D. Le Xiaomi reçoit un clone daté AVANT toute panne, jamais pendant."], ST_ALT)
L(["5", "Table D", "", "Son nom de fichier est codé en dur à trois endroits : original_init.sh (641), rz_build_system.py (107), rz_build_grammar.py (65). Tout renommage les corrige ensemble."], ST_CORPS)
L(["", "", "", ""])

L(["JOURNAL DES VERSIONS", "", "", ""], ST_SOUSTIT)
L(["Version", "Date", "Auteur", "Changement"], ST_ENTETE)
L(["1.0", AUJ, "VP / Claude",
   "Création. Rassemble quatre tables jusque-là dispersées en neuf fichiers, dont deux paires de doublons stricts et deux légendes donnant un ordre VPIV erroné (désormais dans old\\). Sources retenues : TableA_updatedV3_corrige.ods (10/08), TableC.json (18/04), tableD_catalogueV3_corrige.ods (06/08), TableVarSyntheseLegendeTableA-Unifiee.csv (04/07)."], ST_CORPS)
L(["1.1", AUJ, "VP / Claude",
   "CAUSE RACINE TRAITEE. La colonne VPIV_EX n'avait jamais ete migree lors du passage de la convention $CAT:VAR:INST:PROP:VAL# a $CAT:VAR:PROP:INST:VAL# : 150 de ses 251 exemples non vides portaient encore l'ancien ordre (13 variables : CfgS, Ctrl_L, FS, IRmvt, Lring, Lrub, Mic, Mtr, Odom, SecMvt, Srv, US, Urg). Comme c'est la seule colonne qui montre une trame entiere, c'est d'elle que tout le monde recopiait : les 89 commentaires du firmware, les 58 trames de IMP encodage, les deux legendes, et jusqu'au code de reception de la Mega — qui ne pouvait donc recevoir aucune commande. Corrigee ici par permutation deterministe, PROP servant de seul juge (INSTANCE contient des enumerations). Zero cas ambigu. Historique complet : docs/convention_VPIV_ordre.md."], ST_ALT)
L(["", "", "", ""], ST_CORPS)
L(["", "", "", ""], ST_ALT)
L(["", "", "", ""])

L(["DOCUMENT ASSOCIÉ", "", "", ""], ST_SOUSTIT)
L(["docs/tables_RZ_descripteur.md, dans le dépôt Git. C'est LUI la référence normative rédigée : il décrit les quinze champs de la Table A, le protocole VPIV, l'histoire des tables et un journal des erreurs. Ce classeur porte les données, le descripteur porte les règles.",
   "", "", ""])

doc.spreadsheet.addElement(t)

# ---------------------------------------------------------------------------
# 3bis. FEUILLE _Audit — ecarts Table C / Table A
#
#   Produite par confrontation automatique des couples VAR/PROP des deux
#   tables, puis classee a la main. Elle materialise le lot F de la
#   synthese d'etat : « report Tables B/C via ui_VarG ».
#   La correspondance proposee est une HYPOTHESE de lecture, a confirmer
#   variable par variable avant toute saisie dans ui_VarG.
# ---------------------------------------------------------------------------
AUDIT = [
    # (VAR, PROP dans C, PROP correspondante dans A, nature, commentaire)
    ("Lrub","rgb","col","renommage",
     "🔴 Bug déjà identifié dans la synthèse d'état, lot F. À corriger via ui_VarG uniquement."),
    ("IRmvt","Mode","mode","casse",
     "Majuscule parasite. Une comparaison sensible à la casse échoue silencieusement."),
    ("Gyro","mode","modeGyro","renommage",""),
    ("Gyro","val","dataContGyro","renommage",""),
    ("Gyro","freqCont","paraGyro","regroupement",
     "La fréquence est passée dans l'objet de paramètres."),
    ("Mag","mode","modeMg","renommage",""),
    ("Mag","val","DataContMg","renommage",
     "Noter le D majuscule côté Table A — incohérent avec les autres, mais c'est l'état réel."),
    ("Sys","cpu","dataContCpu","renommage",""),
    ("Sys","batt","dataContBatt","renommage",""),
    ("Sys","temp","dataContThermal","renommage",""),
    ("Sys","freqCont","paraSys","regroupement",""),
    ("Srv","ang","angle","renommage",""),
    ("MicroSE","dir","direction","renommage",""),
    ("MicroSE","rms","dataContRMS","renommage",""),
    ("TorchSE","act","active","renommage",
     "Ailleurs la propriété d'activation s'appelle « act ». TorchSE est l'exception."),
    ("Mic","modeMicsl","modeMicsF / modeMicsI","à vérifier",
     "Le dernier caractère est probablement un I majuscule lu comme un l minuscule. "
     "La Table A distingue deux modes, F et I : lequel ?"),
    ("Lring","timeout","—","absent de A",
     "Lrub a bien « timeout », pas Lring. Oubli dans la Table A, ou propriété "
     "qui n'a pas lieu d'être ici ?"),
    ("TorchSE","level","—","absent de A",
     "La Table A ne connaît que « active » pour TorchSE. Niveau d'intensité jamais normalisé."),
    ("Appli","launch","—","absent de A",
     "La Table A liste des applications (Baby, zoom, NavGPS…), pas des actions. "
     "Divergence de modèle, pas simple oubli."),
    ("Appli","close","—","absent de A", "Même remarque que launch."),
    ("COM","ping","—","absent de A",
     "La Table A ne connaît que debug, error, info, warn."),
]

t = Table(name="_Audit")
for l in ("22mm","28mm","34mm","26mm","100mm"):
    t.addElement(TableColumn(stylename=colonne(l)))

ajoute_ligne(t, ["AUDIT DE COHÉRENCE — Table C face à la Table A", "", "", "", ""], ST_SOUSTIT)
ajoute_ligne(t, [f"Produit le {AUJ} par confrontation automatique des couples VAR/PROP, "
                 "puis classé à la main.", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["⚠ La colonne « PROP dans A » est une HYPOTHÈSE de lecture. "
                 "À confirmer variable par variable AVANT toute saisie dans ui_VarG.",
                 "", "", "", ""], ST_ALERTE)
ajoute_ligne(t, ["", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["CONSTAT", "", "", "", ""], ST_SOUSTIT)
ajoute_ligne(t, ["La Table C date du 18/04/2026 et porte des noms de propriétés que la Table A "
                 "a renommés depuis. Sur 80 couples VAR/PROP, 21 ne trouvent pas leur "
                 "correspondant : 16 sont des renommages, des regroupements ou une différence de casse, 5 sont "
                 "réellement absents de la Table A. S'y ajoute la variable LockSE, présente "
                 "dans la Table A et inconnue de la Table C.",
                 "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["✅ RIEN N'EST CASSÉ AUJOURD'HUI — vérifié le 22/10/2026 sur le dépôt.",
                 "", "", "", ""], ST_SOUSTIT)
ajoute_ligne(t, ["1) SE n'est pas initialisé depuis la Table C. Il l'est par "
                 "termux/config/courant_init.json, qui emploie les noms de la Table A "
                 "(modeGyro, paraGyro, angleVSEBase, modeMg, paraMg…). Les noms divergents "
                 "ne l'atteignent jamais.", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["2) Le seul nœud qui transformerait enrichedVars en trames VPIV est "
                 "SF_Init_4, dans l'onglet Gtion_Init. Il exige des champs abbrVarG et "
                 "abbrP qui n'existent dans AUCUNE variable ni AUCUNE propriété de "
                 "globalSave.json (0 sur 22 et 0 sur 73). Il sort donc deux tableaux vides : "
                 "zéro trame émise. D'où son étiquette « (STUB) ».",
                 "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["3) globalSave.json (Table B réelle) porte exactement les mêmes écarts que "
                 "TableC.json : les deux sont cohérentes entre elles. TableC.json n'est pas "
                 "un instantané périmé.", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["⚠ LE RISQUE EST DIFFÉRÉ, PAS ABSENT.", "", "", "", ""], ST_ALERTE)
ajoute_ligne(t, ["Le jour où SF_Init_4 sera écrit pour de bon (lot D de la synthèse d'état), "
                 "il lira les noms de propriétés d'enrichedVars pour construire les trames. "
                 "Ce tableau est alors la liste exacte de ce qu'il faut avoir corrigé avant. "
                 "Deux chantiers à ne pas confondre : aligner les noms (ce tableau, lot F) "
                 "et décider du modèle de données attendu par SF_Init_4 — il a été écrit "
                 "contre un modèle à abréviations qui n'a jamais existé dans les données.",
                 "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["", "", "", "", ""], ST_NEUTRE)
ajoute_ligne(t, ["VAR", "PROP dans C", "PROP dans A (hypothèse)", "Nature", "Commentaire"], ST_ENTETE)
for i, lg in enumerate(AUDIT):
    ajoute_ligne(t, lg, ST_ALT if i % 2 else ST_CORPS)
ajoute_ligne(t, ["LockSE", "— (variable absente)", "camera, microphone", "variable manquante",
                 "Ajoutée à la Table A après le 18/04. La Table C l'ignore entièrement."], ST_ALERTE)
doc.spreadsheet.addElement(t)

# ---------------------------------------------------------------------------
# 4. LES QUATRE FEUILLES DE DONNEES
# ---------------------------------------------------------------------------
feuille("TableA", ENT_A, LIG_A,
        ["18mm","22mm","26mm","14mm","20mm","30mm","22mm","24mm","24mm",
         "26mm","20mm","20mm","22mm","42mm","60mm"])

feuille("TableC", ENT_C, LIG_C,
        ["20mm","30mm","55mm","24mm","24mm","24mm","30mm","55mm","18mm","34mm","40mm"])

feuille("TableD", ENT_D, LIG_D,
        ["14mm","34mm","26mm","14mm","24mm","20mm","40mm","20mm","20mm",
         "22mm","22mm","24mm","34mm","40mm","18mm","50mm"])

t = Table(name="Legende")
for l in ("32mm", "26mm", "120mm"):
    t.addElement(TableColumn(stylename=colonne(l)))
ajoute_ligne(t, [TITRE_LEG, "", ""], ST_SOUSTIT)
ajoute_ligne(t, ENT_L, ST_ENTETE)
for i, lg in enumerate(LIG_L):
    ajoute_ligne(t, (list(lg) + ["", "", ""])[:3], ST_ALT if i % 2 else ST_CORPS)
doc.spreadsheet.addElement(t)

doc.save(SORTIE)
print(f"\nOK — {SORTIE}")
print(f"   _Version · TableA {len(LIG_A)} · TableC {len(LIG_C)} · "
      f"TableD {len(LIG_D)} · Legende {len(LIG_L)}")
