# Travaux exigeant le robot assemblé

> **Fichier** : `docs/travaux_robot_assemble.md`
> **Objet** : registre de tout ce qui ne peut pas être fait sur établi, et qui
> attend que le robot soit monté, câblé et alimenté.
> **Pourquoi ce fichier existe** : chacun de ces travaux coûte le même
> préalable — démonter la génération précédente, remonter, recâbler. Les
> découvrir un par un ferait payer ce préalable autant de fois. Groupés, ils
> tiennent en une campagne.
> **Articulation** : complète `docs/synthese_etat.md`, qui donne l'état
> d'ensemble, et `docs/installation_RPI_serveur.md` pour la partie serveur.
> **Précaution** : ce fichier se tient à jour **au fil de l'eau**. Dès qu'un
> travail s'avère dépendre du matériel, il vient ici, même si son tour est
> lointain.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe

---

## 0. Le préalable

La génération actuelle n'est pas montée. Le robot en service est la version
précédente, sur **RPi 4 et une autre carte** — c'est elle que décrit le dossier
`L - RZ-v1 sur RPi4`.

Tout ce qui suit suppose donc, dans l'ordre :

1. Démonter la version RPi 4
2. Monter la structure de la génération actuelle
3. Câbler Mega, UNO, capteurs et actionneurs
4. Alimenter et vérifier les tensions avant tout essai logiciel

**Ce préalable est le coût fixe de la campagne.** C'est la raison d'être de ce
registre.

---

## 1. Ce qui est bloqué, et ce que chaque essai trancherait

### 1.1 🔴 Le chien de garde est-il trop sévère ?

**Question** : `main.cpp` verrouille le système dès qu'une itération de `loop()`
dépasse `LOOP_MAX_MS = 80`. Le 22/10/2026, sur établi, l'urgence
`URG_LOOP_TOO_SLOW_A` s'est déclenchée moins d'une seconde et demie après le
démarrage, à chaque essai.

**Pourquoi l'établi ne tranche pas** : la carte était **sans aucun capteur
connecté**. `NewPing` attend l'expiration complète de son délai pour chacun des
neuf ultrasons quand aucun écho ne revient, et le HX711 du capteur de force
attend une donnée qui ne viendra jamais. Les 80 ms sont dépassées pour cette
seule raison.

**Ce que l'essai dira** : si la boucle tient sous 80 ms capteurs branchés, le
seuil est bon et il n'y a rien à corriger. Sinon, il faut une période de grâce
au démarrage, ou exiger plusieurs dépassements consécutifs.

> À faire **après** la correction de la réception VPIV (§9.5 de la synthèse) :
> tant qu'elle n'est pas appliquée, l'urgence ne peut pas être effacée et
> chaque essai se termine sur un reset.

### 1.2 Les neuf ultrasons

**Question** : chaque capteur répond-il, et à quelle distance ?

Les seuils de `SecMvt` — `warn` et `danger` — n'ont jamais été réglés sur des
mesures réelles. Le lot G de la roadmap place `Mtr` et `US` en premier
précisément parce qu'ils conditionnent les garde-fous.

### 1.3 L'odométrie — trois paramètres à mesurer

`config.h` porte des valeurs **indicatives**, et le dit :

```c
#define ODO_WHEEL_DIAMETER_MM_DEFAULT 100   /* diamètre roue (mm) — valeur indicative */
#define ODO_TRACK_MM_DEFAULT 300            /* entraxe roues (mm) — valeur indicative */
#define ODO_ENCODER_CPR_DEFAULT 256         /* ticks/tour encodeur */
```

**À mesurer sur le robot réel**, puis à saisir via `ui_VarG`. L'onglet
`travail_Calibrage Odometrie` du Xiaomi contient un prototype de procédure —
c'est l'un des six onglets à rapatrier, voir §4.10 de la synthèse.

Une calibration d'odométrie se fait **sur un vrai sol**, pas sur un établi : le
glissement des roues en dépend.

### 1.4 Le capteur de force et la laisse

`FS` expose `tare` et `cal`. Ni l'un ni l'autre n'a de sens sans la laisse
montée et le capteur en charge.

Le pilotage par laisse (`Ctrl_L`) dépend de `FS` pour l'angle et de `US` pour la
distance : il ne peut être réglé qu'une fois les deux étalonnés. Ses paramètres
— `dist`, `vmax`, `dead` — sont à déterminer à l'usage.

### 1.5 Les servomoteurs — vérifier les positions neutres

`config.h` fixe `SERVO_TGD_ZERO 30`, `SERVO_THB_ZERO 70`, `SERVO_BASE_ZERO 0`.
Ces valeurs correspondent à un montage donné : elles sont à revérifier après
remontage, faute de quoi la tête part de travers.

À vérifier aussi : les **deux positions d'équilibre** du tronc, levé et couché,
décidées en V6 pour que la coupure des servos ne fasse pas s'affaisser le robot.
Voir `docs/genese_et_evolution_RZ.md` §5.1.

### 1.6 Les moteurs et le keepalive de l'UNO

`mtr_processPeriodic()` envoie un signal de vie toutes les 100 ms. **L'UNO a un
délai de sécurité de 1000 ms** : sans ce signal, il freine progressivement
jusqu'à l'arrêt. Ce mécanisme n'a jamais été observé en conditions réelles.

À vérifier également : le passage à l'échelle par `SecMvt` (`scale`), et l'arrêt
forcé.

### 1.7 Les trois micros et le détecteur infrarouge

`Mic` expose `bruitNiv1`, `bruitNiv2`, `micAngle`, `micPIC` : des seuils qui ne
veulent rien dire sans mesure du bruit ambiant réel. `MvtIR` de même.

### 1.8 La chaîne complète SP → SE → Arduino

Une fois la réception VPIV corrigée, il faudra vérifier le trajet entier d'une
commande : interface Node-RED → MQTT → `rz_vpiv_parser.sh` → `mqtt_bridge.py` →
USB-C → Mega → actionneur, **et l'ACK en retour**. Jamais fait de bout en bout.

C'est aussi la seule manière de valider le lot D (`SF_Init_4`) une fois écrit.

---

## 2. Ce qui n'a PAS besoin du robot

Pour mémoire, afin de ne pas reporter à tort :

| Travail | Ce qu'il faut |
|---|---|
| Correction de la réception VPIV | La Mega seule, en USB sur le PC |
| Vérifier qu'une commande est acceptée | La Mega seule — un ACK suffit, l'actionneur n'a pas à bouger |
| Table A, Table C, Table D | Rien |
| Node-RED, lots B à F, H à K | Rien, hors validation finale |
| Documentation | Rien |

**La correction du firmware se fait et se vérifie sur établi.** Seul son effet
physique attend le robot.

---

## 3. Suggestion d'ordre pour la campagne

1. Démontage et remontage mécanique
2. Câblage, vérification des tensions **avant** toute mise sous tension logique
3. Positions neutres des servos (§1.5) — avant tout mouvement
4. Les neuf ultrasons (§1.2) — ils conditionnent la sécurité
5. Le chien de garde (§1.1) — maintenant mesurable
6. Moteurs et keepalive UNO (§1.6)
7. Odométrie (§1.3) — sur un vrai sol
8. Force et laisse (§1.4)
9. Micros et infrarouge (§1.7)
10. Chaîne complète SP → SE → Arduino (§1.8)

L'ordre suit une logique : **rien ne bouge avant que la sécurité ne réponde.**

---

*V1 — 22/10/2026. Créé après la découverte du défaut bloquant de réception VPIV
(§9.5 de la synthèse d'état), qui a montré qu'un essai sur établi sans capteurs
peut mener à une conclusion fausse — le chien de garde y paraissait trop sévère
alors que c'est peut-être le montage qui le faisait sonner.*
