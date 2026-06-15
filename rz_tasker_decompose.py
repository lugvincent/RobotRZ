#!/usr/bin/env python3
# =============================================================================
# SCRIPT  : rz_tasker_decompose.py
# CHEMIN  : (outil de développement, à exécuter côté PC/SSHD)
#
# OBJECTIF
# --------
# Décompose un fichier backup Tasker global (userbackup.xml / backup.xml)
# en fichiers individuels : tâches, profils, scènes.
# Ces fichiers peuvent ensuite être copiés dans l'arborescence SSHD du projet.
#
# UTILISATION
# -----------
#   python3 rz_tasker_decompose.py <fichier_backup.xml> [dossier_sortie]
#
# SORTIE
# ------
#   dossier_sortie/
#     tasks/       taskNNN_NomTache.tsk.xml
#     profiles/    profNNN_NomProfil.prf.xml
#     scenes/      sceneNomScene.scn.xml
#     summary.txt  liste de ce qui a été extrait
#
# FORMAT DES FICHIERS GÉNÉRÉS
# ---------------------------
# Chaque fichier est un XML autonome lisible par Tasker :
#   <TaskerData sr="" dvi="1" tv="X.X.X">
#     <Task sr="taskNNN">...</Task>
#   </TaskerData>
#
# AUTEUR  : Script généré pour RobotRZ — Vincent Philippe
# VERSION : 1.0
# DATE    : 2026-05-16
# =============================================================================

import xml.etree.ElementTree as ET
import os
import sys
import re

# --------------------------------------------------------------------------
# Configuration
# --------------------------------------------------------------------------
INDENT_XML = True   # Indenter la sortie XML (lisibilité)

# --------------------------------------------------------------------------
# Utilitaires
# --------------------------------------------------------------------------

def sanitize_name(name):
    """Convertit un nom Tasker en nom de fichier sûr."""
    return re.sub(r'[^a-zA-Z0-9_\-]', '_', name) if name else 'unnamed'

def indent_xml(elem, level=0):
    """Indentation récursive pour lisibilité."""
    indent = "\n" + "\t" * level
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = indent + "\t"
        if not elem.tail or not elem.tail.strip():
            elem.tail = indent
        for child in elem:
            indent_xml(child, level + 1)
        # Dernier enfant via accès direct
        last_child = list(elem)[-1]
        if not last_child.tail or not last_child.tail.strip():
            last_child.tail = indent
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = indent

def make_wrapper(tv="6.6.20"):
    """Crée l'élément racine TaskerData."""
    root = ET.Element("TaskerData")
    root.set("sr", "")
    root.set("dvi", "1")
    root.set("tv", tv)
    return root

def write_xml(filepath, root_element, child_element, tv="6.6.20"):
    """Écrit un fichier XML autonome avec l'enveloppe TaskerData."""
    wrapper = make_wrapper(tv)
    wrapper.append(child_element)
    if INDENT_XML:
        indent_xml(wrapper)
    tree = ET.ElementTree(wrapper)
    ET.indent(tree, space="\t")
    with open(filepath, 'wb') as f:
        f.write(b'<?xml version="1.0" encoding="utf-8"?>\n')
        tree.write(f, encoding='utf-8', xml_declaration=False)
    return filepath

# --------------------------------------------------------------------------
# Décomposition principale
# --------------------------------------------------------------------------

def decompose(backup_path, output_dir=None):
    if not os.path.isfile(backup_path):
        print(f"ERREUR : fichier introuvable : {backup_path}")
        sys.exit(1)

    if output_dir is None:
        base = os.path.splitext(backup_path)[0]
        output_dir = base + "_decomposed"

    # Créer les dossiers de sortie
    tasks_dir    = os.path.join(output_dir, "tasks")
    profiles_dir = os.path.join(output_dir, "profiles")
    scenes_dir   = os.path.join(output_dir, "scenes")
    for d in [tasks_dir, profiles_dir, scenes_dir]:
        os.makedirs(d, exist_ok=True)

    tree = ET.parse(backup_path)
    root = tree.getroot()
    tv = root.get("tv", "6.6.20")   # Version Tasker du backup

    summary_lines = [
        f"=== Décomposition de : {os.path.basename(backup_path)} ===",
        f"Version Tasker : {tv}",
        f"Dossier sortie : {output_dir}",
        ""
    ]

    # ---- TÂCHES ----
    summary_lines.append("--- TÂCHES ---")
    task_count = 0
    for task in root.findall('.//Task'):
        sr = task.get('sr', f'task{task_count}')
        name_el = task.find('nme')
        name = name_el.text.strip() if (name_el is not None and name_el.text) else None

        # Nom de fichier
        if name:
            filename = f"{sr}_{sanitize_name(name)}.tsk.xml"
        else:
            # Tâche interne sans nom (bouton de scène, etc.)
            filename = f"{sr}_internal.tsk.xml"
            name = f"(interne {sr})"

        filepath = os.path.join(tasks_dir, filename)
        write_xml(filepath, root, task, tv)
        summary_lines.append(f"  {filename}  ← {name}")
        task_count += 1

    summary_lines.append(f"  Total : {task_count} tâches\n")

    # ---- PROFILS ----
    summary_lines.append("--- PROFILS ---")
    prof_count = 0
    for prof in root.findall('.//Profile'):
        sr = prof.get('sr', f'prof{prof_count}')
        name_el = prof.find('nme')
        name = name_el.text.strip() if (name_el is not None and name_el.text) else f'profil_{sr}'

        filename = f"{sr}_{sanitize_name(name)}.prf.xml"
        filepath = os.path.join(profiles_dir, filename)
        write_xml(filepath, root, prof, tv)
        summary_lines.append(f"  {filename}  ← {name}")
        prof_count += 1

    summary_lines.append(f"  Total : {prof_count} profils\n")

    # ---- SCÈNES ----
    summary_lines.append("--- SCÈNES ---")
    scene_count = 0
    for scene in root.findall('.//Scene'):
        sr = scene.get('sr', f'scene{scene_count}')
        name_el = scene.find('nme')
        name = name_el.text.strip() if (name_el is not None and name_el.text) else sr

        filename = f"{sanitize_name(name)}.scn.xml"
        filepath = os.path.join(scenes_dir, filename)
        write_xml(filepath, root, scene, tv)
        summary_lines.append(f"  {filename}  ← {name}")
        scene_count += 1

    summary_lines.append(f"  Total : {scene_count} scènes\n")

    # ---- RÉCAPITULATIF ----
    summary_lines.append(f"=== Total : {task_count} tâches, {prof_count} profils, {scene_count} scènes ===")
    summary_text = "\n".join(summary_lines)
    
    summary_path = os.path.join(output_dir, "summary.txt")
    with open(summary_path, 'w', encoding='utf-8') as f:
        f.write(summary_text)

    print(summary_text)
    print(f"\nFichiers créés dans : {output_dir}")
    return output_dir

# --------------------------------------------------------------------------
# Point d'entrée
# --------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 rz_tasker_decompose.py <backup.xml> [dossier_sortie]")
        print("Exemple: python3 rz_tasker_decompose.py backup3.xml ./output_tasker")
        sys.exit(1)
    
    backup_file = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else None
    
    decompose(backup_file, out_dir)