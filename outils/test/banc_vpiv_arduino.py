#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  Fichier    : banc_vpiv_arduino.py
#  Chemin     : outils/test/banc_vpiv_arduino.py
#  Projet     : RobotRZ — banc de test du protocole VPIV, côté Arduino Mega
#
#  OBJECTIF
#    Éprouver les 72 commandes SP→A de la Table A contre la carte réelle,
#    SANS capteur branché et SANS robot monté. Pour chaque commande : envoyer
#    la trame, lire la réponse, la confronter au FORMAT_VALUE déclaré.
#
#  POURQUOI CE BANC EST POSSIBLE AUJOURD'HUI, ET PAS LA SEMAINE DERNIERE
#    La Table A corrigée le 22/10 n'est plus seulement une référence : c'est
#    un ORACLE. Ses 253 lignes portent toutes un FORMAT_VALUE exploitable et
#    251 un exemple de trame complet. Le jeu de tests était donc deja ecrit ;
#    il manquait le programme qui le deroule. C'est ce fichier.
#    Avant la correction, un tel banc aurait valide l'erreur au lieu de la
#    trouver : il aurait compare la carte a une table fausse a 61 %.
#
#  CE QU'IL TESTE, ET CE QU'IL NE PEUT PAS TESTER
#    ✅ la couche PROTOCOLE : la carte accepte-t-elle la trame, repond-elle,
#       et sa reponse respecte-t-elle le format declare ?
#    ❌ les VALEURS physiques : une distance ultrason, un cap, une force.
#       Elles demandent les capteurs, donc le robot monte. Voir
#       docs/travaux_robot_assemble.md.
#
#  🔴 PRECAUTION DE SECURITE — LIRE AVANT DE LANCER
#    Certaines commandes FONT BOUGER le materiel : Mtr.cmd fait tourner les
#    moteurs, Srv.angle deplace un servo. Si la Mega est reliee a l'UNO et
#    que les moteurs sont alimentes, le robot peut partir.
#    Par defaut ces commandes sont ECARTEES. Il faut --avec-mouvement pour
#    les inclure, et c'est a faire chassis cale, roues en l'air.
#
#  USAGE
#    # 1) a blanc, sans materiel : montre ce qui serait envoye
#    python3 banc_vpiv_arduino.py --table <TableA.csv> --simulation
#
#    # 2) sur la carte, commandes sans effet mecanique
#    python3 banc_vpiv_arduino.py --table <TableA.csv> --port COM5
#
#    # 3) tout, y compris ce qui bouge — chassis cale
#    python3 banc_vpiv_arduino.py --table <TableA.csv> --port COM5 --avec-mouvement
#
#  DEPENDANCE
#    pyserial  (pip install pyserial) — uniquement hors simulation.
#
#  ARTICULATION
#    Lit  : la Table A corrigee (outils/vpiv/TableA_VPIV_EX_corrige.csv)
#    Ecrit: un rapport CSV horodate, a joindre au journal de seance
#    Voir : docs/convention_VPIV_ordre.md  — l'ordre des champs
#           docs/cablage_RZ_depuis_le_code.md §2 — la liaison USB
#
#  Version 1.0 — 24/10/2026 — Vincent Philippe / Claude
# =============================================================================

import argparse, csv, re, sys, time, datetime

# --- Motif d'une trame VPIV, borne a la ligne --------------------------------
#  Borne volontairement : un motif qui traverse les lignes avale les entetes
#  de commentaire et produit de faux resultats. Lecon du 22/10.
MOTIF = re.compile(r'^\$([A-Za-z]+):([A-Za-z_][A-Za-z0-9_]*):'
                   r'([^:#\n]*):([^:#\n]*):([^#\n]*)#$')

# --- Commandes a effet mecanique ---------------------------------------------
#  Ecartees par defaut. La liste est volontairement LARGE : mieux vaut
#  ecarter une commande inoffensive que faire partir le robot.
MOUVEMENT = {
    ('Mtr', 'cmd'), ('Mtr', 'act'), ('Mtr', 'override'), ('Mtr', 'scale'),
    ('Mtr', 'kturn'), ('Mtr', 'targets'),
    ('Srv', 'angle'), ('Srv', 'act'), ('Srv', 'speed'),
    ('Ctrl_L', 'act'), ('Ctrl_L', 'vmax'),
}

# --- Commandes qui PERTURBENT le deroulement du banc -------------------------
#  Elles ne sont pas dangereuses, mais elles cassent la campagne en cours :
#  un reset relance la carte au milieu du test et invalide tout ce qui suit.
#  Ecartees par defaut, et placees EN DERNIER si on les demande.
PERTURBATEURS = {
    ('CfgS', 'reset'): "redemarre la carte — invaliderait la suite du banc",
    ('Odom', 'reset'): "remet la pose a zero — fausse un etalonnage en cours",
}


def charger_table(chemin):
    """Rend la liste des lignes de la Table A."""
    with open(chemin, encoding='utf-8-sig', newline='') as f:
        return list(csv.DictReader(f, delimiter=';'))


def construire_matrice(lignes, avec_mouvement, perturbateurs=False):
    """Construit le jeu de tests : une entree par commande SP→A.

    Pour chaque commande on retient :
      - la trame a envoyer, prise telle quelle dans VPIV_EX (donc deja
        verifiee a l'ordre normatif depuis le 22/10) ;
      - le format attendu en reponse, lu sur la ligne A→SP de meme
        VAR.PROP quand elle existe.

    Les 7 commandes sans retour declare de meme propriete (les read, Mtr.cmd,
    Urg.clear) repondent sur une AUTRE propriete : on les marque, on les
    envoie quand meme, et on note ce qui revient sans le juger.
    """
    retours = {}
    for l in lignes:
        cle = (l['VAR'].strip(), l['PROP'].strip())
        if l['DIRECTION'].strip() == 'A→SP':
            retours.setdefault(cle, l)

    matrice, ecartees = [], []
    for l in lignes:
        if l['DIRECTION'].strip() != 'SP→A' or l['CAT'].strip() != 'V':
            continue
        var, prop = l['VAR'].strip(), l['PROP'].strip()
        trame = (l['VPIV_EX'] or '').strip()
        if not MOTIF.match(trame):
            ecartees.append((var, prop, "pas d'exemple exploitable"))
            continue
        if (var, prop) in MOUVEMENT and not avec_mouvement:
            ecartees.append((var, prop, "effet mecanique — --avec-mouvement"))
            continue
        if (var, prop) in PERTURBATEURS and not perturbateurs:
            ecartees.append((var, prop, PERTURBATEURS[(var, prop)]))
            continue
        ret = retours.get((var, prop))
        matrice.append({
            'perturbateur': (var, prop) in PERTURBATEURS,
            'var': var, 'prop': prop, 'trame': trame,
            'attendu': (ret['FORMAT_VALUE'].strip() if ret else ''),
            'retour_meme_prop': ret is not None,
        })
    # Les perturbateurs passent en dernier : ainsi un reset demande ne
    # casse que lui-meme, et non les cas qui restaient a eprouver.
    matrice.sort(key=lambda c: c['perturbateur'])
    return matrice, ecartees


def juger(reponse, cas):
    """Confronte la reponse de la carte au format declare.

    Trois verdicts seulement, et le doute profite a l'humain :
      OK       la reponse est conforme
      ECART    elle arrive mais ne respecte pas le format
      MUET     rien n'est revenu dans le delai
      A LIRE   la carte repond sur une autre propriete (les 7 cas connus)
    """
    if not reponse:
        return 'MUET', ''
    m = MOTIF.match(reponse)
    if not m:
        return 'ECART', 'reponse non conforme au format VPIV'
    _cat, var, p3, _p4, valeur = m.groups()

    if var != cas['var']:
        return 'ECART', f"variable {var} au lieu de {cas['var']}"
    if not cas['retour_meme_prop']:
        return 'A LIRE', f"repond sur « {p3} » — attendu, a verifier a l'oeil"
    if p3.strip() != cas['prop']:
        return 'ECART', f"propriete « {p3} » au lieu de « {cas['prop']} »"

    # Le format n'est exploitable que si c'est une enumeration de litteraux.
    attendu = cas['attendu']
    jetons = [t.strip() for t in attendu.split('|')] if attendu else []
    litteral = jetons and all(re.fullmatch(r'[A-Za-z0-9_.\-]{1,24}', t) for t in jetons)
    if litteral and re.fullmatch(r'[A-Za-z0-9_.\-]+', valeur.strip()):
        if valeur.strip() not in jetons:
            return 'ECART', f"valeur « {valeur.strip()} » hors de « {attendu} »"
    return 'OK', ''


def main():
    ap = argparse.ArgumentParser(description="Banc de test VPIV — cote Arduino Mega")
    ap.add_argument('--table', required=True, help="la Table A corrigee, en CSV")
    ap.add_argument('--port', help="port serie de la Mega : COM5, /dev/ttyACM0…")
    ap.add_argument('--vitesse', type=int, default=115200)
    ap.add_argument('--simulation', action='store_true',
                    help="n'ouvre aucun port : montre ce qui serait envoye")
    ap.add_argument('--avec-mouvement', action='store_true',
                    help="inclut les commandes qui font bouger le materiel")
    ap.add_argument('--lever-urgence', action='store_true',
                    help="envoie $V:Urg:clear:*:1# avant le banc, si la carte est verrouillee")
    ap.add_argument('--perturbateurs', action='store_true',
                    help="inclut les resets, places en dernier")
    ap.add_argument('--attente', type=float, default=1.0,
                    help="delai de reponse en secondes (defaut 1.0)")
    ap.add_argument('--rapport', default='',
                    help="chemin du rapport CSV (defaut : horodate)")
    a = ap.parse_args()

    if not a.simulation and not a.port:
        ap.error("il faut --port, ou --simulation pour un essai a blanc")

    lignes = charger_table(a.table)
    matrice, ecartees = construire_matrice(lignes, a.avec_mouvement, a.perturbateurs)

    print("=" * 74)
    print("BANC DE TEST VPIV — Arduino Mega" + ("   [SIMULATION]" if a.simulation else ""))
    print("=" * 74)
    print(f"  Table A        : {a.table}  ({len(lignes)} lignes)")
    print(f"  Cas a eprouver : {len(matrice)}")
    print(f"  Ecartes        : {len(ecartees)}")
    if not a.avec_mouvement:
        print("  🔴 Les commandes a effet mecanique sont ECARTEES.")
        print("     --avec-mouvement pour les inclure — chassis cale, roues en l'air.")
    if not a.perturbateurs:
        print("  ⚠️  Les resets sont ECARTES : ils relanceraient la carte en cours de banc.")
        print("     --perturbateurs pour les inclure — ils passeront en dernier.")
    print()

    # --- Simulation : on montre, on n'envoie rien ----------------------------
    if a.simulation:
        print(f"  {'VAR.PROP':<22} {'TRAME':<44} ATTENDU")
        print("  " + "-" * 70)
        for c in matrice:
            att = c['attendu'] if c['retour_meme_prop'] else '(autre propriete)'
            print(f"  {c['var']+'.'+c['prop']:<22} {c['trame']:<44} {att[:24]}")
        if ecartees:
            print(f"\n  ECARTES ({len(ecartees)}) :")
            for v, p, r in ecartees:
                print(f"     {v}.{p:<14} {r}")
        print("\n  Rien n'a ete envoye. Relancer avec --port pour eprouver la carte.")
        return 0

    # --- Essai reel ----------------------------------------------------------
    try:
        import serial
    except ImportError:
        print("  🔴 pyserial absent.  pip install pyserial")
        return 2

    try:
        lien = serial.Serial(a.port, a.vitesse, timeout=a.attente)
    except Exception as e:
        print(f"  🔴 Port {a.port} inaccessible : {e}")
        return 2

    # La Mega redemarre a l'ouverture du port : on laisse passer le boot,
    # sinon ses messages d'initialisation sont pris pour des reponses.
    print("  Ouverture du port, attente du demarrage de la carte (3 s)...")
    time.sleep(3)
    lien.reset_input_buffer()

    # --- Lever l'urgence, si demande ----------------------------------------
    #  Une carte en arret d'urgence refuse TOUT sauf CfgS:emg et Urg:clear.
    #  Sans cela le banc ne mesure qu'une chose : que le filtre fonctionne.
    #  On ne leve jamais d'office : c'est un etat de securite, et il peut
    #  avoir une bonne raison d'etre la.
    if a.lever_urgence:
        print("  Levee de l'urgence demandee : envoi de $V:Urg:clear:*:1#")
        lien.write(b"$V:Urg:clear:*:1#\n")
        time.sleep(0.5)
        echo = []
        t0 = time.time()
        while time.time() - t0 < 2.0:
            l = lien.readline().decode(errors='replace').strip()
            if l.startswith('$') and l.endswith('#'):
                echo.append(l)
        leve = any('cleared' in x for x in echo)
        print(f"     reponse : {echo[0] if echo else '(rien)'}")
        print(f"     {'✅ urgence levee' if leve else '⚠️  non confirmee — le banc continue quand meme'}")
        lien.reset_input_buffer()

    resultats = []
    compte = {}
    for i, c in enumerate(matrice, 1):
        lien.reset_input_buffer()
        lien.write((c['trame'] + "\n").encode())

        # --- Lecture CORRELEE (corrige le defaut du 24/10 au matin) --------
        #  La carte diffuse ses mesures en continu : ultrasons, odometrie,
        #  flux divers. Prendre la PREMIERE trame venue revenait a comparer
        #  la commande a un bulletin de capteur arrive entre-temps — c'est
        #  ce qu'a montre le premier essai, ou 59 cas sur 60 etaient juges
        #  sur du bruit.
        #  On collecte donc tout ce qui arrive pendant le delai, puis on
        #  choisit : d'abord un refus d'urgence, sinon une trame portant la
        #  bonne variable, sinon rien. Le reste est garde comme « bruit »
        #  pour que le rapport en porte la trace.
        t0 = time.time()
        recoltees = []
        reponse, refus = '', ''
        while time.time() - t0 < a.attente:
            ligne = lien.readline().decode(errors='replace').strip()
            if not (ligne.startswith('$') and ligne.endswith('#')):
                continue
            recoltees.append(ligne)
            m = MOTIF.match(ligne)
            if not m:
                continue
            _c, var_r, p3, _p4, _v = m.groups()
            if var_r == 'Urg' and p3.strip() == 'lock':
                refus = ligne          # commande refusee : urgence active
                break
            if var_r == c['var']:
                reponse = ligne        # la vraie reponse, enfin correlee
                break

        if refus:
            verdict, note = 'REFUSE', "urgence active — seuls CfgS:emg et Urg:clear passent"
            reponse = refus
        else:
            verdict, note = juger(reponse, c)
        bruit = ' | '.join(x for x in recoltees if x != reponse)[:300]
        compte[verdict] = compte.get(verdict, 0) + 1
        marque = {'OK': '✅', 'ECART': '🔴', 'MUET': '⚠️ ',
                  'A LIRE': '👁 ', 'REFUSE': '🔒'}[verdict]
        print(f"  {i:>3}/{len(matrice)} {marque} {c['var']+'.'+c['prop']:<20} "
              f"{c['trame']:<40} -> {reponse or '(rien)'}")
        if note:
            print(f"            {note}")
        resultats.append({**c, 'reponse': reponse, 'verdict': verdict,
                          'note': note, 'bruit': bruit})
    lien.close()

    # --- Rapport --------------------------------------------------------------
    horo = datetime.datetime.now().strftime('%Y-%m-%d_%Hh%M')
    chemin = a.rapport or f"rapport_banc_vpiv_{horo}.csv"
    with open(chemin, 'w', encoding='utf-8-sig', newline='') as f:
        w = csv.DictWriter(f, delimiter=';', lineterminator='\r\n',
                           fieldnames=['var', 'prop', 'trame', 'attendu',
                                       'retour_meme_prop', 'perturbateur',
                                       'reponse', 'verdict', 'note', 'bruit'],
                           extrasaction='ignore')
        w.writeheader()
        w.writerows(resultats)

    print("\n" + "=" * 74)
    print("BILAN")
    print("=" * 74)
    for v in ('OK', 'A LIRE', 'REFUSE', 'ECART', 'MUET'):
        if v in compte:
            print(f"  {v:<10} {compte[v]:>3}")
    print(f"\n  Rapport : {chemin}")
    print("\n  Lecture :")
    print("    ✅ OK      la carte accepte et repond au format declare")
    print("    👁  A LIRE  elle repond sur une autre propriete — les 7 cas connus")
    print("    🔴 ECART   la reponse contredit la Table A : a instruire")
    print("    🔒 REFUSE  urgence active — la carte repond $E:Urg:lock:*:STOP actif#")
    print("    ⚠️  MUET    rien dans le delai")
    if compte.get('REFUSE'):
        print()
        print("  🔒 LA CARTE EST EN ARRET D'URGENCE.")
        print("     Les commandes refusees ne sont PAS en defaut : le filtre a fait")
        print("     son office. Lever l'urgence, puis relancer le banc :")
        print("         python outils/test/banc_vpiv_arduino.py --table ... --port ... --lever-urgence")
    return 1 if compte.get('ECART') else 0


if __name__ == '__main__':
    sys.exit(main())
