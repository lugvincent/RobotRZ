# Table A — Structure et Règles des Champs
## Document de référence — Projet RZ

**Auteur :** Vincent Philippe  
**Version :** 2.0 — Mars 2026  
**Statut :** Référence stabilisée — à déposer dans le projet Claude.ai

---

## Rôle de la Table A dans le projet RZ

La Table A est la **référence unique et partagée** de toutes les variables manipulées par le système RZ, qu'elles soient échangées entre SP, SE et Arduino, ou internes à un sous-système. Elle sert simultanément à :

- La **construction correcte des trames VPIV** — chaque message peut être vérifié contre la table
- La **validation des intents STT** — le handler vocal sait quelles valeurs sont légales
- La **cohérence entre les couches** — interface SP, logique SE, firmware Arduino partagent la même définition
- La **maintenance long terme** — un développeur qui reprend le projet retrouve en un seul endroit toutes les règles

La Table A est **normative** : si un comportement du code diverge de la table, c'est le code qui est à corriger, pas la table (sauf découverte d'une erreur dans la table elle-même).
IMPORTANT : La Table A est la table mère source de l'implémentation du global.json de node-red coté SP . Pour cela elle suivra une table de conversion
---

## Liste synthétique des champs

Le tableau suivant donne la vue d'ensemble des 15 champs. La description détaillée de chacun suit dans la section 2.

| # | Nom retenu | Anciens noms / alias | Rôle en une ligne |
|---|-----------|---------------------|-------------------|
| 1 | **SOURCE** | Ref | Qui est responsable d'émettre ce message |
| 2 | **VAR** | — | Identifiant de la variable dans le protocole VPIV |
| 3 | **PROP** | propriété | Propriété de la variable ciblée |
| 4 | **CAT** | catégorie, type_message | Catégorie protocolaire du message VPIV |
| 5 | **TYPE** | type_valeur, type_logique | Famille logique de la valeur transportée |
| 6 | **FORMAT_VALUE** | format, valeurs_acceptées, Abrev-indices | Valeurs légales et format de pattern |
| 7 | **CODAGE** | *(nouveau)* | Unité de mesure et facteur multiplicateur |
| 8 | **DIRECTION** | sens, SENS, flux | Sens de circulation du message VPIV |
| 9 | **INSTANCE** | — | Instances valides pour cette propriété |
| 10 | **COMPLEXE** | — | Documentation des sous-champs pour TYPE=object |
| 11 | **CYCLE** | INITIAL, init, initialisation | Moment et mode de mise en place de la valeur |
| 12 | **NATURE** | STATUT, statut_propriété | Rôle sémantique du message dans le système |
| 13 | **INTERFACE** | *(extrait de STATUT=EXPOSEE)* | Propriété visible et pilotable depuis SP |
| 14 | **VPIV_EX** | VPIV-ACTIONS, exemple_vpiv, exemple | Trame VPIV complète illustrative |
| 15 | **NOTES** | COMMENTAIRES, remarques, précautions | Rôle fonctionnel et précautions libres |

> **Note sur le champ COMPLEXE :** Ce champ existait dans la table précédente et couvre un besoin réel pour les propriétés de TYPE=object. Sa description détaillée est dans la section 2.10 ci-dessous.

---

## Description détaillée de chaque champ

---

### 1. SOURCE

**Question à laquelle il répond :** *Quel composant du système est responsable d'émettre ce message VPIV ?*

#### Signification

SOURCE identifie l'**émetteur logique** de la ligne — c'est-à-dire le composant qui construit et publie le message VPIV décrit par cette ligne. Cette information est distincte de DIRECTION (qui décrit le trajet complet) : SOURCE précise l'origine, DIRECTION précise le chemin.

L'information de SOURCE est particulièrement utile dans LibreOffice Calc pour filtrer rapidement toutes les lignes émises par un composant donné — par exemple, auditer tout ce que SE publie, ou vérifier quelles urgences Arduino peut déclencher.

#### Valeurs acceptées

| Valeur | Signification |
|--------|--------------|
| `P` | SP (Node-RED) est le seul émetteur possible |
| `S` | SE (smartphone embarqué) est le seul émetteur possible |
| `A` | Arduino Mega est le seul émetteur possible |
| `E` | Soit SE soit Arduino peut émettre selon qui détecte la situation |

> **Précision sur E :** E signifie *"l'un ou l'autre selon le contexte"*, pas *"les deux simultanément"*. Par exemple, une urgence batterie peut venir de SE (qui surveille sa propre batterie) ou d'Arduino (qui surveille sa tension d'alimentation). E ne crée pas un bus partagé — il documente l'ambiguïté légitime de l'émetteur.

#### Règles

- Un seul code par ligne — pas de combinaison `S+A` sauf via `E`
- Si les deux composants émettent systématiquement (rare), créer deux lignes distinctes
- Le code `P` est presque exclusivement associé à `CAT=V` (SP émet les consignes)

#### Relation avec d'autres champs

SOURCE et DIRECTION sont complémentaires mais non redondants. `SOURCE=S` + `DIRECTION=SE→SP` précise que c'est bien SE (et non Arduino via le bridge) qui publie vers SP. La combinaison des deux lève l'ambiguïté dans les cas multi-émetteurs.

---

### 2. VAR

**Question à laquelle il répond :** *Quel est l'identifiant de la variable dans le protocole VPIV ?*

#### Signification

VAR est le **premier identifiant structurel du VPIV** — c'est le `VAR` dans `$CAT:VAR:PROP:INST:VAL#`. Il identifie un regroupement logique de propriétés partageant une responsabilité commune dans le système.

VAR est **partagé et compris identiquement par tous les composants** (SP, SE, Arduino). C'est ce caractère universel qui justifie de ne pas lui substituer le terme "module" : les modules (au sens des dossiers ou fichiers de code) peuvent différer d'un composant à l'autre, mais `Gyro` dans le VPIV désigne la même chose pour SP, SE et Arduino.

#### Règles

- Écriture en PascalCase : `CfgS`, `Gyro`, `LRing`, `CamSE`, `MicroSE`
- Stable dans le temps — un changement de VAR casse tous les parsers simultanément
- Pas nécessairement aligné sur un nom de fichier ou de dossier dans le code

#### Exemples

```
Gyro     CamSE     Mtr     CfgS     Urg     COM
LRing    LRub      Srv     US       Odo     MvtIR
Mic      FS        Mag     MicroSE  Appli   STT
```

---

### 3. PROP

**Ancien nom :** propriété  
**Question à laquelle il répond :** *Quelle propriété de la variable est ciblée par ce message ?*

#### Signification

PROP est le **deuxième identifiant structurel du VPIV** — le `PROP` dans `$CAT:VAR:PROP:INST:VAL#`. Il désigne une propriété spécifique de la variable, c'est-à-dire une dimension particulière de son état ou de son comportement.

#### Règles

- Écriture en camelCase : `modeGyro`, `active`, `streamURL`, `cmd`
- **Ne plus utiliser les parenthèses** `(reset)`, `(clear)` — cette convention est remplacée par `NATURE=Event` (voir champ 12)
- Un même nom de PROP peut apparaître dans plusieurs VAR différentes sans ambiguïté (ex : `active` existe pour `CamSE`, `MicroSE`, `LRing`...)
- Le couple VAR+PROP doit être unique dans la table pour une SOURCE et une DIRECTION données

---

### 4. CAT

**Anciens noms :** catégorie
**Question à laquelle il répond :** *Quelle est la nature protocolaire de ce message VPIV ?*

#### Signification

CAT est la **catégorie protocolaire** — le premier caractère du VPIV `$CAT:...#`. Il détermine comment le destinataire doit interpréter et traiter le message au niveau du protocole.

**Important :** depuis la version 2.0 de la Table A, CAT est **purement protocolaire**. Il ne porte plus d'information sémantique sur le rôle du message (ACK, Etat, Intent...). Cette information est désormais dans le champ NATURE. Ce découplage rend CAT plus stable et plus lisible.

#### Valeurs acceptées

| Valeur | Nom | Usage | Direction habituelle |
|--------|-----|-------|---------------------|
| `V` | Consigne | Ordre à exécuter par le destinataire | SP→SE, SP→A |
| `I` | Information | Retour d'état, publication d'information | SE→SP, A→SP |
| `E` | Événement critique | Urgence nécessitant une réaction automatique | SE→SP, A→SP |
| `F` | Flux | Données continues, publiées à fréquence régulière | SE→SP, A→SP |
| `A` | Application | Commande applicative Android (Tasker, apps) | SP→SE |

#### Règles

- `CAT=E` est **exclusivement réservé aux urgences** — situations critiques où SP doit réagir automatiquement sans intervention humaine. Ne pas utiliser E pour les informations importantes mais non urgentes.
- `CAT=F` implique une publication périodique. Si la donnée n'est publiée qu'à la demande, utiliser `I` ou `V` selon le sens.
- `CAT=V` implique que le destinataire *doit* exécuter. Si le destinataire peut refuser (ex : urgence active), il publie une erreur COM en retour.
- La distinction entre `I` et `F` porte sur la fréquence : `F` est un flux automatique et régulier, `I` est une publication déclenchée par un événement ou une demande.

---

### 5. TYPE

**Question à laquelle il répond :** *De quelle famille logique est la valeur transportée dans le VPIV ?*

#### Signification

TYPE décrit la **nature logique de la valeur** dans le message VPIV — pas le type de la variable en C++, pas le type JavaScript, pas le type Python. C'est le type au niveau du protocole, c'est-à-dire la famille qui détermine *comment interpréter et valider* la valeur transportée comme chaîne de texte.

Cette distinction est fondamentale : `boolean` en Table A ne signifie pas qu'Arduino utilise `bool`. Cela signifie que la valeur dans le VPIV sera `"1"` ou `"0"`, et que le code Arduino la convertira en `bool` via `strcmp(value, "1") == 0`. TYPE documente le contrat du protocole, pas l'implémentation interne.

#### Valeurs acceptées

| Valeur | Signification | Représentation dans le VPIV |
|--------|--------------|----------------------------|
| `boolean` | Vrai ou faux | `"1"` ou `"0"` — jamais `true/false` |
| `number` | Valeur numérique entière | Entier sans décimale — les flottants sont interdits |
| `string` | Chaîne de caractères | Pattern libre ou structuré (voir FORMAT_VALUE) |
| `enum` | Valeur parmi un ensemble fini normalisé | String parmi la liste définie dans FORMAT_VALUE |
| `object` | Structure JSON complexe sérialisée | JSON inline entre `{}` dans le VPIV |

#### Règles

- **Pas de float dans le VPIV** — les valeurs décimales sont encodées en entiers avec un multiplicateur (×10, ×100...) documenté dans CODAGE
- `TYPE=object` est autorisé uniquement pour la configuration (paramètres initiaux, calibration, blocs rarement modifiés). Jamais pour le temps réel ou les flux fréquents — cela rendrait le parsing trop coûteux et le débogage difficile
- `TYPE=enum` ne précise pas les valeurs dans ce champ — elles sont dans FORMAT_VALUE
- Pour `TYPE=object`, le champ COMPLEXE documente la structure interne (voir champ 10)

#### Relation avec FORMAT_VALUE et CODAGE

Ces trois champs forment un triplet :
- **TYPE** dit *quelle famille*
- **FORMAT_VALUE** dit *quelles valeurs exactes sont légales*
- **CODAGE** dit *comment interpréter la valeur* (unité, multiplicateur)

TYPE seul est insuffisant pour implémenter ou valider. FORMAT_VALUE est le champ le plus précis pour l'implémentation.

---

### 6. FORMAT_VALUE

**Anciens noms :** format, valeurs_acceptées, Abrev-indices  
**Question à laquelle il répond :** *Quelles sont exactement les valeurs légales et leur format dans le VPIV ?*

#### Signification

FORMAT_VALUE est le **contrat de validation de la valeur** — il précise ce qui est accepté dans le champ VAL du VPIV. C'est le champ le plus concret pour l'implémentation : un développeur qui lit FORMAT_VALUE sait exactement quoi écrire dans le message et comment valider ce qu'il reçoit.

Depuis la version 2.0, FORMAT_VALUE est **recentré sur les valeurs et leur format**. L'unité et le multiplicateur sont sortis dans le champ CODAGE. FORMAT_VALUE ne contient plus que :
- La liste des valeurs acceptées (pour enum)
- La plage numérique (pour number)
- Le format du pattern (pour string)
- La valeur fixe (pour boolean)

#### Contenu selon le TYPE

| TYPE | Contenu de FORMAT_VALUE | Exemples |
|------|------------------------|---------|
| `boolean` | `0\|1` — toujours ces deux valeurs exactes | `0\|1` |
| `number` | Plage : `min–max` ou `> 0` ou `0–255` | `0–3600`, `> 0`, `0–255` |
| `enum` | Liste exhaustive séparée par `\|` | `OFF\|DATACont\|DATAMoy\|ALERTE` |
| `string` | Description du pattern attendu | `"R,G,B"`, `"v,omega,a"`, `"x,y,z"` |
| `object` | Description des champs principaux | voir champ COMPLEXE pour le détail |

#### Règles

- Pour `enum` : la liste doit être **exhaustive** — si une valeur n'est pas dans FORMAT_VALUE, elle est invalide
- Pour `number` : préciser la plage même si elle est large — `0–65535` est plus utile que juste `number`
- Ne pas inclure l'unité dans FORMAT_VALUE — elle appartient à CODAGE
- Ne pas inclure le multiplicateur dans FORMAT_VALUE — il appartient à CODAGE
- Pour `TYPE=object` : indiquer une description synthétique ici, détailler dans COMPLEXE

---

### 7. CODAGE

**Ancien nom :** *(champ nouveau en v2.0)*  
**Question à laquelle il répond :** *Comment interpréter la valeur brute du VPIV — quelle unité représente-t-elle et y a-t-il un multiplicateur à appliquer ?*

#### Signification

CODAGE documente la **transformation entre la valeur brute du VPIV et la grandeur physique réelle**. Ce champ répond à deux questions fréquentes lors de l'implémentation : *"En quelle unité est cette valeur ?"* et *"Dois-je diviser par 10 pour retrouver la valeur réelle ?"*

La règle fondamentale du projet RZ interdit les valeurs décimales dans le VPIV pour des raisons de robustesse du parsing. Les angles, fréquences ou mesures avec décimales sont encodés en entiers avec un facteur multiplicateur. CODAGE documente ce facteur de façon explicite et normative — c'est une **règle d'or du projet** : aucun multiplicateur ne doit être implicite.

#### Valeurs

| Contenu | Signification | Exemple |
|---------|--------------|---------|
| `_` | Valeur brute sans transformation — pas d'unité, pas de multiplicateur | boolean, enum |
| `unité` | Unité physique sans multiplicateur | `Hz`, `ms`, `%`, `°C` |
| `unité × N` | Valeur dans le VPIV = grandeur réelle × N | `deg × 10`, `rad/s × 100`, `µT × 100` |
| `m × N` | Pour distance | `m × 100` (donne des cm entiers) |
| détail par sous-champ | Pour TYPE=object, chaque sous-champ documenté | voir COMPLEXE |

#### Règles

- Toujours renseigner `_` explicitement quand il n'y a pas de transformation — ne pas laisser vide
- Le multiplicateur est toujours une puissance de 10 dans ce projet (×10, ×100, ×1000)
- La valeur dans CODAGE est la source de vérité pour les conversions — si le code diverge, c'est une erreur
- Pour `TYPE=object`, CODAGE peut renvoyer à COMPLEXE avec la mention `voir COMPLEXE`

---

### 8. DIRECTION

**Anciens noms :** sens, SENS, flux  
**Question à laquelle il répond :** *Dans quel sens circule ce message VPIV ?*

#### Signification

DIRECTION décrit le **trajet complet du message** — de qui l'émet à qui le reçoit. C'est une information de routage qui permet de comprendre l'architecture d'échange sans avoir à tracer le code.

DIRECTION est complémentaire de SOURCE : SOURCE dit *qui émet*, DIRECTION dit *vers qui*. La combinaison des deux lève les ambiguïtés dans les cas où plusieurs composants peuvent émettre.

#### Valeurs acceptées

| Valeur | Signification |
|--------|--------------|
| `SP→SE` | De SP vers SE |
| `SE→SP` | De SE vers SP |
| `SP→A` | De SP vers Arduino (via SE et bridge) |
| `A→SP` | D'Arduino vers SP (via bridge et SE) |
| `SE↔SP` | Dans les deux sens (propriété bidirectionnelle) |
| `SP→SE→A` | De SP vers Arduino, transité par SE |
| `A→SE→SP` | D'Arduino vers SP, transité par SE |

#### Règles

- La direction `SP→SE→A` signifie que SE est un **relais transparent** — il ne traite pas le message, il le transfère
- Une propriété bidirectionnelle `SE↔SP` doit généralement avoir deux lignes dans la table : une pour chaque sens, avec des CAT et NATURE différents (ex : `V` pour SP→SE, `I(ACK)` pour SE→SP)
- Arduino ne communique jamais directement avec SP sans passer par SE et le bridge MQTT

---

### 9. INSTANCE

**Ancien nom :** INSTANCE (stable)  
**Question à laquelle il répond :** *Quelles instances de cette variable cette propriété peut-elle cibler ?*

#### Signification

Certaines variables ont plusieurs instances — par exemple `CamSE` a une instance `rear` (caméra arrière) et une instance `front` (caméra frontale). INSTANCE documente quelles valeurs d'instance sont valides pour cette propriété spécifique, et si la propriété accepte l'instance universelle `*`.

#### Valeurs

| Valeur | Signification |
|--------|--------------|
| `*` | Instance unique ou toutes les instances |
| liste | Instances nommées : `rear\|front`, `TGD\|THB\|BASE` |
| `*\|liste` | Propriété acceptant à la fois `*` et les instances nommées |
| index | Instances numériques : `0–8` pour US (9 capteurs) |

#### Règles

- Si le module est mono-instance, INSTANCE = `*` systématiquement
- `*` dans le VPIV signifie selon le contexte *"toutes les instances"* (pour une consigne) ou *"instance unique"* (pour un module sans plusieurs instances)
- Le dispatcher Arduino vérifie l'instance avec `instIsAll()` de `vpiv_utils.h`

---

### 10. COMPLEXE

**Ancien nom :** COMPLEXE (stable)  
**Question à laquelle il répond :** *Pour TYPE=object, quelle est la structure interne de l'objet JSON transporté ?*

#### Signification

Pour les propriétés de `TYPE=object`, la valeur dans le VPIV est un bloc JSON sérialisé. COMPLEXE documente la **structure interne de cet objet** : quels sous-champs existent, leur type, leur unité, leur plage de valeurs.

Sans COMPLEXE, une propriété object est une boîte noire dans la table — on sait qu'il y a un objet mais on ne sait pas ce qu'il contient. COMPLEXE rend l'objet transparent et maintenable.

#### Contenu

COMPLEXE contient une liste de sous-champs au format :

```
nom_sous_champ : TYPE, plage, unité×facteur
```

Exemple pour `Gyro.paraM` :

```
freqCont      : number, > 0, Hz
freqMoy       : number, > 0, Hz
nbValPourMoy  : number, > 0, _
seuilMesure   : object {x,y,z : number, ≥ 0, rad/s × 100}
seuilAlerte   : object {x,y   : number, > 0, rad/s × 100}
angleVSEBase  : number, 0–3600, deg × 10
```

#### Règles

- Obligatoire dès que `TYPE=object`
- Chaque sous-champ doit avoir au minimum : nom, TYPE, plage ou valeurs valides
- Les unités et multiplicateurs des sous-champs sont documentés ici (et non dans CODAGE qui peut renvoyer à COMPLEXE)
- Un objet imbriqué (sous-objet dans un objet) est documenté avec indentation

---

### 11. CYCLE

**Ancien nom :** INITIAL, init, initialisation  
**Question à laquelle il répond :** *Quand cette valeur est-elle mise en place, et peut-elle évoluer au runtime ?*

#### Signification

CYCLE documente le **cycle de vie de la valeur** — quand elle est initialement posée, et si elle peut changer pendant l'exécution. C'est une information de comportement dynamique qui aide à comprendre la stabilité de la propriété.

#### Valeurs acceptées

| Valeur | Signification |
|--------|--------------|
| `Ini` | Initialisée par SP au démarrage (depuis courant_init.json). Peut évoluer ensuite via commande V |
| `Dyn` | Produite uniquement en fonctionnement, pas d'initialisation au démarrage |
| `Cste` | Constante en exécution — modifiable uniquement hors run (flash firmware, calibration usine). Exemples : entraxe des roues, brochage, nom des instances |

#### Règles

- `Cste` est réservé aux valeurs qui ne changent pas sans intervention physique ou reflash
- `Ini` implique qu'il existe une valeur par défaut dans le firmware qui sera écrasée par SP au démarrage
- `Dyn` s'applique typiquement aux valeurs calculées ou mesurées (position odométrique, cap magnétique, température...)
- Une valeur `Cste` n'a généralement pas de ligne `CAT=V` dans la table — on ne l'envoie pas au runtime

---

### 12. NATURE

**Ancien nom :** STATUT, statut_propriété  
**Question à laquelle il répond :** *Quel est le rôle sémantique de ce message dans le système ?*

#### Signification

NATURE décrit **ce que le message fait dans la logique du système** — indépendamment de sa catégorie protocolaire (CAT). Alors que CAT dit *comment* le message est traité au niveau du protocole, NATURE dit *pourquoi* ce message existe et ce qu'il représente fonctionnellement.

Ce découplage entre CAT et NATURE est un gain majeur de la v2.0 : CAT reste un code technique stable à 5 valeurs, NATURE porte la sémantique riche sans alourdir CAT.

**Important :** NATURE remplace également la convention des parenthèses dans PROP. Une propriété qui était notée `PROP=(reset)` devient `PROP=reset` + `NATURE=Event`.

#### Valeurs acceptées

| Valeur | Signification | CAT typiquement associé |
|--------|--------------|------------------------|
| `Config` | Paramètre de configuration pilotable, valeur durable | V |
| `ACK` | Confirmation miroir d'une commande V reçue — même VAR/PROP, sens inverse | I |
| `Etat` | Retour terrain — état réel mesuré ou calculé, non directement pilotable | I ou F |
| `Intent` | Retour de validation d'un intent STT par SP | I |
| `Event` | Commande ponctuelle — déclenche un effet immédiat unique, pas d'état persistant | V |
| `_` | Pas de qualification sémantique particulière | I |

#### Règles

- **NATURE=Event remplace les parenthèses dans PROP** — `(reset)` devient `reset` + `NATURE=Event`. Les parenthèses ne doivent plus apparaître dans PROP
- **NATURE=ACK implique une ligne miroir** — pour chaque ligne `CAT=V` avec retour attendu, il doit exister une ligne `NATURE=ACK` de sens inverse dans la table
- **NATURE=Etat** s'applique aux propriétés qui décrivent un état réel mesuré — elles ne sont pas directement commandables via `CAT=V`, elles sont produites par le composant qui observe
- `_` signifie explicitement "pas de qualification particulière" — ne pas laisser le champ vide
- Les urgences (`CAT=E`) ont généralement `NATURE=Etat` — elles publient un état critique détecté

---

### 13. INTERFACE

**Ancien nom :** extrait de `STATUT=EXPOSEE`  
**Question à laquelle il répond :** *Cette propriété est-elle visible et pilotable depuis l'interface SP (Node-RED) ?*

#### Signification

INTERFACE indique si la propriété doit apparaître dans le tableau de bord Node-RED — en lecture, en écriture ou les deux. C'est une information de conception d'interface, distincte des informations protocolaires.

Ce champ a été extrait de l'ancienne valeur `STATUT=EXPOSEE` pour deux raisons : d'abord parce que STATUT (devenu NATURE) ne devait pas mélanger des préoccupations protocolaires et des préoccupations UI ; ensuite parce qu'une propriété peut être à la fois `NATURE=Config` ET être exposée dans l'interface, ce qu'un seul champ ne pouvait pas représenter.

#### Valeurs acceptées

| Valeur | Signification |
|--------|--------------|
| `oui` | Propriété visible et/ou pilotable dans l'interface SP |
| `_` | Propriété interne — non exposée dans l'interface |

#### Règles

- Une propriété `INTERFACE=oui` doit avoir une représentation dans les flux Node-RED (dashboard, inject, debug...)
- Une propriété `INTERFACE=_` peut toujours être envoyée via MQTT manuellement, elle n't simplement pas d'élément d'interface dédié
- Les propriétés `NATURE=Etat` avec `INTERFACE=oui` apparaissent typiquement en lecture seule dans l'interface
- Les propriétés `NATURE=Config` avec `INTERFACE=oui` apparaissent avec un contrôle d'édition

---

### 14. VPIV_EX

**Anciens noms :** VPIV-ACTIONS, exemple_vpiv, exemple  
**Question à laquelle il répond :** *À quoi ressemble une trame VPIV réelle pour cette propriété ?*

#### Signification

VPIV_EX fournit une **trame VPIV complète et syntaxiquement correcte** illustrant la propriété avec une valeur d'exemple représentative. C'est la passerelle entre la description abstraite de la table et le message concret qui circulera sur le réseau.

VPIV_EX n'est pas redondant avec FORMAT_VALUE et CODAGE — il joue un rôle différent : il montre la **syntaxe complète** du message (`$`, `:`, `#`, accolades pour les objets, guillemets éventuels) qu'on ne peut pas déduire des autres champs seuls. C'est particulièrement critique pour `TYPE=object` où une accolade manquante rend le message illisible.

#### Contenu

Format attendu : trame complète avec le préfixe `$`, les 4 séparateurs `:`, la valeur d'exemple, et le suffixe `#`.

```
$V:Gyro:modeGyro:*:DATACont#
$F:Gyro:dataCont:*:[1200,-450,9800]#
$V:Gyro:paraM:*:{"freqCont":20,"freqMoy":2,"nbValPourMoy":5}#
$I:CamSE:active:rear:On#
$E:Urg:source:SE:URG_LOW_BAT#
```

#### Règles

- **Obligatoire pour toutes les lignes**, y compris `TYPE=object`
- Pour `TYPE=object` : la trame doit montrer la syntaxe JSON complète avec des valeurs représentatives — c'est le template que le développeur copiera et adaptera
- La valeur d'exemple doit être **valide** selon FORMAT_VALUE — pas une valeur hors plage "pour l'exemple"
- Si plusieurs trames illustrent mieux le comportement (ex : On et Off), les séparer par un saut de ligne dans la cellule

---

### 15. NOTES

**Anciens noms :** COMMENTAIRES, remarques, précautions  
**Question à laquelle il répond :** *Quel est le rôle fonctionnel de cette propriété, et y a-t-il des précautions à connaître ?*

#### Signification

NOTES est un champ de **documentation libre** destiné à porter les informations qui ne rentrent dans aucun autre champ. Son contenu est en langage naturel, court (2 à 4 phrases maximum par note) et structuré autour de deux axes : le rôle fonctionnel (à quoi sert cette propriété concrètement dans le système) et les précautions (ce qui peut mal tourner ou les contraintes non évidentes).

Avec la v2.0, NOTES est **allégé** : les unités et multiplicateurs sont dans CODAGE, les valeurs légales dans FORMAT_VALUE, les codes source dans SOURCE. NOTES ne répète pas ces informations.

#### Contenu typique

- Le rôle fonctionnel de la propriété : *"Détermine la fréquence à laquelle le gyroscope publie ses données vers SP. Influe directement sur la charge CPU de SE."*
- Les contraintes et précautions : *"Doit être supérieur à 0. Valeurs > 50 Hz peuvent saturer le bus MQTT."*
- Les dépendances non évidentes : *"Modifié automatiquement par rz_state-manager.sh lors d'un changement de modeRZ."*
- Les renvois à d'autres documents : *"Voir Table D pour l'usage dans le contexte STT."*

#### Règles

- Pas de répétition des informations déjà présentes dans d'autres champs
- Pas de trame VPIV dans NOTES — elles appartiennent à VPIV_EX
- Pas d'unité ni de multiplicateur dans NOTES — ils appartiennent à CODAGE
- Langage français, phrases courtes, ton technique
- Si NOTES est vide : laisser `_`

---

## Tableau de synthèse — relations entre champs

Le tableau suivant récapitule comment les champs se complètent et se distinguent pour les trois dimensions principales : *identification*, *valeur*, et *comportement*.

### Dimension identification

| Champ | Répond à | Redondance avec |
|-------|---------|----------------|
| SOURCE | Qui émet | Partiellement DIRECTION (émetteur) |
| VAR | Quelle variable | — |
| PROP | Quelle propriété | — |
| INSTANCE | Quelle instance | — |
| DIRECTION | Quel trajet | Partiellement SOURCE (origine) |

### Dimension valeur

| Champ | Répond à | Ce qu'il n'inclut pas |
|-------|---------|----------------------|
| TYPE | Quelle famille | Ni les valeurs exactes, ni l'unité |
| FORMAT_VALUE | Quelles valeurs sont légales | Ni l'unité, ni le multiplicateur |
| CODAGE | Comment interpréter la valeur brute | Ni les valeurs légales, ni le format |
| COMPLEXE | Structure interne si object | — |
| VPIV_EX | Syntaxe complète avec exemple | — |

### Dimension comportement

| Champ | Répond à | Ce qu'il n'inclut pas |
|-------|---------|----------------------|
| CAT | Traitement protocolaire | Le rôle sémantique |
| NATURE | Rôle sémantique | La catégorie protocolaire |
| CYCLE | Quand la valeur est posée | — |
| INTERFACE | Visible dans SP ? | — |
| NOTES | Rôle fonctionnel + précautions | Tout ce qui a un champ dédié |

---

## Règles générales de la Table A

Ces règles s'appliquent à la table dans son ensemble, indépendamment des champs individuels.

**Une ligne = un message VPIV possible.** Si une propriété peut être émise dans deux sens différents (Config + ACK), elle donne deux lignes distinctes.

**Pas de float dans le VPIV.** Toutes les valeurs décimales sont encodées en entiers avec un multiplicateur documenté dans CODAGE. Cette règle protège le parsing et évite les erreurs d'arrondi.

**Les parenthèses dans PROP sont supprimées.** La convention `(reset)`, `(clear)` est remplacée par `NATURE=Event`. Les nouvelles lignes n'utilisent plus les parenthèses. Les lignes existantes seront migrées lors de la re-instanciation.

**TYPE=object est réservé à la configuration.** Jamais pour les flux temps réel. La sérialisation JSON est coûteuse et difficile à déboguer dans un flux continu.

**Chaque champ vide est remplacé par `_`.** Les cellules vides dans un tableau sont une source d'ambiguïté : est-ce vide intentionnellement ou est-ce un oubli ? `_` signifie explicitement "sans valeur applicable".

**La table est normative.** En cas de divergence entre la table et le code, la table a raison sauf erreur démontrée. Une divergence doit être résolue, pas ignorée.

---

*Document de référence Table A — Structure et Règles — Projet RZ*  
*Version 2.0 — Mars 2026 — Vincent Philippe*
