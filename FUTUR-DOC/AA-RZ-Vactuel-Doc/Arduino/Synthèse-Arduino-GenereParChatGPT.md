**Architecture Firmware VPIV – RZ- Partie Arduino**

**Titre :**  
**Architecture Firmware du Robot RZ**  
**Communication VPIV – Modules, Sécurité et Pilotages**

**Version :** 1.0 – Livrable final  
**Langue :** Français  
**Périmètre :** Firmware embarqué (Arduino Mega / équivalent)  
**Exclusions explicites :** IMU, SLAM, cartographie  


**Table des matières**

[Périmètre technique du projet	4](#__RefHeading___Toc109278_2626828182)

[1. Introduction générale	5](#__RefHeading___Toc79138_2626828182)

[1.1 Objectif du document	5](#__RefHeading___Toc79140_2626828182)

[1.2 Philosophie générale	5](#__RefHeading___Toc79142_2626828182)

[1.3 Connexions	5](#__RefHeading___Toc109280_2626828182)

[2. Vue d’ensemble de l’architecture	6](#__RefHeading___Toc79144_2626828182)

[2.1 Découpage en couches	6](#__RefHeading___Toc79146_2626828182)

[2.2 Arborescence logique du projet	6](#__RefHeading___Toc79148_2626828182)

[3. Protocole VPIV	7](#__RefHeading___Toc79150_2626828182)

[3.1 Rôle du protocole VPIV	7](#__RefHeading___Toc79152_2626828182)

[3.2 Format canonique d’une trame VPIV	7](#__RefHeading___Toc79154_2626828182)

[3.3 Catégories VPIV	7](#__RefHeading___Toc79156_2626828182)

[Clarification critique :	7](#__RefHeading___Toc48193_2774384090)

[3.4 — VAR : module / variable	8](#__RefHeading___Toc109282_2626828182)

[3.5 - PROP : propriété	8](#__RefHeading___Toc109284_2626828182)

[Règles	8](#__RefHeading___Toc109286_2626828182)

[3.6 — INST : instance	8](#__RefHeading___Toc28950_2626828182)

[3.7 — VALUE : valeur	8](#__RefHeading___Toc28952_2626828182)

[3.8 Contraintes de taille et de fréquence	9](#__RefHeading___Toc28888_2626828182)

[4. Règle des 3 couches d’un module (fondamentale)	10](#__RefHeading___Toc79158_2626828182)

[4.1 Couche Hardware	10](#__RefHeading___Toc79160_2626828182)

[4.2 Couche Métier	10](#__RefHeading___Toc79162_2626828182)

[4.3 Couche Communication (Dispatch)	10](#__RefHeading___Toc79164_2626828182)

[5. Module COMMUNICATION (VPIV Core)	11](#__RefHeading___Toc79166_2626828182)

[5.1 Rôle	11](#__RefHeading___Toc79168_2626828182)

[5.2 Parser VPIV	11](#__RefHeading___Toc79170_2626828182)

[5.3 Interaction avec l’urgence (URG)	11](#__RefHeading___Toc79172_2626828182)

[6. Modules Capteurs	11](#__RefHeading___Toc79174_2626828182)

[6.1 Module US – Capteurs ultrasoniques	11](#__RefHeading___Toc79176_2626828182)

[6.1.1 Rôle fonctionnel	11](#__RefHeading___Toc79178_2626828182)

[6.1.2 Architecture interne	12](#__RefHeading___Toc79180_2626828182)

[6.1.3 Couche Hardware	12](#__RefHeading___Toc79182_2626828182)

[6.1.4 Couche Métier	12](#__RefHeading___Toc79184_2626828182)

[6.1.5 Exposition VPIV – US	12](#__RefHeading___Toc79186_2626828182)

[6.2 Module FS – Capteur de force	13](#__RefHeading___Toc79188_2626828182)

[6.2.1 Rôle fonctionnel	13](#__RefHeading___Toc79190_2626828182)

[6.2.2 Architecture interne	13](#__RefHeading___Toc79192_2626828182)

[6.2.3 Couche Hardware – HX711	13](#__RefHeading___Toc79194_2626828182)

[6.2.4 Couche Métier	13](#__RefHeading___Toc79196_2626828182)

[6.2.5 Exposition VPIV – FS	14](#__RefHeading___Toc79198_2626828182)

[6.3 Module MvtIR – Détecteur de mouvement infrarouge	14](#__RefHeading___Toc79200_2626828182)

[6.3.1 Rôle fonctionnel	14](#__RefHeading___Toc79202_2626828182)

[6.3.2 Architecture	14](#__RefHeading___Toc79204_2626828182)

[6.3.3 Modes de fonctionnement	14](#__RefHeading___Toc79206_2626828182)

[6.3.4 Exposition VPIV – MvtIR	14](#__RefHeading___Toc79208_2626828182)

[7. Module CTRL\_L – Pilotage par laisse	15](#__RefHeading___Toc79210_2626828182)

[7.1 Rôle fonctionnel	15](#__RefHeading___Toc79212_2626828182)

[7.2 Dépendances	15](#__RefHeading___Toc79214_2626828182)

[7.3 Correction VPIV (validée)	15](#__RefHeading___Toc79216_2626828182)

[7.4 Paramètres dynamiques	15](#__RefHeading___Toc79218_2626828182)

[7.5 Fonctionnement	15](#__RefHeading___Toc79220_2626828182)

[7.6 Exposition VPIV – Ctrl\_L	16](#__RefHeading___Toc79222_2626828182)

[8. Module SRV – Servomoteurs	16](#__RefHeading___Toc79224_2626828182)

[8.1 Rôle fonctionnel	16](#__RefHeading___Toc79226_2626828182)

[8.2 Architecture interne	16](#__RefHeading___Toc79228_2626828182)

[8.3 Instances et noms symboliques	16](#__RefHeading___Toc79230_2626828182)

[8.4 Couche Métier – srv.cpp	16](#__RefHeading___Toc79232_2626828182)

[8.5 Couche Communication – VPIV	17](#__RefHeading___Toc79234_2626828182)

[9. Module MTR – Moteurs	17](#__RefHeading___Toc79236_2626828182)

[9.1 Rôle fonctionnel	17](#__RefHeading___Toc79238_2626828182)

[9.2 Architecture	17](#__RefHeading___Toc79240_2626828182)

[9.3 Mode Legacy	18](#__RefHeading___Toc79242_2626828182)

[9.4 Mode Modern	18](#__RefHeading___Toc79244_2626828182)

[9.5 Sécurité et scaling	18](#__RefHeading___Toc79246_2626828182)

[9.6 Exposition VPIV – MTR	18](#__RefHeading___Toc79248_2626828182)

[10. Module SecMvt / mvtSafe – Sécurité mouvement	19](#__RefHeading___Toc79250_2626828182)

[10.1 Clarification de nom	19](#__RefHeading___Toc79252_2626828182)

[10.2 Rôle fonctionnel	19](#__RefHeading___Toc79254_2626828182)

[10.3 Modes	19](#__RefHeading___Toc79256_2626828182)

[10.4 Exposition VPIV	19](#__RefHeading___Toc79258_2626828182)

[11. Module URG – Urgences	19](#__RefHeading___Toc79260_2626828182)

[11.1 Rôle fonctionnel	19](#__RefHeading___Toc79262_2626828182)

[11.2 Comportement clé	20](#__RefHeading___Toc79264_2626828182)

[11.3 Exposition VPIV	20](#__RefHeading___Toc79266_2626828182)

[12. Module CfgS – Configuration système	20](#__RefHeading___Toc79268_2626828182)

[12.1 Rôle	20](#__RefHeading___Toc79270_2626828182)

[12.2 Propriétés VPIV	20](#__RefHeading___Toc79272_2626828182)

[13. Communication VPIV – Architecture	21](#__RefHeading___Toc79274_2626828182)

[13.1 Format canonique	21](#__RefHeading___Toc79276_2626828182)

[13.2 Catégories	21](#__RefHeading___Toc79278_2626828182)

[13.3 Règles fondamentales	21](#__RefHeading___Toc79280_2626828182)

[ANNEXE A — SPÉCIFICATION VPIV COMPLÈTE	21](#__RefHeading___Toc79282_2626828182)

[A.1 Objectif de VPIV	21](#__RefHeading___Toc79284_2626828182)

[A.2 Format canonique	22](#__RefHeading___Toc79286_2626828182)

[A.2.1 Détails des champs	22](#__RefHeading___Toc79288_2626828182)

[A.3 Catégories VPIV	22](#__RefHeading___Toc79290_2626828182)

[A.4 Règles strictes	22](#__RefHeading___Toc79292_2626828182)

[A.5 Parsing et robustesse	22](#__RefHeading___Toc79294_2626828182)

[ANNEXE B — TABLE VPIV PAR MODULE	23](#__RefHeading___Toc79296_2626828182)

[Module URG – CFGS Articulation	23](#__RefHeading___Toc48195_2774384090)

[Point beaucoup plus important : Urg vs CfgS (sécurité globale)	23](#__RefHeading___Toc48197_2774384090)

[2.1 — Clarif conceptuelle essentielle	23](#__RefHeading___Toc48199_2774384090)

[🔴 Urg	23](#__RefHeading___Toc48201_2774384090)

[🔵 CfgS	23](#__RefHeading___Toc48203_2774384090)

[2.2 — Analyse de ta table A (très bonne)	24](#__RefHeading___Toc48205_2774384090)

[2.3 — Pourquoi reset doit rester dans CfgS (et pas Urg)	24](#__RefHeading___Toc48207_2774384090)

[Raisons architecturales :	24](#__RefHeading___Toc48209_2774384090)

[2.4 — Interaction Urg ↔ modeRZ (point subtil mais important)	25](#__RefHeading___Toc48211_2774384090)

[3️⃣ Ton dispatch dispatch\_CfgS.cpp : verdict	26](#__RefHeading___Toc48213_2774384090)

[✅ Ce qui est excellent	26](#__RefHeading___Toc48215_2774384090)

[⚠️ Micro-remarque (optionnelle)	26](#__RefHeading___Toc48217_2774384090)

[4️⃣ Conclusion claire (à retenir)	26](#__RefHeading___Toc48219_2774384090)

[B.1 Module CfgS	26](#__RefHeading___Toc79298_2626828182)

[Nom module	26](#__RefHeading___Toc79300_2626828182)

[Propriétés	26](#__RefHeading___Toc79302_2626828182)

[Exemples	27](#__RefHeading___Toc79304_2626828182)

[B.2 Module Ctrl\_L	27](#__RefHeading___Toc79306_2626828182)

[Nom module	27](#__RefHeading___Toc79308_2626828182)

[Propriétés	27](#__RefHeading___Toc79310_2626828182)

[Exemples	27](#__RefHeading___Toc79312_2626828182)

[B.3 Module Mtr	27](#__RefHeading___Toc79314_2626828182)

[Nom module	27](#__RefHeading___Toc79316_2626828182)

[Propriétés (modern)	27](#__RefHeading___Toc79318_2626828182)

[Propriétés (legacy)	28](#__RefHeading___Toc79320_2626828182)

[B.4 Module Srv	28](#__RefHeading___Toc79322_2626828182)

[Nom module	28](#__RefHeading___Toc79324_2626828182)

[Propriétés	28](#__RefHeading___Toc79326_2626828182)

[B.5 Module Odom	28](#__RefHeading___Toc79328_2626828182)

[Nom module	28](#__RefHeading___Toc79330_2626828182)

[Propriétés	28](#__RefHeading___Toc79332_2626828182)

[B.6 Module US	29](#__RefHeading___Toc79334_2626828182)

[Nom module	29](#__RefHeading___Toc79336_2626828182)

[Propriétés	29](#__RefHeading___Toc79338_2626828182)

[B.7 Module FS	29](#__RefHeading___Toc79340_2626828182)

[Nom module	29](#__RefHeading___Toc79342_2626828182)

[Propriétés	29](#__RefHeading___Toc79344_2626828182)

[B.8 Module Mic	29](#__RefHeading___Toc79346_2626828182)

[Nom module	29](#__RefHeading___Toc79348_2626828182)

[Propriétés	29](#__RefHeading___Toc79350_2626828182)

[B.9 Module SecMvt	30](#__RefHeading___Toc79352_2626828182)

[Nom module	30](#__RefHeading___Toc79354_2626828182)

[Propriétés	30](#__RefHeading___Toc79356_2626828182)

[B.10 Module Urg	30](#__RefHeading___Toc79358_2626828182)

[Nom module	30](#__RefHeading___Toc79360_2626828182)

[Propriétés	30](#__RefHeading___Toc79362_2626828182)

[ANNEXE C — NOMENCLATURE ET CONVENTIONS	30](#__RefHeading___Toc79364_2626828182)

[C.1 Noms de modules	30](#__RefHeading___Toc79366_2626828182)

[C.2 Instances	30](#__RefHeading___Toc79368_2626828182)

[C.3 États	31](#__RefHeading___Toc79370_2626828182)

[ANNEXE D — GLOSSAIRE	31](#__RefHeading___Toc79372_2626828182)

[CONCLUSION TECHNIQUE	31](#__RefHeading___Toc79374_2626828182)


### **Périmètre technique du projet**

| Élément | Rôle |
| :-: | :-: |
| **Arduino Mega 2560** | Cœur bas niveau temps réel |
| **Arduino Uno** | Pilotage moteurs |
| **Smartphone SE** | Gateway terrain (Tasker, événements, capteurs OS) |
| **SP ** | Orchestration Node-RED – clones sur différents supports – 1 seul actif. SP SOURCE DE VERITE |
| **Raspberry Pi 3** | Serveur sur la box : accès distant fiable, broker MQTT alternatif avec celui de SP (qd SP et un smartphone en déplacement (sans box accessible))  
→ structurant mais non critique. Accessible avec adresse locale ou distante. |
| **MQTT** | Bus logique principal |

Le système est conçu pour fonctionner :

- en local (hotspot)

- sans cloud

- avec un broker unique

- un seul utilisateur, 1 seul SP actif par session.

## 1. Introduction générale

### **1.1 Objectif du document**

Ce document décrit **l’architecture complète du firmware embarqué du robot RZ**, en particulier :

- la structuration **modulaire** du code

- le protocole de communication **VPIV**

- les interactions entre **capteurs**, **actionneurs**, **sécurité** et **pilotages**

- les règles de conception garantissant :

  - la robustesse

  - la lisibilité

  - l’évolutivité

Ce document est un **document d’architecture**, pas un tutoriel Arduino, ni une simple documentation d’API.

Il constitue :

- une **référence projet**

- une base de **maintenance**

- un support de **revue technique**

- un point d’entrée pour toute évolution future


### **1.2 Philosophie générale**

Le firmware du robot RZ repose sur trois principes fondamentaux :

1. **Séparation stricte des responsabilités**

2. **Communication textuelle unifiée (VPIV)**

3. **Sécurité prioritaire et transversale**

Ces principes sont appliqués **sans exception** sur l’ensemble des modules.


### **1.3 Connexions**

**Fonctionnement en cas de coupure Internet**

- Le robot reste opérationnel en local

- Le SP peut être un smartphone partageant sa connexion

- Le broker local SP prend alors le relais

La perte d’Internet n’empêche **aucune fonction critique**.

## 2. Vue d’ensemble de l’architecture

### **2.1 Découpage en couches**

L’architecture suit une **règle stricte en trois couches**, valable pour tous les modules fonctionnels :

| **Couche** | **Rôle** |
| :-: | :-: |
| Hardware | Accès direct aux broches, timers, périphériques |
| Métier | Logique fonctionnelle, calculs, décisions |
| Communication | Interface VPIV, validation des commandes |

Aucune couche ne doit violer cette règle.


### **2.2 Arborescence logique du projet**

RzlibrairiesPersoNew.h : Point d’entrée unique de la librairie RZ, elle n’expose QUE les modules nécessaires au .ino elle contiens aussi la déclaration de RZ\_initAll() et est chargé dans le main.cpp

Déclare la fonction d’initialisation : RZ\_initAll() développé dans le .cpp



```
`src/`

` ├── actuators/        (MTR, SRV, LED, etc.)`

` ├── sensors/          (US, FS, IR, ...)`

` ├── safety/           (mvtSafe, liaison avec urg)`

` └── + system/         (urg)`

` ├── control/          (ctrl\_L)`

` ├── communication/    (VPIV, dispatchers)`

` ├── config/           (variables globales)`

` └── + navigation/     (Odom - Odométrie)  
 └── hardware/         (accès bas niveau)`

`+ à la racine RzlibrairiesPersoNew.cpp : qui gère la fonction RZ\_initAll()`
```

Chaque dossier correspond à **un rôle architectural clair**.



## 3. Protocole VPIV

### **3.1 Rôle du protocole VPIV**

VPIV (Variable Property Instance Value) est le **protocole unique** de communication entre :

- le firmware embarqué

- le système hôte (Node-RED, PC, SBC, etc.)

Il permet :

- la commande (l’intent ou la commande)

- la configuration

- le monitoring

- la remontée d’alertes


### **3.2 Format canonique d’une trame VPIV**

```
`$\<CAT\>:\<MODULE\>:\<PROP\>:\<INST\>:\<VALUE\>\#`
```

| **Élément** | **Signification** |
| :-: | :-: |
| CAT | La catégorie définit **la nature logique du message **Type (`V` commande, `I` information,..) |
| MODULE | **Nom du module** (ex: `Mtr`, `Srv`, `Ctrl\_L`) |
| PROP | **Propriété** |
| INST | **Instance ou `\*`** |
| VALUE | **Valeur associée** |


### **3.3 Catégories VPIV**

| Cat | Signification | Origine / Destination | Effet |
| :-: | :-: | :-: | :-: |
| V | Variable | Arduino ↔ SP | pas d’ACK attendu |
| I | Information | Arduino → SP | mise à jour état |
| F | Flux | Arduino → SP | information |
| E | Événement | Arduino → SP / SE → SP | événement prioritaire |
| A | Action applicative | SP ↔ SE uniquement | action applicative |

➡️ Le système reste **asynchrone**, toujours.  
👉 Arduino **ignore totalement** la catégorie A.

Le firmware **ne répond jamais implicitement**.  
Chaque action explicite produit une trame `$I:`. 

#### Clarification critique :

👉 ** CAT ne “temporise” rien**

👉 CAT **classe**, **priorise**, **filtre**  
👉 CAT **ne retarde jamais une action**

### **3.4 — VAR : module / variable**

- Correspond exactement au nom du module et au nom abrégé de la variable dans node-red

- Sert de clé de routage

- Unique dans l’espace VPIV

Exemples : `US, Mic, Mtr, CfgS, Ctrl\_L, ...`

👉 Toute incohérence entre nom de module et VAR est interdite.


### **3.5 - PROP : propriété**

La propriété décrit **l’attribut manipulé**.

#### Règles

- nom court

- explicite

- responsabilité unique

***Exemples*** : `enable,mode, thd, speed`

***Une propriété*** :

- est **interprétée uniquement par son module**

- ne doit jamais avoir de sémantique globale implicite


### 3.6 — INST : instance

***Permet d’adresser :***

- une instance

- plusieurs instances

- ou toutes les instances

***Valeurs autorisées :***

- `\*` : toutes

- `n` : instance unique

- `\[a,b\]` : plage

- `L`, `R` : symbolique

***Les règles d’interprétation sont strictes et non implicites***.


### 3.7 — VALUE : valeur

***Champ libre mais contraint***.

***Types autorisés ***:

- entier

- flottant (prévu, mais à éviter strictement / poids)

- texte

- JSON compact

***Contraintes :***

- taille maximale (`VPIV\_MAX\_MSG\_LEN`)

- aucun retour à la ligne

***Le parsing est assuré par `vpiv\_utils`.***


### **3.8 Contraintes de taille et de fréquence**

- Messages \< 100 caractères

- Faible fréquence côté SE

- Messages courts côté Arduino (temps réel)

Tasker : excellent support Regex, découpage topic, mais JSON possible mais volontairement limité

👉 Le protocole VPIV compact est **optimal** mais pourrait faire l’objet d’un traitement dans Termux de SE avant Tasker.

## 4. Règle des 3 couches d’un module (fondamentale)

### 4.1 Couche Hardware

- Accès direct aux broches

- Aucune logique métier

- Aucune trame VPIV

- Non bloquante

Exemples :

- `srv\_hardware.cpp`

- `mtr\_hardware.cpp`

- `fs\_hardware.cpp`


### **4.2 Couche Métier**

- Cœur fonctionnel

- Calculs

- États internes

- Appels hardware

- Émission **contrôlée** d’informations VPIV

Exemples :

- `mtr.cpp`

- `srv.cpp`

- `ctrl\_L.cpp`

- `mvt\_safe.cpp`


### **4.3 Couche Communication (Dispatch)**

- Décodage VPIV

- Validation syntaxique

- Routage vers le métier

- Aucun calcul fonctionnel

Exemples :

- `dispatch\_Mtr.cpp`

- `dispatch\_Srv.cpp`

- `dispatch\_Ctrl\_L.cpp`


🛑 **Règle absolue**  
Un dispatcher :

- **ne calcule rien**

- **ne touche jamais au hardware**

- **n’invente aucun état**



## 5. Module COMMUNICATION (VPIV Core)

### **5.1 Rôle**

Le module `communication.cpp` est :

- le **point d’entrée unique** des commandes

- le **point de sortie unique** des informations


### **5.2 Parser VPIV**

Fonctions clés :

- `parseVPIV()`

- `dispatchVPIV()`

- `sendInfo()`

- `sendError()`

Le parser :

- ignore les caractères parasites

- protège contre les débordements

- déclenche une **urgence** en cas d’overflow


### **5.3 Interaction avec l’urgence (URG)**

Lorsque `URG` est actif :

- seules les commandes autorisées passent

- toutes les autres sont refusées

- une erreur VPIV est renvoyée


## **6. Modules Capteurs**

Cette section décrit les modules capteurs **effectivement présents dans le firmware**, leur rôle, leur découpage en couches, et leur exposition via VPIV.

Capteurs couverts :

- US (Ultrasons)

- FS (Capteur de force – HX711)

- MvtIR (Détection de mouvement infrarouge)


## **6.1 Module US – Capteurs ultrasoniques**

### **6.1.1 Rôle fonctionnel**

Le module **US** fournit une mesure de distance en centimètres à partir de plusieurs capteurs ultrasoniques disposés autour du robot.

Ces mesures sont utilisées par :

- le pilotage (Ctrl\_L)

- la sécurité mouvement (SecMvt / mvtSafe)

- le monitoring (Node-RED)


### **6.1.2 Architecture interne**

| **Couche** | **Fichier** |
| :-: | :-: |
| Hardware | `us\_hardware.cpp` |
| Métier | `us.cpp` |
| Communication | `dispatch\_US.cpp` |


### **6.1.3 Couche Hardware**

Responsabilités :

- génération du trigger

- mesure du temps d’écho

- conversion temps → distance brute

Contraintes :

- non bloquant

- aucune logique d’agrégation

- aucune temporisation métier


### **6.1.4 Couche Métier**

Fonctions principales :

- filtrage simple

- stockage de la dernière valeur valide

- accès **peek** sans déclencher de mesure

Fonctions clés :

- `us\_getValue(idx)`

- `us\_peekCurrValue(idx)`

- `us\_processPeriodic()`

La notion de **peek** est centrale :  
elle permet à d’autres modules (SecMvt, Ctrl\_L) de lire la dernière valeur sans provoquer de perturbation temporelle.


### **6.1.5 Exposition VPIV – US**

Instances :

- index numérique **ou**

- nom symbolique (si défini dans `config.cpp`)

Propriétés supportées :

| **Propriété** | **Description** |
| :-: | :-: |
| `read` | Lecture immédiate |
| `act` | Activation / désactivation |
| `freq` | Période de mesure |
| `thd` | Seuil d’alerte |

Exemple :

```
`$V:US:read:3:\#`

`$I:US:value:3:42\#`
```


## **6.2 Module FS – Capteur de force**

### **6.2.1 Rôle fonctionnel**

Le module **FS** mesure :

- une force

- une orientation angulaire associée

Il est utilisé principalement par :

- `Ctrl\_L` (orientation utilisateur)

- le monitoring


### **6.2.2 Architecture interne**

| **Couche** | **Fichier** |
| :-: | :-: |
| Hardware | `fs\_hardware.cpp` |
| Métier | `fs.cpp` |
| Communication | `dispatch\_FS.cpp` |


### **6.2.3 Couche Hardware – HX711**

Caractéristiques :

- lecture 24 bits signés

- non bloquant

- gestion du gain (128)

Fonction clé :

- `fs\_hw\_readRaw(long &outValue)`


### **6.2.4 Couche Métier**

Responsabilités :

- tare

- calibration

- conversion brute → force exploitable

- mémorisation de la dernière valeur

Fonctions exposées :

- `fs\_doRead()`

- `fs\_doTare()`

- `fs\_doCal()`

- `fs\_lastForce()`

- `fs\_lastAngle()`


### **6.2.5 Exposition VPIV – FS**

Propriétés :

| **Propriété** | **Rôle** |
| :-: | :-: |
| `read` | Lecture force |
| `raw` | Lecture brute |
| `tare` | Mise à zéro |
| `cal` | Calibration |
| `act` | Activation |
| `freq` | Fréquence |
| `thd` | Seuil |

Exemple :

```
`$V:FS:\*:tare:1\#`

`$I:FS:tare:\*:OK\#`
```


## **6.3 Module MvtIR – Détecteur de mouvement infrarouge**

### **6.3.1 Rôle fonctionnel**

Le module **MvtIR** détecte :

- la présence

- le mouvement  
dans une zone donnée.

Utilisé pour :

- surveillance

- alertes

- comportement autonome


### **6.3.2 Architecture**

| **Couche** | **Fichier** |
| :-: | :-: |
| Hardware | `mvt\_ir\_hardware.cpp` |
| Métier | `mvt\_ir.cpp` |
| Communication | `dispatch\_IRmvt.cpp` |


### **6.3.3 Modes de fonctionnement**

| **Mode** | **Description** |
| :-: | :-: |
| `idle` | Désactivé |
| `monitor` | Détection simple |
| `surveillance` | Détection + seuil |


### **6.3.4 Exposition VPIV – MvtIR**

Propriétés :

| **Propriété** | **Description** |
| :-: | :-: |
| `read` | Lecture instantanée |
| `act` | Activation |
| `freq` | Période |
| `thd` | Seuil |
| `mode` | Mode |


## **7. Module CTRL\_L – Pilotage par laisse**

### **7.1 Rôle fonctionnel**

`Ctrl\_L` implémente le **pilotage par laisse** :

- distance utilisateur → vitesse

- orientation → rotation

Ce module est **un contrôleur**, pas un capteur.


### **7.2 Dépendances**

- FS (force / angle)

- US (distance)

- MTR (commande moteurs)

- URG (blocage en urgence)


### **7.3 Correction VPIV (validée)**

Nom du module :

```
`Ctrl\_L`
```

**et non** `ctrl` ou `L`.

Toutes les trames doivent respecter :

```
`$I:Ctrl\_L:\<prop\>:\*:\<value\>\#`
```


### **7.4 Paramètres dynamiques**

| **Paramètre** | **Description** |
| :-: | :-: |
| distance cible | mm |
| vitesse max | signed |
| rotation max | signed |
| dead zone | mm |
| fenêtre US | échantillons |


### **7.5 Fonctionnement**

1. Lecture US (distance)

2. Moyenne glissante

3. Calcul delta

4. Lecture FS (angle)

5. Conversion force → rotation

6. Envoi moteur


### **7.6 Exposition VPIV – Ctrl\_L**

Propriétés :

| **Propriété** | **Rôle** |
| :-: | :-: |
| `act` | Activation |
| `test` | Mode test |
| `status` | État |

## **8. Module SRV – Servomoteurs**

### **8.1 Rôle fonctionnel**

Le module **SRV** pilote les servomoteurs du robot.  
Il est responsable :

- de la position angulaire

- de la vitesse de déplacement

- de l’activation / désactivation individuelle

Ce module **n’implémente aucune logique de haut niveau** (trajectoire, sécurité).  
Il exécute strictement des consignes.


### **8.2 Architecture interne**

| **Couche** | **Fichier** |
| :-: | :-: |
| Métier | `src/actuators/srv.cpp` |
| Communication | `src/communication/dispatch\_Srv.cpp` |
| Hardware | `src/hardware/srv\_hardware.cpp` |


### **8.3 Instances et noms symboliques**

Les servomoteurs sont référencés par :

- un **nom symbolique**

- un **index interne**

Exemple (`config.cpp`) :

```
`const char \*srvNames\[\] = \{"TGD", "THB", "BASE"\};`
```

Cette convention est **obligatoire** :

- le VPIV manipule les noms

- le firmware convertit en index


### **8.4 Couche Métier – `srv.cpp`**

Responsabilités :

- validation des bornes (0–180°)

- stockage des consignes

- synchronisation avec le hardware

- publication VPIV d’état

Fonctions principales :

- `srv\_init()`

- `srv\_setAngle(idx, angle)`

- `srv\_setSpeed(idx, speed)`

- `srv\_setActive(idx, bool)`

- `srv\_process()`

📌 Le module ne connaît **ni les pins**, **ni le PWM**, **ni les timers**.


### **8.5 Couche Communication – VPIV**

Module VPIV :

```
`Srv`
```

Propriétés supportées :

| **Propriété** | **Description** |
| :-: | :-: |
| `angle` | Position (0–180) |
| `speed` | Vitesse |
| `act` | Activation |
| `read` | Lecture état |

Instance :

- nom symbolique

- `\*` pour tous

Exemples :

```
`$V:Srv:angle:TGD:90\#`

`$V:Srv:act:\*:0\#`

`$I:Srv:angle:BASE:45\#`
```


## **9. Module MTR – Moteurs**

### **9.1 Rôle fonctionnel**

Le module **MTR** pilote les moteurs de déplacement.

Il supporte **deux paradigmes** :

- legacy (acc, spd, dir)

- modern (v, ω, a)

Cette double compatibilité est **volontaire**.


### **9.2 Architecture**

| **Couche** | **Fichier** |
| :-: | :-: |
| Métier | `src/actuators/mtr.cpp` |
| Communication | `src/communication/dispatch\_Mtr.cpp` |
| Hardware | `src/hardware/mtr\_hardware.cpp` |


### **9.3 Mode Legacy**

Commandes :

- `acc` : accélération

- `spd` : vitesse

- `dir` : direction

Utilisé pour :

- compatibilité historique

- tests simples


### **9.4 Mode Modern**

Entrées :

- vitesse linéaire `v`

- vitesse angulaire `ω`

- niveau d’accélération `a`

Conversion :

```
`(v, ω) → (L, R)`
```

Fonction clé :

```
`mtr\_setTargetsSigned(v, omega, accel)`
```


### **9.5 Sécurité et scaling**

Fonctionnalités intégrées :

- arrêt forcé

- scaling dynamique (SecMvt)

- override stop


### **9.6 Exposition VPIV – MTR**

Module :

```
`Mtr`
```

Propriétés :

| **Propriété** | **Description** |
| :-: | :-: |
| `cmd` | v,ω,a |
| `mode` | modern / legacy |
| `scale` | scaling |
| `read` | état |
| `stop` | arrêt |


## **10. Module SecMvt / mvtSafe – Sécurité mouvement**

### **10.1 Clarification de nom**

- **SecMvt** : nom VPIV / dispatcher

- **mvtSafe** : implémentation interne

Il s’agit **du même module fonctionnel**.


### **10.2 Rôle fonctionnel**

Le module surveille les distances US et :

- émet des alertes

- réduit la vitesse

- déclenche une urgence


### **10.3 Modes**

| **Mode** | **Comportement** |
| :-: | :-: |
| `idle` | inactif |
| `soft` | réduction |
| `hard` | arrêt |


### **10.4 Exposition VPIV**

Module :

```
`SecMvt`
```

Propriétés :

- `act`

- `mode`

- `thd`

- `scale`

- `status`


## **11. Module URG – Urgences**

### **11.1 Rôle fonctionnel**

Le module **URG** gère :

- les états d’urgence globaux

- le verrouillage des commandes

- la signalisation (LED ring)

- la communication VPIV


### **11.2 Comportement clé**

Lorsqu’une urgence est active :

- seules les commandes `clear` sont acceptées

- tous les autres modules sont bloqués


### **11.3 Exposition VPIV**

Module :

```
`Urg`
```

Propriétés :

- `code`

- `clear`


`Repris pour clarification :  
`

| **Type** | **Fonction** | **Exemple** |
| :-: | :-: | :-: |
| `sendInfo` | État / info normale | `$I:CfgS:modeRZ:\*:2\#` |
| `sendError` | Erreur générique | `$E:COMM:parse:\*:Format invalide\#` |
| `urg\_sendAlert` | Urgence structurée | `$I:Urg:code:\*:3:MOTOR\_STALL\#` |

`👉 Mise en place d’une hyerarchie entre :`

- erreur bloquante

- erreur transitoire

- urgence sécurité



## **12. Module CfgS – Configuration système**

### **12.1 Rôle**

`CfgS` pilote les paramètres globaux :

- mode robot

- type de pilotage

- reset

- gestion urgence


### **12.2 Propriétés VPIV**

| **Propriété** | **Description** |
| :-: | :-: |
| `modeRZ` | mode global |
| `typePtge` | type pilotage |
| `emg` | clear |
| `reset` | reset |


## **13. Communication VPIV – Architecture**

### **13.1 Format canonique**

```
`$\<CAT\>:\<MODULE\>:\<PROP\>:\<INST\>:\<VALUE\>\#`
```


### **13.2 Catégories**

| **Cat** | **Sens** |
| :-: | :-: |
| V | Commande |
| I | Information |
| E | Erreur |


### **13.3 Règles fondamentales**

- pas de logique dans le parser

- pas de blocage

- dispatch unique

- sécurité prioritaire


# **ANNEXE A — SPÉCIFICATION VPIV COMPLÈTE**


## **A.1 Objectif de VPIV**

VPIV (Virtual Peripheral Interface for Vehicles) est le **protocole de communication canonique** entre :

- le firmware embarqué (Arduino Mega / RZ)

- les couches supérieures (Node-RED, UI, logique métier distante)

VPIV est :

- **lisible humainement**

- **déterministe**

- **stateless**

- **orienté module**

Aucune logique métier n’est implémentée dans VPIV.  
VPIV **transporte**, il ne **décide pas**.


## **A.2 Format canonique**

```
`$\<CAT\>:\<MODULE\>:\<PROP\>:\<INST\>:\<VALUE\>\#`
```

### **A.2.1 Détails des champs**

| **Champ** | **Description** |
| :-: | :-: |
| CAT | Catégorie (1 caractère) |
| MODULE | Nom du module (identique au nom de la  variable’abrégé coté node-red |
| PROP | Propriété |
| INST | Instance |
| VALUE | Valeur brute |


## **A.3 Catégories VPIV**

| **CAT** | **Nom** | **Sens** |
| :-: | :-: | :-: |
| V | Command | Commande descendante |
| I | Info | État / retour |
| F | Flux | Données continues |
| E | Error | Erreur |


## **A.4 Règles strictes**

1. **Pas d’espace**

2. **Pas de JSON (ZZ vérifize que la règle est stable)**

3. **Pas d’état implicite**

4. **Une trame = une action**

5. **Valeur brute non interprétée par le parser**


## **A.5 Parsing et robustesse**

Le parsing VPIV :

- ignore CR/LF

- protège les débordements

- tolère les listes `\[1,2,3\]`

- tolère `\*`

Un message malformé :

- ne bloque jamais

- déclenche `URG\_PARSING\_ERROR` si critique


## **ANNEXE B — TABLE VPIV PAR MODULE**


## Module URG – CFGS Articulation

# Point beaucoup plus important : **Urg vs CfgS (sécurité globale)**

Tu as parfaitement identifié **le vrai sujet architectural** 👌  
👉 **Urg n’est PAS une configuration**,  
👉 **CfgS est bien le bon endroit pour les commandes de sécurité globale**.

Et ton tableau le dit déjà implicitement.


## 2.1 — Clarif conceptuelle essentielle

### 🔴 `Urg`

- est **un état système asynchrone**

- déclenché par :

  - capteurs

  - watchdog

  - moteur

- **non négociable**

- bloque immédiatement

👉 Urg **NE REÇOIT PAS d’ordres**, sauf `clear`


### 🔵 `CfgS`

- est **le panneau de contrôle du système**

- reçoit des ordres humains / superviseur

- orchestre :

  - modes

  - reset

  - clear urgence

👉 **CfgS est le seul point d’entrée VPIV pour la sécurité**

➡️ **Ton intuition est strictement correcte**


## 2.2 — Analyse de ta table A (très bonne)

Prenons la ligne clé :

```
`CfgS (emg) V SP→A | SP→SE string clear`
```

Tu as écrit :

> *Commande ponctuelle de clear urgence (redirige vers urg\_clear)*

👉 C’est exactement ça  
👉 Et ton dispatch fait **exactement ce qu’il faut**

```
`if (strcmp(prop, "emg") == 0)`

`\{`

`    if (value && strcmp(value, "clear") == 0)`

`    \{`

`        urg\_clear();`

`        sendInfo("CfgS", "emg", "\*", "0");`

`        return true;`

`    \}`

`\}`
```

✔️ clair  
✔️ propre  
✔️ cohérent Table A  
✔️ pas de VPIV I inutile côté Urg


## 2.3 — Pourquoi `reset` doit rester dans `CfgS` (et pas Urg)

Tu as écrit :

> *Je ne vois qu'une logique à mettre le reset dans Cfgs*

👉 **C’est exactement la bonne conclusion**

### Raisons architecturales :

| **Action** | **Nature** | **Module** |
| :-: | :-: | :-: |
| Urgence | Réflexe sécurité | `Urg` |
| Clear urgence | Commande humaine | `CfgS` |
| Reset logiciel | Commande système | `CfgS` |
| Mode robot | Politique globale | `CfgS` |

👉 Un reset :

- n’est pas une urgence

- n’est pas déclenché par un capteur

- n’est pas automatique

- est une **commande volontaire**

➡️ **Donc CfgS, pas Urg**


## 2.4 — Interaction Urg ↔ modeRZ (point subtil mais important)

Ta table dit :

> *modeRZ conditionne règles de sécurité*

👉 Cela signifie :

- `Urg` **ne connaît pas** `modeRZ`

- `Urg` **s’enclenche quand même**

- MAIS :

  - certaines causes peuvent être **ignorées** ou **filtrées**

  - **AVANT** d’appeler `urg\_handle()`

Exemple (ailleurs dans le code) :

```
`if (cfg\_modeRz != MODE\_ARRET)`

`\{`

`    urg\_handle(URG\_MOTOR\_STALL);`

`\}`
```

👉 **Très bon découplage**

- Urg = mécanisme

- CfgS / Ctrl / Gtion = politique


# 3️⃣ Ton dispatch `dispatch\_CfgS.cpp` : verdict

### ✅ Ce qui est excellent

- clear urgence via `CfgS`

- pas de `urg\_sendAlert`

- pas de mélange état / commande

- ACK VPIV propre

### ⚠️ Micro-remarque (optionnelle)

Tu pourrais un jour distinguer :

```
`sendInfo("CfgS", "emg", "\*", "cleared");`
```

au lieu de `"0"`,  
mais **ce n’est pas obligatoire**, ton modèle actuel est cohérent.


# 4️⃣ Conclusion claire (à retenir)

✔ **Enum UrgReason : ne numérote pas, ce n’est pas nécessaire**  
✔ **Urg = état réflexe, jamais commandé**  
✔ **CfgS = panneau de contrôle sécurité**  
✔ **reset + clear urgence = CfgS**  
✔ **Ta table A est saine**  
✔ **Ton dispatch est cohérent**


## **B.1 Module CfgS**

### **Nom module**

```
`CfgS`
```

### **Propriétés**

| **Prop** | **Inst** | **Value** | **Sens** |
| :-: | :-: | :-: | :-: |
| modeRZ | \* | 0–N | Mode global |
| typePtge | \* | 0–4 | Type pilotage |
| emg | \* | clear | Clear urgence |
| reset | \* | 1 | Reset logiciel |


### **Exemples**

```
`$V:CfgS:modeRZ:\*:1\#`

`$V:CfgS:typePtge:\*:3\#`

`$V:CfgS:emg:\*:clear\#`
```


## **B.2 Module Ctrl\_L**

### **Nom module**

```
`Ctrl\_L`
```

⚠️ **Correction validée**  
Tous les `sendInfo("ctrl","L"...` sont **invalides**.


### **Propriétés**

| **Prop** | **Inst** | **Value** | **Sens** |
| :-: | :-: | :-: | :-: |
| act | \* | 0/1 | Activation |
| target | \* | mm | Distance cible |
| maxSpeed | \* | int | Vitesse max |
| maxTurn | \* | int | Rotation max |
| usWin | \* | 1–10 | Fenêtre US |
| dead | \* | mm | Zone morte |
| test | \* | 0/1 | Mode test |
| state | \* | texte | État |


### **Exemples**

```
`$V:Ctrl\_L:act:\*:1\#`

`$I:Ctrl\_L:state:\*:OK\#`
```


## **B.3 Module Mtr**

### **Nom module**

```
`Mtr`
```


### **Propriétés (modern)**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| cmd | \* | v,ω,a |
| mode | \* | modern/legacy |
| scale | \* | in,out |
| read | \* | — |


### **Propriétés (legacy)**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| acc | \* | 0–9 |
| spd | \* | 0–9 |
| dir | \* | 0–9 |
| triplet | \* | a,s,d |



## **B.4 Module Srv**

### **Nom module**

```
`Srv`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| angle | nom/\* | 0–180 |
| speed | nom/\* | 1–10 |
| act | nom/\* | 0/1 |
| read | nom/\* | — |



## **B.5 Module Odom**

### **Nom module**

```
`Odom`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| act | \* | 0/1 |
| read | \* | — |
| pose | \* | x,y,θ |
| speed | \* | v,ω |



## **B.6 Module US**

### **Nom module**

```
`US`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| read | nom/\* | — |
| act | \* | 0/1 |
| alert | nom | cm |



## **B.7 Module FS**

### **Nom module**

```
`FS`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| read | \* | force,angle |
| thd | \* | seuil |



## **B.8 Module Mic**

### **Nom module**

```
`Mic`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| rms | nom | int |
| thd | nom | int |
| act | \* | 0/1 |



## **B.9 Module SecMvt**

### **Nom module**

```
`SecMvt`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| act | \* | 0/1 |
| mode | \* | idle/soft/hard |
| thd | \* | warn,danger |
| scale | \* | 0.0–1.0 |
| status | \* | état |



## **B.10 Module Urg**

### **Nom module**

```
`Urg`
```


### **Propriétés**

| **Prop** | **Inst** | **Value** |
| :-: | :-: | :-: |
| code | \* | id:message |
| clear | \* | OK |


## **ANNEXE C — NOMENCLATURE ET CONVENTIONS**


### **C.1 Noms de modules**

- PascalCase

- Pas d’abréviation implicite

- Identiques dans code / VPIV / doc


### **C.2 Instances**

- Toujours symboliques

- Jamais d’index exposé

- `\*` = broadcast


### **C.3 États**

- `OK`

- `WARN`

- `DANGER`

- `ERROR`


## **ANNEXE D — GLOSSAIRE**

| **Terme** | **Définition** |
| :-: | :-: |
| VPIV | Protocole |
| Dispatcher | Routeur |
| Métier | Logique |
| Hardware | Bas niveau |
| Instance | Élément adressable |


## **CONCLUSION TECHNIQUE**

Ce firmware :

- est **modulaire**

- est **déterministe**

- est **sécurisé**

- est **extensible**

La séparation :

- Métier / Communication / Hardware  
est **strictement respectée**.

VPIV constitue :

- le **contrat unique**

- la **clé d’évolutivité**

- la **barrière de sécurité**

