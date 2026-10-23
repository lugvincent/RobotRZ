# RobotRZ — synthèse générale

> **Fichier** : `docs/synthese_generale_RobotRZ.md`
> **Objet** : l'entrée unique du projet. Ce qu'est RobotRZ, comment il est fait,
> et **où trouver chaque chose**. C'est le document à ouvrir en premier, après
> une interruption ou pour s'y retrouver.
>
> **⚠ À ne pas confondre avec `docs/synthese_etat.md`.** Celui-ci dit ce que le
> projet **est** — il bouge peu. L'autre dit **où il en est** — il change à
> chaque séance. En cas de doute sur l'avancement, c'est l'autre qui fait foi.
>
> **Ce qu'il ne fait pas** : recopier les documents de référence. Chaque
> chapitre donne l'essentiel et renvoie. Recopier, c'est fabriquer la
> divergence silencieuse — deux textes qui disent la même chose jusqu'au jour
> où l'un est corrigé et pas l'autre. Le projet en a assez souffert.
>
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe

---

## 1. Le projet en une page

RobotRZ est un **robot compagnon mobile**, piloté à distance ou en autonomie,
développé bénévolement par Vincent Philippe.

Son intelligence est répartie entre **quatre sous-systèmes** qui échangent tous
dans un **protocole unique, VPIV**, transporté par MQTT et par liaison série.

| | Rôle | Technologie |
|---|---|---|
| **SP** — Système Pilote | **Maître absolu.** Décide de tous les modes, envoie la configuration, porte l'interface, traite les alertes | Node-RED sur RPi 3 · secours sur Xiaomi |
| **SE** — Smartphone Embarqué | Cerveau local. Traite les commandes de SP, gère les capteurs du téléphone, reconnaissance vocale, relaie vers Arduino | Android · Termux · Tasker |
| **A** — Arduino Mega | Contrôleur temps réel des capteurs et actionneurs physiques | C++ · PlatformIO |
| **UNO** | Esclave moteur exclusif, invisible depuis SE et SP | C++ |

```
        SP  (Node-RED, RPi 3 — 192.168.1.40)
         ↕  MQTT          topics SE/commande et SE/statut
        SE  (Doro — Termux, Tasker ; pas de carte SIM)
         ↕  USB-C série
      Mega  (capteurs, actionneurs, sécurité)
         ↕  Serial3, format <L,R,A>
       UNO  (moteurs)
```

**Trois principes gouvernent l'ensemble**, et ils expliquent la plupart des
choix de conception :

1. **SP est la seule source de vérité logique.** Aucune décision non
   strictement matérielle n'est prise ailleurs.
2. **Chaque appareil gère ses tâches bas niveau** et n'échange que le
   nécessaire.
3. **L'Arduino garde une sécurité autonome.** Il arrête les moteurs sans
   attendre SP, et ne les réactive jamais de lui-même.

---

## 2. L'index — où trouver quoi

### Les documents de référence, dans le dépôt

| Document | Répond à |
|---|---|
| **`BRANCHES.md`** *(racine)* | **Quelle branche est laquelle.** `main` seule sert au développement ; `master`, `vMars` et la sauvegarde sont figées |
| **`synthese_etat.md`** | **Où en est le projet ?** Avancement, lots, bugs ouverts, pièges connus. Change à chaque séance |
| `tables_RZ_descripteur.md` | Les quatre tables, les quinze champs de la Table A, les règles du protocole VPIV |
| 🔴 **`convention_VPIV_ordre.md`** | **Pourquoi l'ordre des champs a été faux partout, et ce qui fait foi désormais.** À lire avant de recopier le moindre exemple de trame |
| `reference_firmware_arduino.md` | Le code Arduino : organisation, boucle, traitement d'une trame, urgences, pièges |
| `reference_SE_termux_tasker.md` | Le code SE : démarrage, parser VPIV, FIFOs, état, managers, chaîne vocale, pièges |
| `reference_Tasker.md` | La couche Android : démarrage du robot, routeur, arbitrage vocal, scènes, pièges |
| `genese_et_evolution_RZ.md` | Pourquoi ce robot existe, et comment il est passé de la V0 à la V6 |
| `installation_RPI_serveur.md` | Installer le RPi serveur de zéro — réseau, Node-RED, Mosquitto, accès distant |
| `travaux_robot_assemble.md` | Tout ce qui exige le robot monté, groupé en une campagne |
| `cablage_RZ_depuis_le_code.md` | Les trois liaisons, le brochage de la Mega, les pièges de montage — **extrait du code, régénérable** |
| `schemas_SE.md` | Trois schémas du SE : circuit d'une trame, routage vocal, capteurs et verrous — sources Mermaid dans `docs/schemas/` |
| `dev_RZ_HTML/index.html` | Le site de documentation, une vingtaine de pages thématiques |
| `dev_RZ_HTML/vademecum_documentation_V1.html` | Où ranger un document, lequel fait foi en cas d'écart |

### Les données

| | Où |
|---|---|
| Table A, C, D et leur légende | `AA-Doc-horsGit\A - Architecture_Tables_Schemas\Tables\TablesRZ_reference.ods` |
| Table B (runtime Node-RED) | `node-red/config/globalSave.json` — générée, ne pas éditer à la main |
| Flux Node-RED | `node-red/flows/` — un export daté par séance |
| Firmware | `src/mega/`, `src/uno/`, `lib/RZlibrairiesPersoNew/` |
| Scripts SE | `termux/` |
| Tasker | `tasker/` |
| 🔒 Identifiants | `AA-Doc-horsGit\Z - Secret\RZ_Codes_Identifiants.odt` — et nulle part ailleurs |

### Les questions fréquentes

| | |
|---|---|
| Le nom exact d'une variable ou d'une propriété | Table A |
| Ce que le firmware accepte réellement | `reference_firmware_arduino.md` §9 |
| Pourquoi tel choix a été fait | `genese_et_evolution_RZ.md` |
| Un piège déjà rencontré | `synthese_etat.md`, section « Pièges à ne pas réveiller » |
| Où ranger un document | Le vade-mecum, ou `AA-Doc-horsGit\LISEZ-MOI.md` |

---

## 3. Le protocole VPIV

Tout message échangé, quel que soit le support, suit ce format :

```
$CAT:VAR:PROP:INST:VAL#
```

| Champ | Rôle | Exemples |
|---|---|---|
| **CAT** | Type du message | `V` consigne · `I` info et ACK · `E` urgence · `F` flux · `A` application |
| **VAR** | Module concerné | `Gyro`, `Mtr`, `CfgS`, `Urg` |
| **PROP** | Propriété | `modeRZ`, `angle`, `dataContCpu` |
| **INST** | Instance, ou `*` | `TGD`, `rear`, `*` |

> 🔴 **La propriété précède l'instance, et c'est le piège numéro un du projet.**
> La convention a changé en cours de route, mais la colonne `VPIV_EX` de la Table A
> n'a pas suivi : 150 de ses 251 exemples sont restés à l'ancien ordre
> `$CAT:VAR:INST:PROP:VAL#`. Comme c'est la colonne que tout le monde lit, l'erreur
> s'est propagée aux commentaires du firmware, aux documents archivés, aux légendes,
> et jusqu'au **code de réception de la Mega — qui ne pouvait donc recevoir aucune
> commande**. Tout a été corrigé le 22/10/2026.
>
> **Avant de recopier un exemple de trame, vérifier sa source** : `convention_VPIV_ordre.md` §5.1
> donne le tableau de confiance. En deux mots — le dépôt est fiable depuis le 22/10,
> `AA-Doc-horsGit/` ne l'est pas.
| **VAL** | Valeur | `3`, `90`, `[x,y,z]` |

> 🔴 **PROP en position 3, INST en position 4.** Ne jamais intervertir. Cette
> inversion est l'erreur la plus répandue du projet : elle a contaminé deux
> fichiers de légende, les deux tiers d'un document Arduino, et le code de
> réception du firmware lui-même, où elle empêchait la Mega de recevoir la
> moindre commande jusqu'au 22/10/2026.

**Deux autres règles absolues :**

- **Jamais de nombre à virgule sur le fil.** Entiers multipliés par 10, 100 ou
  1000 selon la précision voulue.
- **`VPIV_MAX_MSG_LEN = 128`** — `config/config.h` ligne 18.

Détail complet : `tables_RZ_descripteur.md` §5.

---

## 4. Les quatre tables

| | Rôle | Où elle vit |
|---|---|---|
| **A** | **Référence normative.** Toutes les variables VPIV, leurs propriétés, formats, instances, directions. 253 lignes, 15 colonnes. **Jamais chargée à l'exécution** — c'est un document de conception | PC, dans Calc |
| **B** | Image de travail de Node-RED (`enrichedVars`), générée depuis la Table C | `globalSave.json`, RPi |
| **C** | Modèle de persistance et d'initialisation. 23 variables, 80 propriétés. Éditée par l'onglet `ui_VarG` | `TableC.json`, sur SP |
| **D** | Catalogue des commandes vocales. 72 commandes, 16 colonnes | `tableD_catalogueV2.csv`, sur SE |

```
TableC.json  →  globalSave.json (B)  →  courant_init.json (projection vers SE)
```

**L'autorité**, règle du 18/10/2026 :

> Le **PC** est l'atelier de conception : la Table A y vit, en tête de chaîne.
> Le **RPi** porte l'autorité d'exécution pour B, C et D.
> Chaque sous-système garde une **copie locale persistante** qui lui permet de
> démarrer seul ; SP l'écrase après `SE_READY`.
> Le **Xiaomi** reçoit un clone daté **avant** toute panne, jamais pendant.

---

## 5. Les sous-systèmes

### 5.1 SP — le maître

Node-RED sur Raspberry Pi 3, à `192.168.1.40`. Sept onglets :

| Onglet | Rôle |
|---|---|
| `Gtion_Init` | Initialisation — charge `enrichedVars` |
| `RZ_GestionFlux_V3` | Bus VPIV : parsing, routage, encodage |
| `Gtion_EtatGlobal_V2` | Règles métier, décisions de `modeRZ` |
| `ui_Accueil_v4` | Interface utilisateur |
| `ui_VarG` | Éditeur de la Table C — page `/admin_varG` |
| `Diag_SE_Statut` | Diagnostic SE en temps réel |
| `B2_Variables_Complexes` | Variables structurées |

`RZ_GestionFlux_V3` alimente `global.RZ_<VAR>` à chaque trame reçue. Accès type :
`global.get('RZ_Sys').dataContCpu._last`.

Un clone de secours tourne sur le Xiaomi. **Ce n'est pas un clone exact** : six
onglets n'existent que là, et leur rapatriement est en cours — voir
`synthese_etat.md` §4.9 et §4.10.

### 5.2 SE — le cerveau local

Android + Termux, sans carte SIM. Se connecte par Wi-Fi, ou par partage de
connexion.

**Deux voies, et la séparation est nette.** Termux porte la communication, les
capteurs et la logique ; Tasker porte tout ce qui touche aux applications
Android.

Scripts centraux :

| Script | Rôle |
|---|---|
| `rz_vpiv_parser.sh` | Démon MQTT ↔ FIFO |
| `rz_state-manager.sh` | État global, `global.json` |
| `mqtt_bridge.py` | Pont vers l'Arduino par USB-C |
| `safety_stop.sh` | Arrêt d'urgence |

Les **FIFOs** sont les tuyaux internes entre processus, sans passer par MQTT :
`fifo_termux_in`, `fifo_tasker_in`, `fifo_appli_in`, `fifo_voice_in`,
`fifo_return`.

Les **managers de capteurs** — gyroscope, magnétomètre, système, caméra, micro,
reconnaissance vocale, synthèse vocale, torche — suivent tous le même modèle :
une boucle non bloquante, un mode de fonctionnement, une publication VPIV.

`global.json` est la mémoire partagée de SE : tous les managers y lisent et y
écrivent, et c'est la source de vérité de l'état courant côté embarqué.

### 5.3 Arduino — le temps réel

Mega 2560 pour les capteurs, actionneurs et la sécurité. UNO esclave pour les
moteurs seuls.

Toute l'organisation du code obéit à une **règle de trois couches** —
matériel, métier, communication — et aucun dispatcher ne calcule ni ne touche
au matériel.

Détail complet : `reference_firmware_arduino.md`.

> Le pendant SE est `reference_SE_termux_tasker.md`.

### 5.4 Tasker — les applications Android

Gère ce que seul Android sait faire : Zoom, caméra de surveillance, GPS,
expressions faciales sur l'écran, synthèse vocale. Reçoit des trames de
catégorie `A`, que l'Arduino ignore totalement.

> 🔴 **C'est Tasker qui démarre le robot.** La tâche `RZ_Init_Tasker` lance
> `rz_start.sh` par le greffon Termux:Tasker, attend les démons, puis met en
> place l'affichage. Un SE qui ne démarre pas se diagnostique d'abord ici.

Détail complet : `reference_Tasker.md`.

---

## 6. Les invariants

Ce qui ne se négocie pas, quelle que soit la partie du projet.

**Protocole.** `$CAT:VAR:PROP:INST:VAL#`, PROP en 3, INST en 4. Jamais de
flottant sur le fil. 128 caractères au maximum.

**Autorité.** SP décide. L'Arduino n'obéit qu'à SP, mais garde sa sécurité
autonome : il arrête les moteurs de lui-même et **ne les réactive jamais sans
ordre explicite**.

**Sécurité.** Les identifiants vivent dans un seul fichier, dans `Z - Secret`,
hors dépôt et chiffré. Ne jamais afficher le résultat de `git remote -v` — le
jeton y figure en clair. Ne jamais publier une adresse IPv6 : contrairement à
l'IPv4, elle désigne la machine et ne passe par aucun NAT.

**Documentation.** Le thème décide du dossier, l'âge décide de la place à
l'intérieur. Une version dépassée descend dans le `old\` de sa propre lettre.

**Node-RED.** Exporter les flux en fin de chaque séance, sans exception.
Node-RED ne versionne rien.

---

## 7. Le système documentaire

Quatre emplacements, quatre rôles distincts :

| | Rôle |
|---|---|
| **Dépôt Git** `lugvincent/RobotRZ` | Code, et les documents de référence. **Public** |
| **`AA-Doc-horsGit\`** | Documentation de travail, rangée par lettres. **Hors dépôt**, contient les identifiants |
| **Project Knowledge** | Copie vivante de la synthèse d'état, lue par toutes les conversations |
| **Site HTML** `docs/dev_RZ_HTML/` | Une vingtaine de pages thématiques, consultables hors ligne |

Les lettres de `AA-Doc-horsGit` :

```
A Architecture, tables, schémas    ·  B SP Node-RED     ·  C SE Termux et Tasker
D Arduino                          ·  E sas d'échange   ·  F dev en cours
H Outils                           ·  I Archive         ·  J Électronique et câblage
K Documents utilisateurs           ·  L RZ-v1 sur RPi4  ·  M Bilans de séance
Z 🔒 Secret
```

Détail et procédures : `dev_RZ_HTML/vademecum_documentation_V1.html`.

---

## 8. Ce qui reste à verser dans ce document

Ce document est une charpente complète et un index exact. **Il n'est pas encore
l'agglomération promise** : cinq documents du dossier A portent de la matière
unique qui n'a pas d'autre foyer, et qui n'y est pas encore entrée.

| Source | Ce qu'elle apporterait | Chapitre visé |
|---|---|---|
| `synthèseRZ.odt` *(103 Ko, 133 titres)* | Le plus riche du dossier. Règles de mise en forme Table A, modèle URG complet, séquences VPIV, rôle de CfgS dans la sécurité | 3, 4, 6 |
| `urgence and co.odt` *(67 Ko)* | Gestion des urgences et des erreurs, vue SE et vue Arduino | nouveau chapitre « Urgences » |
| ~~`Synhtèse générale SE-02-26.odt`~~ | ✅ Couvert depuis les sources par `reference_SE_termux_tasker.md` — reste à confronter pour ce qu'il ajouterait | 5.2 |
| `synthèse Communication robot.odt` | La chaîne Arduino → USB → SE → MQTT → Node-RED, pas à pas | 1, 5 |
| `descriptionRZ.odt` | Rôle des variables centrales — CfgS, Urg, COM | 4, 6 |

**La méthode qui a fonctionné** jusqu'ici, et qu'il faut garder : lire la
source, confronter au code ou aux tables, ne verser que ce qui est vérifié, et
consigner les écarts trouvés. C'est ainsi qu'ont été repérés le nom fantôme de
la Table D, l'erreur sur la Table C, `Odom.coeff` manquant, et le défaut de
réception du firmware.

Ne jamais verser sans vérifier : trois des cinq documents ci-dessus se sont
déjà révélés faux sur des points précis.

---

*V1 — 22/10/2026. Écrit après trois jours de remise en ordre de la
documentation : le dossier A trié de 29 à 14 fichiers, quatre erreurs de fond
corrigées, les tables rassemblées en un classeur unique, et cinq documents de
référence créés dans le dépôt. Ce document est leur sommaire.*
