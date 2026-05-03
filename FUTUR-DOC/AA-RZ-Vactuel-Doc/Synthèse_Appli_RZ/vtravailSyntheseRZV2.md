**RZ – V avril 2026**

# Contexte et Motivations

**Centraliser et automatiser** les interactions entre différents composants électroniques et logiciels dans le cadre d’un logiciel libre, pour réaliser un robot de compagnie à faible cout capable  d’assurer des échanges variés avec un utilisateur, de mobiliser des ressources android, de constituer une plateforme mobile d’observation et d’échange. Spécificité de la version :

- Une **communication robuste** via le protocole VPIV. 

- Une **modularité** pour faciliter les mises à jour et l’ajout de nouveaux modules. 

- Une **intégration fluide** avec des applications tierces (ex : Zoom, surveillance). 

### Objectifs du doc

Présentation synthétique du projet, des règles , renvoie vers les docs (les chemins changent mais la dernière version toujours dans AA-RZ-Vactuels).

**Table des matières**

[Contexte et Motivations	1](#__RefHeading___Toc29780_4052463322)

[Objectifs du doc	1](#__RefHeading___Toc62711_4052463322)

[Régles	2](#__RefHeading___Toc29782_4052463322)

[VPIV Encodage de l’information	3](#__RefHeading___Toc29784_4052463322)

[Role:	3](#__RefHeading___Toc29786_4052463322)

[Caractéristiques :	3](#__RefHeading___Toc29788_4052463322)

[Regle absolue :	3](#__RefHeading___Toc29790_4052463322)

[Format canonique d’une trame VPIV – règles associées	4](#__RefHeading___Toc29792_4052463322)

[CAT – Catégorie	4](#__RefHeading___Toc29794_4052463322)

[VAR – Variable	4](#__RefHeading___Toc29796_4052463322)

[PROP - propriété	4](#__RefHeading___Toc29798_4052463322)

[INST - instance	4](#__RefHeading___Toc28950_2626828182)

[VALUE - valeur	5](#__RefHeading___Toc28952_2626828182)

[Contraintes de taille et de fréquence	5](#__RefHeading___Toc29800_4052463322)

[Rappels / Les composants  :	5](#__RefHeading___Toc29802_4052463322)

[Périmètre technique du projet	5](#__RefHeading___Toc109278_2626828182)

[SE : système embarqué:cerveau du robot	7](#__RefHeading___Toc29804_4052463322)

[Doc :	7](#__RefHeading___Toc29806_4052463322)

[Matériel & logiciels:	7](#__RefHeading___Toc29808_4052463322)

[Rôle :	7](#__RefHeading___Toc29810_4052463322)

[Position de SE par rapport à Arduino	7](#__RefHeading___Toc38410_492035557)

[Permet un mode de pilotage vocal du robot	7](#__RefHeading___Toc38412_492035557)

[Gestion des données - 2 voies : Termux et Tasker	8](#__RefHeading___Toc38414_492035557)

[Installation de l’application termux sur SE	9](#__RefHeading___Toc29812_4052463322)

[Démarrage du robot	9](#__RefHeading___Toc29814_4052463322)

[Explication détaillée des étapes :	10](#__RefHeading___Toc24212_3036915566)

[Points Clés	10](#__RefHeading___Toc29816_4052463322)

[A : Arduino mega (un arduino Uno esclave du mega est dédié aux moteurs est inclus)	11](#__RefHeading___Toc29818_4052463322)

[Doc :	11](#__RefHeading___Toc29820_4052463322)

[Rôle :	11](#__RefHeading___Toc29822_4052463322)

[Arborescence logique de la partie Arduino	11](#__RefHeading___Toc29824_4052463322)

[Specificités Arduino :	11](#__RefHeading___Toc29826_4052463322)

[SP – système pilote	12](#__RefHeading___Toc29828_4052463322)

[RPI	13](#__RefHeading___Toc29830_4052463322)

[Matériel	13](#__RefHeading___Toc29832_4052463322)

[Rôle	13](#__RefHeading___Toc29834_4052463322)

[Configuration communication :	13](#__RefHeading___Toc29836_4052463322)

[Identifiants :	13](#__RefHeading___Toc29838_4052463322)

[Les tables sources de l’appli RZ	14](#__RefHeading___Toc29840_4052463322)

[Table A	14](#__RefHeading___Toc62713_4052463322)

[Connexions	14](#__RefHeading___Toc29842_4052463322)

[Structure du projet	14](#__RefHeading___Toc29844_4052463322)

[Structure des scripts & modules	14](#__RefHeading___Toc29846_4052463322)

[Structure du script SP - Node-red	14](#__RefHeading___Toc29848_4052463322)

[Structure du script SE – partie Termux	14](#__RefHeading___Toc29850_4052463322)

[Structure du script de l’arduino	14](#__RefHeading___Toc29852_4052463322)

[Schéma fonctionnel général	14](#__RefHeading___Toc29854_4052463322)

[Cycle loop() (MEGA)	15](#__RefHeading___Toc29856_4052463322)

[Architecture des configurations (cfg\_…)	15](#__RefHeading___Toc29858_4052463322)

[Sécurité centrale (urgence)	15](#__RefHeading___Toc11389_2415352087)

[Diagramme logique concernant les urgences	16](#__RefHeading___Toc28045_1733942984)

[Structure du code (arborescence)	16](#__RefHeading___Toc29860_4052463322)

[Communication MEGA → UNO	17](#__RefHeading___Toc29862_4052463322)

[Structure du script de tasker	17](#__RefHeading___Toc29864_4052463322)

[Annexes	17](#__RefHeading___Toc29866_4052463322)

[Historique des versions	17](#__RefHeading___Toc29868_4052463322)

[Schémas	17](#__RefHeading___Toc29870_4052463322)

[Présentation des modules	17](#__RefHeading___Toc29872_4052463322)

[Philosophie générale	17](#__RefHeading___Toc29874_4052463322)

[Découpage en couches	18](#__RefHeading___Toc29876_4052463322)

[Table A	19](#__RefHeading___Toc62715_4052463322)


# Régles

- Règles de conceptions :

  - système d’échanges protégés avec priorité aux urgences

  - stratégie d’économie de moyen pour des appareils anciens ou limités

  - facilité la maintenance et l’évolutivité par :

    - des structures de scripts modulaires

    - des tableaux de données

    - des structures et des scripts commentées

- une seule source de vérité celle de SP

- un seul système d’encodage des données pour le transport : VPIV

- un seul modèle de définition des variables, basé sur les données nécessaires à VPIV

- une seule source d’intialisation : le fichier contenant enrichedVars sur le SP courant

- un seul « module » par actionneur ou par capteur porté par SE ou A . 

- A chaque « module » correspond une variable qui en reprend les propriétés, défini les instances (les propriétés sont communes aux instances d’une variable)

- Un utilisateur unique, qui contrôle le robot par SP, un seul SP actif au sens de la source des pages générés par node-red.

- Deux bases de données node-red, une seule en contrôle, en fonction du contexte : 

  - si ~~***WiFi** maison → broker local RPI , également SP via adresse localement

  - si ~~***internet** → DuckDNS broker RPI , également SP si pas de Smartphone SP configurée

  - si ~~***hotspot** SP configuré (pas de box accessible)→ broker SP local & node-red localement

  - si mode autonome celle du serveur RPI quand SE est sur le réseau personnel, ou si hors connexion, en mode autonome réaction automatique de SE qui deviens un SP dégradé (pas de base Node-red, mais prends des décisisons donc SP aux fonctions restreintes). 

- Une variable clef pour le comportement du robot CfgS :

  - ~~***modeRZ** : ~~***0=ARRET|1=VEILLE|2=FIXE|3=DEPLACEMENT|4=AUTONOME|5=ERREUR**

  - typePtge (pilotage) : ~~***0=ECRAN|1=VOCALE|2=JOYSTICK|3=LAISSE|4=LAISSE+VOCALE**

  - ~~***+ événements associés :**

    - ~~*** emg → efface urgence active**

    - ~~*** reset → réinitialisation complète. Refusé si urgence active.**

- ~~***Règles d’urgences : voir le doc urgence and co.odt**

# VPIV Encodage de l’information

Système d’encodage commun à toute l’application, permettant le transport des données sous une forme condensée par MQTT.

### Role:

VPIV (Variable Property Instance Value) est le **protocole unique** de communication entre :

- le firmware embarqué

- le système hôte (Node-RED, PC, SBC, etc.)

Il permet :

- la commande (l’intent ou la commande)

- la configuration

- le monitoring

- la remontée d’alertes

### Caractéristiques :

- être transmis sans transformation via MQTT / UART

- être lisible humainement

- être facilement parsable en C sur Arduino

- gérer modules / propriétés / instances

### Regle absolue :

- **pas de float sur le fil VPIV.**

- **communication textuelle**

- **système asynchrone**

## Format canonique d’une trame VPIV – règles associées 


$CAT:VAR:PROP:INSTANCE:VALUE\#


| **Champ** | **Description** |
| - | - |
| **CAT** | **V = Consigne  |  I = Info / ACK  |  E = Événement prioritaire |  A = Application (action applicative)  |  F = flux –   
CAT : utile pour : classer, prioriser, filtrer → ne temporise pas en soit.** |
| **VAR (module)** | **Module concerné  (ex : CamSE, Gyro, Sys, Appli…) - Clé de routage unique** |
| **PROP** | **Propriété dans le module  (ex : modeCam, dataContGyro, alerte…)** |
| **INSTANCE** | **Sous-élément  (\* = tous, rear/front pour cam, TGD/THB pour servo…)** |
| **VALUE** | **Valeur. Objet JSON entre \{\}. String avec guillemets \\"…\\" dans les événements.** |
| **$ et \#** | **Délimiteurs obligatoires début et fin** |

Exemples :  
\`\`\`  
$V:Mtr:\*:cmd:50,0,2\#          \<- SP envoie commande moteur  
$I:CfgS:\*:modeRZ:1\#           \<- Arduino confirme modeRZ  
$E:Urg:A:source:loop\_slow\#    \<- Arduino signale urgence  
$F:Odom:pos:\*:1234,567,1570\#  \<- Arduino publie position

### **CAT – Catégorie**

Rôle : CAT  permet de : classer, prioriser, filtrer → ne temporise pas en soit. zz

| CAT | Signification | Priorité | Comportement Système | Usage Type |
| :-: | :-: | :-: | :-: | :-: |
| **V** | **Variable de commande** | 🟠 Haute | **Synchrone :** Fixe une intention. Attend souvent un ACK. | Pilotage, consigne moteur, config. |
| **I** | **Information / État** | 🟢 Moyenne | **Fiable :** Retour terrain suite à une commande ou changement d'état. | Acquittement (ACK), statut "Prêt". |
| **F** | **Flux (Streaming)** | ⚡ Temps Réel | **Fast Lane :** Pas d'ordre garanti, pas d'ACK, pipeline optimisée. | Odométrie, scans haute fréquence. |
| **E** | **Erreur / Événement** | 🔴 Immédiate | **Alerte :** Déclenche une logique de sécurité ou un log prioritaire. | Timeout, valeur invalide, arrêt d'urgence. |
| **A** | **Application** | ⚪ Faible | **Logique :** Non traité par l'Arduino. Uniquement pour le SE/UI. | Debug, logs applicatifs, alertes UI. |


### **VAR – Variable**

- Clé de routage unique

- Lié à un module (ex : CamSE, Gyro, Sys, Appli…)

- 4 variables à incidences multiples : CfgS, Urg, COM, Sys

### **PROP** - propriété

La propriété décrit **l’attribut manipulé  - règles :**

- nom court, explicite,responsabilité unique

- est **interprétée uniquement par son module**

- ne doit jamais avoir de sémantique globale implicite

### INST - instance

***Permet d’adresser* : ***une instance, plusieurs, toutes les instances

**Valeurs autorisées* :***

- `\*` : toutes

- `n` : instance unique

- `\[a,b\]` : plage

- `L`, `R` : symbolique

***Les règles d’interprétation sont strictes et non implicites**.

### VALUE - valeur

***Champ libre mais contraint.**

***Types autorisés :**

- entier

- flottant (prévu, mais à éviter strictement / poids)

- texte

- JSON compact

***Contraintes :**

- taille maximale (`VPIV\_MAX\_MSG\_LEN dans l’arduino)`

- aucun retour à la ligne

***Le parsing est assuré par `vpiv\_utils dans arduino`***

### Contraintes de taille et de fréquence

- Messages \< 100 caractères

- Faible fréquence côté SE

- Messages courts côté Arduino (temps réel)

Tasker : excellent support Regex, découpage topic, mais JSON possible mais volontairement limité

👉 Le protocole VPIV compact est **optimal** mais pourrait faire l’objet d’un traitement dans Termux de SE avant Tasker.


# Rappels / Les composants  :

ℹ  Node-RED (SP) est le maître absolu. SE exécute les ordres de SP, collecte et lui remonte les données, et signale les urgences. SE ne prend aucune décision autonome de haut niveau.

### Périmètre technique du projet

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

- avec un broker unique (qui peut selon le contexte être sur le smartphone SP configuré ou sur RPI.

- un seul utilisateur, 1 seul SP actif par session.


| **Sens** | **Contenu** |
| - | - |
| **SP → SE** | **Consignes de mode ($V:), commandes application ($A:), demandes de config ($V:...paraMicro…)** |
| **SE → SP** | **ACK de chaque consigne ($I:), flux de données capteurs ($I:), événements et urgences ($E:), trames d'init au boot ($F:)** |
| **SE → Arduino** | **Relais des $V: Arduino via MQTT bridge + USB-C** |
| **Arduino → SP** | **SE relaie les $I: Arduino vers SP** |



## SE : système embarqué:cerveau du robot

### Doc :

Doc\_SE\_ref\_node-red → prépa info node-red : présente chaque module et 	articulations avec modeRZ, typePilotage, Sys, Urg. → les trois modules généraux.

Voir aussi C:\\Users\\Vincent.VINCENTPHACER\\Documents\\RobotRZ-horsIO\\DOC\\04\_SE-TASKER

### Matériel & logiciels:

**Smartphone sans carte SIM –* Exemple le Doro préconfig***

***- Le développement du code se fait sur le PC via visual code studio actualisation GIT, reversement sur le smatphone (attention aux FIFO) – IP fixe : 192.168.1.12***

- ***SSHD → pour pouvoir le manipuler depuis PC***

- ***Termux → pour le développement de l’appli – extensions : sh, json, py***

- ***Tasker → manipulation des applis android***

### Rôle :

- trouver ou retrouver SP (test adresse IP ou saisie)

- s’adapter au contexte défini par SP et sa variable Cfgs (configuration système)  

- initialiser ses variables via infos SP

- gérer un bridge USB pour un arduino Mega → trafic de VPIV + alimentation de la batterie du smartpone.

- publier et recevoir les messages MQTT sur deux « lignes » séparé selon que la communication concerne A ↔ SP ou SE ↔ SP 

- fournir les données de ses capteurs et actionneurs

- acceder aux applis android via Tasker

- permettre de transmettre les commandes vocales en fonction de type de pilotage


### Position de SE par rapport à Arduino

| Arduino | SE |
| :-: | :-: |
| Temps réel | Applicatif |
| Sécurité physique | Sécurité logique |
| Action directe moteurs | Aucune |
| Capteurs bas niveau | Capteurs riches |
| Blocage urgence | Signalement urgence |

👉 **SE ne court-circuite jamais Arduino**

### Permet un mode de pilotage vocal du robot 

Avec les applis de STT (speack to text ) SP permet le pilotage vocal, mais reste contrôlé :

- STT = **capteur intelligent**

- SP = **cerveau**

- SE = **exécutant contrôlé**

- La voix **n’a jamais le dernier mot**

> ✅ **SE ne change jamais directement un mode**  
✅ **SE soumet une intention explicite**  
✅ **Gtion\_EtatGlobal (flux node-red de SP central) reste l’unique arbitre**

### Gestion des données - 2 voies : Termux et Tasker

L’appli historique de gestion des données des premières version était Tasker, appli android qui permet de créer des scènes sur l’écran de SE, de gérer des événements, de manipuler des fonctions du smartphone, d’accéder à ses capteurs et actionneurs et manipuler les applications android : lancement, fermeture,… Termux permet également plusieurs de ces tâches , mais chacun peut-être plus efficace pour certaine. Il s’agira de répartir les tâches .


#### Ce qui va dans Tasker

- **Scènes UI** : 

  - Expressions faciales (yeux, bouche). Interaction utilisateur directe

  - Boutons de contrôle (ex: mode urgence). 

- Gestion des applications Apps (Zoom, Cam, ...)

- STT / TTS passent essentiellement dans termux

👉 **VPIV de type A (Action)** majoritairement


#### **Ce qui va dans Termux : communication, capteurs, logique métier**

- Décryptage et gestion des échanges VPIV avec SP (via MQTT)

- Capteurs (Gyro, Mag, Light, Mic) 

- Système (Batt, Sys)

- Boucles périodiques

- Gestion des modes (`paraMicroSE`, `paraGyro`). 

- Calculs (moyennes, seuils). 

- Fréquences ajustables

#### **Architecture Simplifiée pour SE**

**👉 Chaque service est découplé - 👉 Le VPIV est centralisé - 👉 Aucune dépendance directe Arduino **

##### **Rôles des Composants sur SE**

| Composant | Rôle | Avantages |
| :-: | :-: | :-: |
| **Termux** | - Parsing des messages VPIV.  
- Gestion des scripts Bash/Python (STT, TTS, MQTT).  
- Traitement des capteurs/actionneurs. - Bridge Arduino (USB C) | - **Léger** (pas de Node-RED).  
  
- **Modulaire** (scripts indépendants).  
  
- **Accès direct aux capteurs** via Termux API. |
| **Tasker** | - Lancement d’applications (Zoom, caméra, etc.).  
- Gestion des événements Android (capteurs natifs, notifications).  
- Intégration avec Termux via `run shell`. | - **Optimisé pour Android**.  
  
- **Gestion des permissions natives**.  
  
- **Automatisation des tâches** (ex: allumer la torche). |
| **MQTT (mosquitto)** | - Communication avec SP et Arduino via le broker MQTT (RPI). | - **Léger et fiable**.  
  
- **Intégration simple** avec Termux/Tasker. |



### **Installation de l’application termux sur SE**

#### **Fichier original**` (original\_init.sh) :`

- Est exécuté une seule fois lors de l'installation initiale 

- Génère la structure de base 

- Ne change jamais après sa création 

#### **Fichier courant (`courant\_init.sh`) :**

- Contient les valeurs modifiables 

- Est créé une seule fois lors de l'installation 

- Peut être modifié pendant l'exécution 

- Est utilisé à chaque démarrage pour charger la configuration 

#### **Fichiers JSON :**

- Sont générés à partir du fichier courant à chaque démarrage 

- Contiennent les valeurs spécifiques pour chaque capteur 

- **Sont utilisés par les scripts pendant l'exécution**

### **Démarrage du robot**

***zz à compléter***

#### **Phase d’initialisation au démarrage du robot**

- **Courant\_init.sh → charger les configurations**

- **`check\_config.sh` vérifie la cohérence **

- **Chargement des valeurs valides dans les fichiers JSON **

#### **Démarrage des services**

- **`rz\_state-manager.sh` gère les états globaux **

- **`rz\_vpiv\_parser.sh` traite les messages VPIV **

- **`rz\_gyro\_manager.sh` gère ‘(exple le gyroscope)**

- **`rz\_mqtt\_bridge.py` fait le pont MQTT **


### Explication détaillée des étapes :

#### **Envoi initiale (SP → MQTT) :**

  - SP envoie une commande VPIV via MQTT 

  - Format : `$V:Gyro:mode:\*:BRUT\#` 

  - Composants : Node-RED → Broker MQTT → Termux 

#### **Traitement par Termux (SE) :**

  - `rz\_vpiv\_parser.sh` : Décode le message 

  - Vérification des conditions (modeRZ, etc.) 

  - Si valide : Activation du script sensoriel spécifique 

  - Si invalide : Envoi d'une erreur (CAT E) 

#### **Interaction avec le hardware :**

  - Le script sensoriel active le hardware via Termux API 

  - Lecture des données brutes 

  - Application des traitements (calibration, filtres, etc.) 

  - Retour des données traitées 

#### **Retour des données (SE → MQTT) :**

  - Envoi des données traitées (CAT F) 

  - Format : `$F:Gyro:data:\*:\[x,y,z\]\#` 

  - Fréquence : Déterminée par la configuration 

#### **Traitement par SP :**

  - Mise à jour des variables globales 

  - Vérification des seuils d'alerte 

  - **Envoi d'une confirmation (CAT I) **

### Points Clés

#### **Séparation des responsabilités :**

  - `rz\_state-manager.sh` gère uniquement les états globaux 

  - Les gestionnaires spécifiques (ex: `rz\_gyro\_manager.sh`) gèrent le hardware 

#### **Validation à chaque étape :**

  - Vérification du modeRZ avant toute action 

  - Vérification des configurations locales 

#### **Communication non-bloquante :**

  - Utilisation des FIFO pour la communication inter-processus 

  - Publication asynchrone des données MQTT 

#### **Gestion des erreurs :**

  - Logs détaillés à chaque étape 

  - Retour aux valeurs par défaut en cas d'erreur 

## A : Arduino mega (un arduino Uno esclave du mega est dédié aux moteurs est inclus)

### Doc :

- synthese\_arduino\_nodered.md → Document de reference Arduino a utiliser pour demarrer le developpement Node-RED (SP). Source normative : Table\_A\_Arduino v3.1

- synthèse-Arduino GenereParChatGPT.odt

### Rôle :

Gestion bas niveau des capteurs et actionneurs non embarqués sur le smartphone SE,  
permettre de transmettre les commandes de la laisse électronique en fonction du type de pilotage.

### Arborescence logique de la partie Arduino

**RzlibrairiesPersoNew.h** :

- Point d’entrée unique de la librairie RZ

- elle n’expose QUE les modules nécessaires au .ino

- elle est chargé dans le main.cpp

- déclare la fonction d’initialisation : RZ\_initAll() développé dans le .cpp


**Enchaînement des inclusions**

```
\`\`\`

RZlibrairiesPerso.h

 └── config.h         → constantes globales

 └── communication.h  → protocole VPIV

 └── autres modules   → leds, servos, moteurs, capteurs, etc.


**Arborescence (extrait)**

```
`src/`

` ├── actuators/        (MTR, SRV, LED, etc.)`

` ├── sensors/          (US, FS, IR, ...)`

` ├── safety/           (mvtSafe, liaison avec urg)`

` └── + system/         (urg)`

` ├── control/          (ctrl\_L, laisse électronique)`

` ├── communication/    (VPIV, dispatchers)`

` ├── config/           (« constantes » variables globales)`

` └── + navigation/     (Odom - Odométrie)  
 └── hardware/         (accès bas niveau)`

`+ à la racine RzlibrairiesPersoNew.cpp : qui gère la fonction RZ\_initAll()`
```

Chaque dossier correspond à **un rôle architectural clair**.

### Specificités Arduino :

- pas de catégorie A (applicatives dans les trames vpiv de l’arduino

- correspondance exacte entre le nom du module et le nom de la variable dans node-red

- Quand Urgence (var Urg) propriété etat → à la valeur active, toutes les commandes vpiv sont ignoré hors : 

  - Urg – etat – cleared (levé urgence)

  - CfgS



## SP – système pilote

Est défini comme celui qui porte le node-red actif. Le serveur RPI ou le smartphone configuré.

Le pilotage physique par l’utilisateur est possible depuis n’importe quel appareil si il existe du réseau. Pour l’économie de moyen et hors espace wifii on utilise le smartphone configuré - \> via réseau local ou partage de connexion. Le doublonnage bluetooth n’a finalement pas été retenu.

La configuration dans le cas du RPI a été spécifié dans le chapitre correspondant. Pour le smartphone configuré voir le doc configuration pour le détail : Appli installée obligatoire : Termux, SSH (pour la config), node-red (copie de la version actuelle disponible sur le rpi)

Doc : voir installation.odt

## RPI

### Matériel

Raspberry Pi3 B+

### Rôle

- Fonction de conservation de la version node-red source qui peut être installée sur   
les smartphones configuré pour être des SP hors réseau. 

- Accès Internet : serveur web

  - lorsqu’à la portée Wifi de la box de l’administrateur avec l’adresse locale 

  - par internet, via le serveur web Nginx (alternative à apache) avec adresse sur domaine duckdns.org → robotz-vincent.duckdns.org 

  - Gestion secondaire d’un site perso à l’adresse : vincent-ph.duckdns.org

- Broker mqtt quand support SP (sauf dans le cas SP smartphone hors réseaux wifi)

### Configuration communication :

*Dans le cas d’une installation d’un clone du robot penser à configurer la box de l’administrateur !*

- Web server (Node-red) – port 80 (interne-externe) – protocole TCP

- serveurMQTTRPI3 – port 1883 (interne-externe) – protocole TCP/UDP - config  HTTPS - IPV6

- sitePerso – port 443 (interne\_externe) – protocole TCP – config : HTTPS et IPV4


### Identifiants :

- SSH → adresse local du RPI : 192.168.1.40 – Id : rzServeur – Mot de passe : n-habituel- ?

- Nginx → pour protection NodeRed Nginx → Id : RobotV4rz – Mot de passe : nodeRed-habituel- ?

- Duckdns → id:adresse mail vincentphcom – token 99bab…..ad  - mot de passe : connexion via google.

- MQTT broker : id (user) : rpiBrokerUser – mot de passe : RZ-habituel- ?  
Si l’on veux augmenter la sécurité on peut ajouter un chiffrement TLS/SSL des communications MQTT.


# Les tables sources de l’appli RZ

Les tables sont développés sur des tableurs et convertis en csv puis en json pour le développement et l’installation et l’initialisation de l’application RZ.  
Infos développés  cf Description et fichiers dans  AA-RZ-Vactuel-Doc/Tables

## Table A – Table mère  pour le développement des modules

La Table A est la référence unique et partagée de toutes les variables manipulées par le système RZ, qu'elles soient échangées entre SP, SE et Arduino, ou internes à un sous-système. Elle sert à :

• La construction correcte des trames VPIV — chaque message peut être vérifié contre la table

• La validation des intents STT — le handler vocal sait quelles valeurs sont légales

• La cohérence entre les couches — interface SP, logique SE, firmware Arduino partagent la même définition

• La maintenance long terme — un développeur retrouve en un seul endroit toutes les règles

**La Table A est normative** : si un comportement du code diverge de la table, c'est le code qui est à corriger, pas la table (sauf découverte d'une erreur dans la table elle-même).

#### Liste synthétique des champs

Le tableau ci-dessous donne la vue d'ensemble des 15 champs. La description détaillée de chacun est dans la section 3. du doc d’origine

| **\#** | **Nom retenu** | **Anciens noms / alias** | **Rôle en une ligne** |
| - | - | - | - |
| 1 | SOURCE | Ref | Qui est responsable d'émettre ce message |
| 2 | VAR | — | Identifiant de la variable dans le protocole VPIV |
| 3 | PROP | propriété | Propriété de la variable ciblée |
| 4 | CAT | catégorie, type\_message | Catégorie protocolaire du message VPIV |
| 5 | TYPE | type\_valeur, type\_logique | Famille logique de la valeur transportée |
| 6 | FORMAT\_VALUE | format, valeurs\_acceptées, Abrev-indices | Valeurs légales et format de pattern |
| 7 | CODAGE | (nouveau v2.0) | Unité de mesure et facteur multiplicateur |
| 8 | DIRECTION | sens, SENS, flux | Sens de circulation du message VPIV |
| 9 | INSTANCE | — | Instances valides pour cette propriété |
| 10 | COMPLEXE | — | Documentation des sous-champs pour TYPE=object |
| 11 | CYCLE | INITIAL, init, initialisation | Moment et mode de mise en place de la valeur |
| 12 | NATURE | STATUT, statut\_propriété | Rôle sémantique du message dans le système |
| 13 | INTERFACE | (extrait de STATUT=EXPOSEE) | Propriété visible et pilotable depuis SP |
| 14 | VPIV\_EX | VPIV-ACTIONS, exemple\_vpiv, exemple | Trame VPIV complète illustrative |
| 15 | NOTES | COMMENTAIRES, remarques, ... | Rôle fonctionnel et précautions libres |

## Table C – Table fille de A – fonctionnelle

Source de l’application stocké dans fichier présent dans SP (mémoire des var Globales)  utilisé à l’initialisation de l’application .

## Table D – Catalogue des commandes STT

Permet de lister les commandes STT dans le cas d’un pilotage vocale : depuis la définition des expressions clefs → la création d’un vpiv de la commande. Le même tableau précise les vpiv à réaliser pour appliquer la commande. Cette table doit être chargé depuis SP au départ elle est utilisé côté SP et coté SE. 

# Connexions

**Fonctionnement en cas de coupure Internet**

- Le robot reste opérationnel en local

- Le SP peut être un smartphone partageant sa connexion

- Le broker local SP prend alors le relais

La perte d’Internet n’empêche **aucune fonction critique**.

# Structure du projet

# Structure des scripts & modules

Doc : futur-doc/F-dev-en-cours/ …. → un doc par module pour SE et A

## Structure du script SP -  Node-red

## Structure du script SE – partie Termux 

Cf synthèse générale SE .odt (se reporter au github pour la dernière 

## Structure du script de l’arduino

Doc source (avant la dernière actualisation de l’arduino  → NewRZ-ArduinoMega-Unov6.odt  
description complète, même si quelques évolutions permet de comprendre l’organisation du code, inclus conseil intégration Node-red et doc module (attention source de vérité var prop table actuelle).

### Schéma fonctionnel général

```
`                 ┌───────────────────────────┐`

`                 │   Node-RED Dashboard      │`

`                 │  (Joystick, Vidéo, UI)    │`

`                 └───────────────┬───────────┘`

`                                 MQTT`

`                                 │`

`                     Arduino IOT UART Gateway`

`                                 │ USB`

`                                 ▼`

`                 ┌─────────────────────────────┐`

`                 │        Arduino MEGA         │`

`                 │─────────────────────────────│`

`                 │  Communication VPIV         │`

`                 │  Config / Emergency         │`

`                 │  Odométrie                  │`

`                 │  Ultrason / IR / Micro      │`

`                 │  Force sensor (laisse)      │`

`                 │  LEDs / Servos              │`

`                 │  MvtSafe                    │`

`                 │  Moteurs (mtr.cpp)          │`

`                 └───────┬─────────────────────┘`

`                         Serial3 (115200)`

`                         ▼`

`                 ┌────────────────────────────┐`

`                 │         Arduino UNO        │`

`                 │────────────────────────────│`

`                 │ Parsing trames \<L,R,A\>     │`

`                 │ Lissage A=0..4             │`

`                 │ Timeout sécurité           │`

`                 │ Dual VNH5019               │`

`                 └────────────────────────────┘`
```

### Cycle loop() (MEGA)

La boucle principale ne fait **que du non-bloquant**.

`loop():`

`    communication\_processInput()`

`    lring\_processPeriodic()`

`    mic\_processPeriodic()`

`    us\_processPeriodic()`

`    fs\_processPeriodic()`

`    ir\_processPeriodic()`

`    odom\_processPeriodic()`

`    mvtsafe\_process()`

`    lrub\_processTimeout()`

`    srv\_process()`

`    checkEmergencyConditions()`

Objectif : **loop \< 80 ms**

### Architecture des configurations (cfg\_…)

Tous les modules exposent leurs paramètres dans `config.h` + `config.cpp`.

- Chaque paramètre a **une valeur par défaut**.

- Node-RED peut envoyer un `$V:xxx:yyy:\*:value\#` qui écrase la valeur.

- MEGA renvoie confirmation via `$I:xxx:yyy:\*:value\#`.

- Ces valeurs sont persistantes tant que le Mega reste sous tension.

Exemples :

```
`$V:Odom:geom:\*:0.100,0.300,256\#`

`$V:us:cycle:\*:150\#`

`$V:mvtSafe:mode:\*:hard\#`
```

### Sécurité centrale (urgence)

Le module `urg.cpp` gère les cas critiques :

- dépassement de courant

- capteur non répondant

- buffer serial saturé

- anti-collision hard MvtSafe

- parsing invalide

Quand `urg\_active=true` :

👉 **Toutes les commandes VPIV sont ignorées**, sauf :

```
`scfg:clear`

`urg:clear`
```

MEGA renvoie :

```
`$E:URG:state:\*:\<code\>\#  
`
```

### `Diagramme logique concernant les urgences`

`\[Capteurs US\]   
 │  
 ▼  
\[mvtsafe\_process()\]  
 │  
 ├─\> SOFT : \_apply\_soft\_reduction() → réduction moteur  
 │  
 └─\> HARD : \_apply\_hard\_stop()  
 ├─\> mtr\_stopAll()           → arrêt moteurs  
 ├─\> urg\_handle(URG\_MOTOR\_STALL)  
                 │      ├─\> urg\_stopAllMotors()   → sécurité redondante  
                 │      ├─\> lring\_emergencyTrigger() → LED  
                 │      └─\> urg\_sendAlert()       → VPIV $I:Urg  
                 └─\> \_send\_vpiv\_state("alert","danger") → VPIV mvtSafe`

### Structure du code (arborescence) 

```
`/src`

`  /communication`

`      communication.cpp/h`

`      vpiv\_dispatch.cpp/h`

`      vpiv\_utils.cpp/h`

`  /configuration`

`      config.cpp/h`

`      urg.cpp/h`

`  /navigation`

`      Odom.cpp/h`

`  /sensors`

`      us.cpp/h`

`      mic.cpp/h`

`      fs.cpp/h`

`      mvt\_ir.cpp/h`

`  /safety`

`      mvt\_safe.cpp/h`

`  /actuators`

`      mtr.cpp/h`

`      mtr\_hardware.cpp/h`

`      lring.cpp/h`

`      lrub.cpp/h`

`      srv.cpp/h`
```

### Communication MEGA → UNO

Format **ASCII**, très fiable : - 

```
`\<L,R,A\> - ZZ ATTENTION A Vérifier à peut-être évolué`
```

- `L` = vitesse gauche signée (-400..400)

- `R` = vitesse droite signée (-400..400)

- `A` = niveau d’accélération (0..4)

**UNO lisse automatiquement et applique au driver Pololu Dual VNH5019.**

## Structure du script de tasker

# Annexes

## Historique des versions


| Version | Date | Modifications Principales |
| :-: | :-: | :-: |
| RZV6 | 17/12/2025 | Stabilisation du protocole VPIV, ajout des urgences. |
| RZV5 | 01/11/2025 | Refonte de l’architecture modulaire. |
| RZV4 | 15/06/2025 | Intégration de Node-RED et MQTT. |


## Schémas

## Présentation des modules

### Philosophie générale

Le firmware du robot RZ repose sur trois principes fondamentaux :

1. **Séparation stricte des responsabilités**

2. **Communication textuelle unifiée (VPIV)**

3. **Sécurité prioritaire et transversale**

Ces principes sont appliqués **sans exception** sur l’ensemble des modules.

### Découpage en couches

L’architecture suit une **règle stricte en trois couches**, valable pour tous les modules fonctionnels :

| Couche | Rôle |
| :-: | :-: |
| Hardware | Accès direct aux broches, timers, périphériques |
| Métier | Logique fonctionnelle, calculs, décisions |
| Communication | Interface VPIV, validation des commandes |

Aucune couche ne doit violer cette règle.



\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

ANNEXES THÉMATIQUE

## Table A


\#\#- Une table A (table mère) que nous développons dans ce chat est un I**nventaire contractuel des variables (modèle conceptuel & contractuel), doit permettre une vue d’ensemble sur un fichier csv .  
\#\#\# régles applicables à la table A**

***Cette table décrit toutes les variables manipulées par le système, qu’elles soient :  
- échangées entre SP, SE et Arduino,  
- internes à Node-RED,  
- purement locales à un sous-système.**

***Elle sert de référence unique pour :  
- la construction des VPIV,  
- la validation des intents,  
- la cohérence STT / UI / logique métier,  
- la maintenabilité long terme.**

***  
- Quand dans le champ propriété un nom entre parenthèse il ne s’agit pas d’une propriété, mais d’une commande ponctuelle : Une propriété qui ne fait qu’un effet immédiat , CAT : V seule possible si retour ailleurs  
exemple de commandes ponctuelles : (emg), (reset), (clear), (init), (reboot), (calibrate), on les notes comme des propriétés en les mettant entre parenthèses, mais ce sont plus des fonctions.  
A ne pas confondre avec une propriété potentiellement de type flux et dont on ne veux qu’une valeur. Dans ce cas on note la propriété normalement, mais on choisis une CAT V au lieux de CAT F.**

**- Un état durable = propriété -\> dans le module qui porte la responsabilité réelle  **

***-  Types acceptés : boolean,  number, string,   
enum (interne seulement, string sur VPIV), object (sous conditions).  
**

***La Table A accepte explicitement le TYPE object  
Ce n’est pas une dérive, c’est un cas prévu pour :**

***paramètres de calibration,  
constantes algorithmiques,  
blocs de configuration rarement modifiés,  
messages “paquet” cohérents (évite 12 VPIV atomiques)  
  
⚠️ Ce n’est pas contradictoire avec Node-RED, à condition de respecter les règles suivantes.**

**Règles strictes quand TYPE = object**

**🔹 Règle 1 —Le type objet peut être utilisé pour la configuration, pas du temps réel, exemple : paraM = calibration → INITIALE = ini, pas de flux continu**

***🔹 Règle 2 — VPIV transporte une représentation sérialisée  
Le VPIV n’interprète pas l’objet, il le transporte.  
➡️ C’est Node-RED / SE qui désérialise, pas VPIV.**

***FORMAT\_VALUE = description sémantique  
VPIV VALUE = string sérialisée  
exemple :  
 $V:Gyro:paraM:\*:\{"frequence\_Hz":20,"freqOdo\_Hz":40,"seuilMesure\_xyz":\[10,10,5\],"seuilAlerte\_xy" :\[30,35\],"nbValPourMoy":8,"ajustMagnetic\_deg10":-50\}\# **

**🔹 Règle 3 — le type ENUM ne signifie pas un enum dans le vpiv, seulement que le string est normalisé et l’on fourni les valeurs alternatives possible (utile pour la cohérence )**

**🔹 Règle 4 — Pas de float implicite**

***Attention au multiplicateur éventuel pour éviter les float  
👉 C’est une règle d’or dans la Table A : les facteurs (x10, x100) doivent être documentés, jamais implicites**

**Précision /champs :**

*** 🔹 INITIAL:  
  
ini : Propriété initialisée par SP au démarrage (valeur par défaut chargée depuis JSON SP). Peut évoluer ensuite. Réinitialisé par reset.**

**Dyn : Propriété non initialisée au démarrage, produite uniquement en fonctionnement.**

**Cste : Propriété constante en exécution (modifiable uniquement hors run : flash, compilation, calibration usine). Pas de modif en cours de fonctionnement.**

**  🔹 STATUT :**

**ETAT :**

*** 👉 La propriété est : un retour de terrain, non pilotable directement ou issue d’un calcul ou d’un sous-système → souvent en CAT = I   
- Exemples : Mtr.targets, Urg.etat, Bat.level, Net.state  
📌 ETAT = information de statut réel, pas une intention. Exple : retour terrain**

** EXPOSE : ❓ Est-ce que cette propriété fait partie du modèle de décision ou de l’interface ? → EXPOSEE exemple : 👉 Gyro.paraM est clairement EXPOSEE.**

**\#\#Table C =  fichier JSON *Modèle de persistance & d’initialisation***

**Rôles cumulés (très importants) :**

1. **Valeurs par défaut (constants & initialisations)**

2. **Persistance inter-sessions**

3. **Source du point zéro**

4. **Seed des variables Node-RED (table B)**

**👉 Elle est l’unique source persistante.**

**Contenu :**

- **Variables système (table A)**

- **Variables internes SP qui doivent survivre au redémarrage**

- **Paramètres d’initialisation SE et Arduino**

**Paramètres de seuils, mappings, constantes métier**

**Elle est construite à partir de la Table A**

**\#\#Table B = *image de travail volatile -  Node-RED runtime***

- **`global`**

- **`flow`**

- **`context`**

- **calculs intermédiaires**

- **états temporaires**

- **flags de traitement**

- **buffers**

- **déductions**

**Propriétés clés :**

- **Initialisée à partir de C**

- **Modifiée pendant la session**

- **Jetée à la fermeture**

- **Peut contenir :**

  - **des miroirs de A**

  - **des dérivés**

  - **des états composites**

**👉 Elle ne doit JAMAIS être décrite dans la table A**
