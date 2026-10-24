# `outils/test/` — le banc de test du protocole VPIV

> **Ce que la semaine du 20-24 octobre a rendu possible.** La Table A corrigée n'est pas
> seulement une référence : c'est un **oracle**. Ses 253 lignes portent toutes un
> `FORMAT_VALUE` exploitable et 251 un exemple de trame complet. Le jeu de tests était
> donc déjà écrit — il manquait le programme qui le déroule.
>
> *V1 — 24/10/2026 — Vincent Philippe / Claude*

---

## Pourquoi ce banc n'aurait pas pu exister la semaine dernière

Avant la correction du 22/10, 150 des 251 exemples de la Table A portaient l'ancien ordre
des champs. **Un banc construit sur cette table aurait validé l'erreur au lieu de la
trouver** : il aurait comparé la carte à une référence fausse à 61 %, et déclaré conforme
exactement ce qui empêchait la Mega de recevoir la moindre commande.

C'est la raison d'être de la chronologie : corriger la table d'abord, en faire un oracle
ensuite.

---

## Ce qu'il éprouve, et ce qu'il ne peut pas éprouver

| | |
|---|---|
| ✅ **La couche protocole** | La carte accepte-t-elle la trame ? Répond-elle ? Sa réponse respecte-t-elle le format déclaré ? |
| ❌ **Les valeurs physiques** | Une distance ultrason, un cap, une force. Elles demandent les capteurs, donc le robot monté — voir `docs/travaux_robot_assemble.md` |

**Sur les 72 commandes `SP→A` de la Table A, 60 sont éprouvables immédiatement**, sur
établi, sans un seul capteur branché. C'est le gain de la semaine, et il n'était pas
prévu : la campagne « robot monté » rétrécit d'autant.

---

## Mode d'emploi

```bash
# 1) À blanc, sans matériel : montre ce qui serait envoyé, n'envoie rien
python3 banc_vpiv_arduino.py --table ../vpiv/TableA_VPIV_EX_corrige.csv --simulation

# 2) Sur la carte, commandes sans effet mécanique
python3 banc_vpiv_arduino.py --table ../vpiv/TableA_VPIV_EX_corrige.csv --port COM5

# 3) Tout, y compris ce qui bouge — châssis calé, roues en l'air
python3 banc_vpiv_arduino.py --table ../vpiv/TableA_VPIV_EX_corrige.csv --port COM5 \
        --avec-mouvement --perturbateurs
```

Dépendance hors simulation : `pip install pyserial`.

**Commencer toujours par la simulation.** Elle ne touche à rien et montre exactement ce
qui partira.

---

## Les deux garde-fous

### 🔴 Les commandes à effet mécanique sont écartées par défaut

`Mtr.cmd` fait tourner les moteurs, `Srv.angle` déplace un servo. Si la Mega est reliée à
l'UNO et les moteurs alimentés, **le robot peut partir**. Douze commandes sont dans ce cas.
La liste est volontairement large : mieux vaut écarter une commande inoffensive que faire
partir le robot.

### ⚠️ Les resets sont écartés aussi, pour une autre raison

`$V:CfgS:reset:*:1#` **relance la carte au milieu du banc** et invaliderait tout ce qui
suit. `Odom.reset` fausserait un étalonnage en cours. Ils ne sont pas dangereux, ils sont
perturbateurs. Avec `--perturbateurs`, ils passent **en dernier** : un reset demandé ne
casse alors que lui-même.

---

## Lire le verdict

| | |
|---|---|
| ✅ `OK` | la carte accepte et répond au format déclaré |
| 👁 `A LIRE` | elle répond sur une **autre** propriété — les 7 cas connus |
| 🔴 `ECART` | la réponse contredit la Table A : à instruire |
| ⚠️ `MUET` | rien dans le délai — commande refusée, ou urgence active |

### Les sept cas « A LIRE »

`FS.read`, `Mtr.cmd`, `Mtr.read`, `Odom.read`, `Srv.read`, `US.read`, `Urg.clear`.

Ce ne sont pas des lacunes de la Table A : ces commandes **répondent sur une autre
propriété**. Un `read` déclenche un flux ou une mesure, `Urg.clear` répond par
`$I:Urg:etat:*:cleared#`. Le banc les envoie, note ce qui revient, et ne juge pas — c'est
à l'œil humain de confirmer.

### Un `MUET` n'est pas forcément une panne

Une urgence active **bloque toutes les commandes** sauf `CfgS:emg` et `Urg:clear`. Si le
banc rend beaucoup de `MUET`, vérifier d'abord l'état d'urgence de la carte avant de
conclure à un défaut.

---

## Le rapport

Un CSV horodaté est écrit à chaque passage : une ligne par cas, avec la trame envoyée, le
format attendu, la réponse reçue et le verdict. À joindre au journal de séance — c'est lui
qui permettra, dans six mois, de savoir ce qui marchait le jour où.

---

## Ce qui reste à construire

| Banc | État |
|---|---|
| Arduino Mega, par l'USB | ✅ ce fichier |
| SE, par injection dans `fifo_termux_in` | ⏳ à écrire — 89 couples `SE↔SP` |
| SP, par publication sur `SE/commande` | ⏳ **après** le chantier Node-RED : des tests visant SP se périmeraient à mesure qu'il est remanié |
