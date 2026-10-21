# TABLES RZ — DESCRIPTEUR DE RÉFÉRENCE

> ## 🔴 CE FICHIER MARKDOWN EST LA VERSION OFFICIELLE
>
> Un export `.odt` existe pour le confort de lecture dans Writer. **Il n'est qu'une copie.**
> En cas d'écart entre les deux, **ce fichier a raison**. Toute modification se fait ici,
> puis l'ODT est réexporté. Jamais l'inverse.

**Fichier** : `docs/tables_RZ_descripteur.md` (dépôt Git)
**Objet** : décrire les tables A, C et D du projet RobotRZ — leur rôle, la structure de leurs champs, la chaîne qui les relie au code, leur histoire et les erreurs rencontrées.
**Remplace** : `table_A_structure_regles.md` v2.0 (mars 2026), dont il reprend et actualise la description des champs.
**Version** : 3.0 — 21/10/2026 — Vincent Philippe / Claude

---

## Sommaire

1. [À quoi sert ce document](#1)
2. [Les quatre tables et leur rôle](#2)
3. [Qui fait foi, et la chaîne vers le code](#3)
4. [Table A — structure des quinze champs](#4)
5. [Le protocole VPIV](#5)
6. [Histoire des tables](#6)
7. [Erreurs rencontrées — journal](#7)
8. [Points ouverts](#8)

---

<a id="1"></a>
## 1. À quoi sert ce document

Il répond à trois questions que l'on se pose en reprenant le projet après une pause.

**Que signifie ce champ ?** La section 4 décrit les quinze colonnes de la Table A, une par une, avec leurs valeurs légales et leurs règles.

**Quel fichier dois-je modifier ?** La section 3 donne la chaîne complète, du classeur que vous éditez jusqu'au fichier que le robot consomme.

**Pourquoi cette valeur bizarre ?** La section 7 tient le journal des erreurs rencontrées et corrigées. Plusieurs d'entre elles ont coûté des heures : elles sont consignées pour ne pas se reproduire.

---

<a id="2"></a>
## 2. Les quatre tables et leur rôle

| Table | Rôle | Chargée à l'exécution ? |
|---|---|---|
| **A** | Référence normative de **toutes** les variables VPIV du système. 253 lignes, 24 variables, 15 colonnes. | **Non, jamais.** Elle sert à concevoir, pas à faire tourner. |
| **B** | `enrichedVars` dans Node-RED, sur disque `globalSave.json`. Variables d'amorçage de SP. | Oui, au démarrage de SP. Volatile. |
| **C** | `courant_init.json` — configuration opérationnelle persistante de SE et Arduino. | Oui, au démarrage de SE. |
| **D** | Catalogue des commandes vocales STT. Fait le pont entre une phrase prononcée et une trame VPIV. | Oui, via `stt_catalog.json` sur SE. |

**La Table A est normative.** Si le code diverge de la table, c'est le code qu'on corrige — sauf erreur démontrée dans la table elle-même, auquel cas la table est corrigée et l'erreur consignée en section 7.

**La Table A est aussi la table mère** du `globalSave.json` de Node-RED côté SP, via une table de conversion.

---

<a id="3"></a>
## 3. Qui fait foi, et la chaîne vers le code

### 3.1 Le principe

**Le classeur `.ods` est la source — vous l'éditez. Le `.csv` est un produit — vous ne l'éditez jamais.**

Un CSV modifié à la main est perdu au prochain export, et pire, il fait croire que la table dit quelque chose qu'elle ne dit pas.

### 3.2 Un seul CSV est une interface avec le code

Vérification faite sur l'ensemble du dépôt le 21/10/2026 : **seule la Table D est lue par du code.**

| Table | Son CSV est-il consommé ? | Conséquence |
|---|---|---|
| A | Non — aucun script ne la lit | Son nom de fichier est libre |
| C | Non — `tableC.csv` n'est lu par rien | Son nom de fichier est libre |
| **D** | **Oui, par trois scripts** | **Son nom est une interface : le changer casse le code** |

Les trois points d'ancrage du nom de la Table D :

```
original_init.sh      ligne 641 : CATALOGUE_CSV=".../tableD_catalogueV2.csv"   (il le GÉNÈRE)
rz_build_system.py    ligne 107 : FILE_CSV = 'tableD_catalogueV2.csv'
rz_build_grammar.py   ligne  65 : FILE_CSV = 'tableD_catalogueV2.csv'
```

Et l'en-tête de `rz_stt_handler.sh` en annonce un quatrième, `tableD_catalogueV2_corrige.csv`.

### 3.3 ⚠️ La chaîne de la Table D est actuellement rompue

Deux chaînes coexistent et ne se rencontrent jamais :

```
Celle du code :
  original_init.sh (catalogue embarqué DANS le script, balise « === SECTION CATALOGUE STT === »)
     → tableD_catalogueV2.csv → rz_build_system.py → stt_catalog.json → le Doro

La vôtre :
  tableD_catalogueV3_corrige.ods → tableD_catalogueV3_corrige.csv → nulle part
```

**C'est l'explication complète du décalage connu : 70 commandes déployées sur le Doro contre 72 dans la table.** Les deux commandes manquantes, `APP_IA_CONV_ON` et `APP_IA_CONV_OFF`, n'ont jamais eu de chemin pour y arriver. Ce n'était pas un oubli de saisie, c'est une canalisation débranchée.

**La réparation** consiste à extraire le catalogue de `original_init.sh`, à le remplacer par une lecture de fichier, et à aligner les trois noms codés en dur. C'est le lot I de la synthèse d'état, prérequis des MACRO atomiques. À faire côté code, pas côté tables.

### 3.4 L'autorité à l'exécution

Règle posée le 18/10/2026 : **le PC est l'atelier de conception, le RPi porte l'autorité d'exécution** pour les tables B, C et D. Chaque sous-système garde une copie locale persistante lui permettant de démarrer seul ; SP l'écrase après `SE_READY`.

La Table A échappe à cette règle : elle vit dans Calc sur le PC et n'est jamais chargée.

---

<a id="4"></a>
## 4. Table A — structure des quinze champs

### 4.0 Vue d'ensemble

| # | Champ | Anciens noms | Rôle en une ligne |
|---|---|---|---|
| 1 | **SOURCE** | Ref, Srce | Qui est responsable d'émettre ce message |
| 2 | **VAR** | — | Identifiant de la variable dans le protocole VPIV |
| 3 | **PROP** | propriété | Propriété de la variable ciblée |
| 4 | **CAT** | catégorie, type_message | Catégorie protocolaire du message |
| 5 | **TYPE** | type_valeur, type_logique | Famille logique de la valeur transportée |
| 6 | **FORMAT_VALUE** | format, valeurs_acceptées | Valeurs légales et format de pattern |
| 7 | **CODAGE** | *(nouveau en v2.0)* | Unité et facteur multiplicateur |
| 8 | **DIRECTION** | sens, SENS, flux | Sens de circulation du message |
| 9 | **INSTANCE** | — | Instances valides pour cette propriété |
| 10 | **COMPLEXE** | — | Sous-champs documentés pour TYPE=object |
| 11 | **CYCLE** | INITIAL, init | Moment et mode de mise en place de la valeur |
| 12 | **NATURE** | STATUT, statut_propriété | Rôle sémantique du message |
| 13 | **INTERFACE** | *(extrait de STATUT=EXPOSEE)* | Propriété visible et pilotable depuis SP |
| 14 | **VPIV_EX** | VPIV-ACTIONS, exemple_vpiv | Trame VPIV complète illustrative |
| 15 | **NOTES** | COMMENTAIRES, remarques | Rôle fonctionnel et précautions |

⚠️ **L'intitulé de la colonne 1 diffère selon le format** : `SOURCE` dans le CSV, `Srce` dans l'ODS. Un script qui chercherait `SOURCE` échouerait sur un CSV réexporté depuis l'ODS. À harmoniser.

---

### 4.1 SOURCE — qui émet

Identifie l'**émetteur logique** de la ligne, c'est-à-dire le composant qui construit et publie le message. Distinct de DIRECTION, qui décrit le trajet complet.

| Valeur | Signification |
|---|---|
| `P` | SP (Node-RED) est le seul émetteur possible |
| `S` | SE (smartphone embarqué) est le seul émetteur possible |
| `A` | Arduino Mega est le seul émetteur possible |
| `E` | Soit SE soit Arduino selon qui détecte la situation |

**`E` signifie « l'un ou l'autre selon le contexte », pas « les deux simultanément ».** Une urgence batterie peut venir de SE, qui surveille sa propre batterie, ou d'Arduino, qui surveille sa tension d'alimentation. `E` ne crée pas un bus partagé : il documente une ambiguïté légitime de l'émetteur.

**Cas particulier des variables partagées.** `CfgS` et `Urg` ont des lignes `SOURCE=A` et `SOURCE=S` **explicitement séparées**, parce que les deux émettent réellement. `COM` utilise `SOURCE=E` : partagé aussi, mais par un mécanisme différent.

**Règles.** Un seul code par ligne, pas de combinaison `S+A` hors `E`. Si les deux composants émettent systématiquement, créer deux lignes. Le code `P` est presque exclusivement associé à `CAT=V`.

---

### 4.2 VAR — quelle variable

Premier identifiant structurel du VPIV. Identifie un regroupement logique de propriétés partageant une responsabilité.

VAR est **compris identiquement par SP, SE et Arduino**. C'est ce caractère universel qui justifie de ne pas lui substituer le mot « module » : les modules au sens des fichiers de code diffèrent d'un composant à l'autre, mais `Gyro` dans le VPIV désigne la même chose partout.

**Règles.** Écriture en PascalCase. Stable dans le temps — un changement de VAR casse tous les analyseurs simultanément. Pas nécessairement aligné sur un nom de fichier.

```
Gyro   CamSE   Mtr   CfgS   Urg      COM
LRing  LRub    Srv   US     Odom     IRmvt
Mic    FS      Mag   MicroSE  Appli  STT
```

---

### 4.3 PROP — quelle propriété

Deuxième identifiant structurel. Désigne une dimension particulière de l'état ou du comportement de la variable.

**Règles.** Écriture en camelCase : `modeGyro`, `active`, `streamURL`, `cmd`. **Les parenthèses sont abandonnées** — la convention `(reset)`, `(clear)` est remplacée par `NATURE=Event`. Un même nom de PROP peut exister dans plusieurs VAR sans ambiguïté. Le couple VAR+PROP doit être unique pour une SOURCE et une DIRECTION données.

---

### 4.4 CAT — catégorie protocolaire

Premier caractère de la trame. Détermine **comment** le destinataire traite le message au niveau du protocole.

Depuis la v2.0, CAT est **purement protocolaire** : il ne porte plus d'information sémantique sur le rôle du message. Cette information est dans NATURE. Ce découplage rend CAT stable et lisible.

| Valeur | Nom | Usage | Direction habituelle |
|---|---|---|---|
| `V` | Consigne | Ordre à exécuter par le destinataire | SP→SE, SP→A |
| `I` | Information | Retour d'état, ACK, publication | SE→SP, A→SP |
| `E` | Événement critique | Urgence nécessitant une réaction automatique | SE→SP, A→SP |
| `F` | Flux | Données continues, publiées à fréquence régulière | SE→SP, A→SP |
| `A` | Application | **Commande applicative Android** — Tasker, apps | SP→SE |

**Règles.** `CAT=E` est exclusivement réservé aux urgences — pas aux informations simplement importantes. `CAT=F` implique une publication périodique ; une donnée publiée à la demande relève de `I` ou `V`. `CAT=V` implique que le destinataire *doit* exécuter, ou publier une erreur COM en retour.

⚠️ **`CamSE` utilise `CAT=V` ou `CAT=F`, jamais `CAT=A`**, bien que le traitement soit délégué en interne à Tasker. La délégation est une affaire d'implémentation de SE, elle ne change pas la catégorie protocolaire.

---

### 4.5 TYPE — famille logique de la valeur

Décrit la nature logique de la valeur **au niveau du protocole** — pas le type C++, pas le type JavaScript.

La distinction est fondamentale : `boolean` en Table A ne signifie pas qu'Arduino utilise `bool`. Cela signifie que la valeur dans la trame sera `"1"` ou `"0"`, et que le code la convertira. TYPE documente le contrat du protocole, pas l'implémentation.

| Valeur | Signification | Dans la trame |
|---|---|---|
| `boolean` | Vrai ou faux | `"1"` ou `"0"` — jamais `true/false` |
| `number` | Entier | Pas de décimale — les flottants sont interdits |
| `string` | Chaîne | Pattern libre ou structuré |
| `enum` | Valeur dans un ensemble fini | Parmi la liste de FORMAT_VALUE |
| `object` | Structure JSON sérialisée | JSON inline entre `{}` |

**`TYPE=object` est réservé à la configuration** — paramètres initiaux, calibration, blocs rarement modifiés. Jamais pour le temps réel ni les flux : la sérialisation est coûteuse et le débogage difficile.

**TYPE, FORMAT_VALUE et CODAGE forment un triplet** : TYPE dit *quelle famille*, FORMAT_VALUE *quelles valeurs exactes*, CODAGE *comment interpréter*. TYPE seul ne suffit jamais à implémenter.

---

### 4.6 FORMAT_VALUE — les valeurs légales

Contrat de validation de la valeur. C'est le champ le plus concret pour l'implémentation.

| TYPE | Contenu | Exemples |
|---|---|---|
| `boolean` | Toujours ces deux valeurs | `0\|1` |
| `number` | La plage | `0–3600`, `> 0`, `0–255` |
| `enum` | Liste **exhaustive** séparée par `\|` | `OFF\|DATACont\|DATAMoy\|ALERTE` |
| `string` | Description du pattern | `"R,G,B"`, `"v,omega,a"` |
| `object` | Description synthétique | détail dans COMPLEXE |

**Ni l'unité ni le multiplicateur n'ont leur place ici** — ils appartiennent à CODAGE. Pour `enum`, une valeur absente de la liste est invalide.

---

### 4.7 CODAGE — unité et multiplicateur

Documente la transformation entre la valeur brute de la trame et la grandeur physique réelle.

La règle du projet interdit les décimales dans le VPIV, pour la robustesse du parsing. Les grandeurs décimales sont encodées en entiers avec un facteur. **C'est une règle d'or : aucun multiplicateur ne doit être implicite.**

| Contenu | Signification | Exemple |
|---|---|---|
| `_` | Aucune transformation | boolean, enum |
| `unité` | Unité sans multiplicateur | `Hz`, `ms`, `%`, `°C` |
| `unité × N` | Valeur dans la trame = réel × N | `deg × 10`, `rad/s × 100` |

**Codages en vigueur, vérifiés contre le code le 04/07/2026 :**

| Domaine | Règle | Vérifié dans |
|---|---|---|
| Générique | Entier ×1000 | `Mtr.scale`, `Mtr.kturn`, `SecMvt.scale` |
| Gyro | rad/s × 100 | — |
| Mag | µT × 100 | — |
| MicroSE RMS | RMS × 10 | `rz_balayage_sonore.sh` |
| MicroSE direction | deg × 10 | `rz_balayage_sonore.sh` |
| Odom position | `x_mm = x×1000`, `theta_mrad = theta×1000` | `odom.cpp` |
| Odom vitesse | `v_mm_s = v×1000`, `omega_mrad_s = omega×1000` | `odom.cpp` |

**Toujours renseigner `_` explicitement** quand il n'y a pas de transformation. Une cellule vide est une ambiguïté.

---

### 4.8 DIRECTION — le trajet

Décrit le trajet complet du message. Complémentaire de SOURCE, qui ne dit que l'origine.

| Valeur | Signification |
|---|---|
| `SP→SE` · `SE→SP` | Entre SP et SE |
| `SP→A` · `A→SP` | Entre SP et Arduino, via SE et le pont |
| `SE↔SP` | Bidirectionnel |
| `SP→SE→A` · `A→SE→SP` | Transité par SE, qui est **relais transparent** |

**Arduino ne communique jamais directement avec SP** sans passer par SE et le pont MQTT. Une propriété bidirectionnelle doit généralement donner deux lignes, avec des CAT et NATURE différents.

---

### 4.9 INSTANCE — quelles instances

| Valeur | Signification |
|---|---|
| `*` | Instance unique, ou toutes les instances |
| liste | Instances nommées : `rear\|front`, `TGD\|THB\|BASE` |
| `*\|liste` | Accepte `*` et les instances nommées |
| index | Instances numériques : `0–8` pour les neuf capteurs US |

Si le module est mono-instance, `INSTANCE = *` systématiquement. Le dispatcher Arduino vérifie l'instance avec `instIsAll()` de `vpiv_utils.h`.

---

### 4.10 COMPLEXE — structure des objets

Pour `TYPE=object`, documente la structure interne du JSON transporté. Sans lui, une propriété object est une boîte noire.

Format : `nom_sous_champ : TYPE, plage, unité×facteur`

```
freqCont      : number, > 0, Hz
freqMoy       : number, > 0, Hz
nbValPourMoy  : number, > 0, _
seuilMesure   : object {x,y,z : number, ≥ 0, rad/s × 100}
angleVSEBase  : number, 0–3600, deg × 10
```

⚠️ **COMPLEXE peut contenir deux choses différentes** : un simple drapeau `O`/`N`, ou la structure détaillée d'un objet imbriqué. **Toujours croiser FORMAT_VALUE *et* COMPLEXE** avant de conclure qu'une propriété structurelle est absente de la norme. Une vérification fondée sur le seul FORMAT_VALUE avait fait croire à tort que `paraCam` n'existait pas, le 29 juin 2026.

---

### 4.11 CYCLE — cycle de vie de la valeur

| Valeur | Signification |
|---|---|
| `Ini` | Initialisée par SP au démarrage depuis `courant_init.json`. Peut évoluer ensuite via `CAT=V` |
| `Dyn` | Produite uniquement au runtime. Pas de valeur au démarrage |
| `Cste` | Constante en exécution — ne change pas sans reflash ou calibration |

`Cste` est réservé aux valeurs qui ne changent pas sans intervention physique : entraxe des roues, brochage, noms d'instances. Une valeur `Cste` n'a généralement pas de ligne `CAT=V`.

---

### 4.12 NATURE — rôle sémantique

Dit **pourquoi** le message existe, là où CAT dit **comment** il est traité. Ce découplage est le gain majeur de la v2.0.

| Valeur | Signification | CAT associé |
|---|---|---|
| `Config` | Paramètre pilotable durable | V |
| `ACK` | Confirmation miroir d'une commande — même VAR/PROP, sens inverse | I |
| `Etat` | État réel mesuré ou calculé, non directement commandable | I ou F |
| `Intent` | Capsule d'intention STT envoyée à SP pour validation | I |
| `Event` | Commande ponctuelle — effet immédiat, pas d'état persistant | V |
| `Fusion` | Donnée externe (SE) relayée par SP vers Arduino — ex. `Odom.gyro`, `Odom.compass` | V |
| `param` | Paramètre de configuration — ex. `paraOdom` | V |
| `_` | Pas de qualification particulière | I |

⚠️ **La casse compte** : Table A écrit `Fusion` avec une majuscule. L'ancienne légende de mars écrivait `fusion`. Un code qui chercherait la minuscule ne trouverait rien.

**`NATURE=Event` remplace les parenthèses dans PROP.** `(reset)` devient `reset` + `NATURE=Event`.

**`NATURE=ACK` implique une ligne miroir** : pour chaque `CAT=V` avec retour attendu, il doit exister une ligne `NATURE=ACK` de sens inverse.

---

### 4.13 INTERFACE — visible dans SP

| Valeur | Signification |
|---|---|
| `oui` | Propriété visible et/ou pilotable dans le tableau de bord Node-RED |
| `_` | Propriété interne — pas d'élément d'interface dédié |

Extrait de l'ancien `STATUT=EXPOSEE`, pour deux raisons : ne pas mélanger préoccupations protocolaires et préoccupations d'interface, et permettre qu'une propriété soit à la fois `NATURE=Config` **et** exposée.

Une propriété `INTERFACE=_` reste envoyable manuellement par MQTT — elle n'a simplement pas de contrôle dédié.

---

### 4.14 VPIV_EX — la trame d'exemple

Fournit une trame complète et syntaxiquement correcte. Ce n'est pas redondant avec FORMAT_VALUE et CODAGE : c'est le seul champ qui montre la **syntaxe complète** — le `$`, les quatre `:`, les accolades, le `#` — qu'on ne peut déduire des autres.

```
$V:Gyro:modeGyro:*:DATACont#
$F:Gyro:dataCont:*:[1200,-450,9800]#
$V:Gyro:paraM:*:{"freqCont":20,"freqMoy":2,"nbValPourMoy":5}#
$I:CamSE:active:rear:On#
$E:Urg:source:SE:URG_LOW_BAT#
```

**Obligatoire pour toutes les lignes**, et particulièrement critique pour `TYPE=object`, où une accolade manquante rend le message illisible. La valeur d'exemple doit être **valide** selon FORMAT_VALUE.

---

### 4.15 NOTES — documentation libre

Porte ce qui ne rentre dans aucun autre champ : rôle fonctionnel concret, précautions, dépendances non évidentes, renvois à d'autres documents.

**NOTES ne répète jamais** ce qui a un champ dédié : pas d'unité (CODAGE), pas de valeurs légales (FORMAT_VALUE), pas de trame (VPIV_EX). Langage français, phrases courtes. Si vide : `_`.

---

### 4.16 Règles générales de la Table A

**Une ligne = un message VPIV possible.** Une propriété émise dans deux sens donne deux lignes.

**Pas de flottant dans le VPIV.** Toute décimale est encodée en entier avec un multiplicateur documenté dans CODAGE.

**Les parenthèses dans PROP sont supprimées**, remplacées par `NATURE=Event`.

**`TYPE=object` est réservé à la configuration.**

**Chaque champ vide est remplacé par `_`.** Une cellule vide est ambiguë : oubli ou absence délibérée ?

**La table est normative.** En cas de divergence avec le code, la table a raison sauf erreur démontrée — et toute divergence doit être résolue, jamais ignorée.

---

<a id="5"></a>
## 5. Le protocole VPIV

Cette section remplace `protocoleVPIV.odt` v2.1 du 18/04/2026, dont trois affirmations ont été vérifiées fausses contre le code le 21/10/2026. Les corrections sont consignées en section 7.

### 5.1 Format canonique

```
$CAT:VAR:PROP:INST:VAL#
```

**PROP en position 3, INST en position 4.** Ne jamais inverser.

| Élément | Rôle |
|---|---|
| `$` | Marqueur d'ouverture, obligatoire |
| `CAT` | Comment le message doit être traité |
| `VAR` | Clé de routage — le module concerné |
| `PROP` | Attribut ou action au sein du module |
| `INST` | Sous-élément ciblé |
| `VAL` | Donnée brute — numérique, texte ou JSON compact |
| `#` | Marqueur de fermeture, protège l'intégrité |

### 5.2 Contraintes techniques

**Taille maximale : 128 caractères**, constante `VPIV_MAX_MSG_LEN` définie dans `lib/RZlibrairiesPersoNew/src/config/config.h`.

Entiers préférés aux flottants, pour le coût processeur. JSON compact autorisé entre `{}`. **Aucun retour à la ligne à l'intérieur d'une trame.** Parsing assuré par `vpiv_utils` côté Arduino, par expression régulière ou découpage côté Tasker et Node-RED.

### 5.3 Le cas du mode Flux

`CAT=F` est conçu pour ne pas saturer les processeurs. Pas de poignée de main : l'émetteur envoie, le récepteur traite s'il peut. Utilisé pour des cadences de 20 à 100 messages par seconde. **Node-RED ne répond jamais à un message de type F.**

Note de conception : si un module sature la bande passante, le basculer de `V` vers `F` libère les ressources de traitement.

### 5.4 Format interne à Node-RED

Depuis le 05/07/2026, SP manipule en interne un objet, pas une chaîne :

```javascript
msg.vpiv = { cat, var, prop, inst, val }
```

`msg.origine` vaut `"STT"`, `"UI"`, ou est absent pour le trafic matériel brut. Seul `SF_ValidationIntent_V1` s'y intéresse.

### 5.5 La capsule STT

Une commande vocale voyage encapsulée :

```
$A:STT:intent:<id_requete>:{<id_traitement>:<cat>:<var>:<prop>:<inst>:<val_brute>}#
```

**Cinq champs entre accolades** = déjà résolu par SE, SP laisse passer.
**Six champs** = à résoudre sur SP via le registre `global.RZ_Traitements`.

L'`id_requete` est porté par l'enveloppe **externe**, dans `vpiv.inst`, jamais par le contenu interne.

---

<a id="6"></a>
## 6. Histoire des tables

### 6.1 Table A

| Date | Étape |
|---|---|
| janvier 2026 | `tablevf-Table A old avec commentairePerso` — premières notes |
| mars 2026 | Explosion de versions : `table_A_v3_SE_A`, `table_A_arduino_v4`, `v5`, `table_A_rzV3`, `table_A_SE_v2`. Période de recherche du bon découpage |
| mars 2026 | **v2.0 de la structure** : CAT devient purement protocolaire, NATURE est créé, CODAGE est extrait de FORMAT_VALUE, INTERFACE est extrait de STATUT |
| 27/04/2026 | `TableA-ok.csv` — première version stabilisée |
| 24/06/2026 | `TableA_updated.csv` |
| 25/06/2026 | `TableA_updatedV3.csv` |
| **04/07/2026** | **`TableA_updatedV3_corrige.csv` — référence normative actuelle**, 253 lignes, 24 variables |
| 10/08/2026 | Export `.ods` du même contenu. Vérifié : identique au CSV hors espaces et intitulé de la colonne 1 |

### 6.2 Les classeurs de synthèse

Deux lignées ont coexisté, `TableVarSyntheseV2` et `TableVarSyntheseV21`, ce qui a longtemps brouillé la lecture — un « V2 » modifié en août paraissait antérieur à un « V21 » de juin.

**Tranché le 21/10/2026 par comparaison feuille à feuille.** V2 du 10/08 contient tout ce que contient V21 du 24/06, plus deux feuilles supplémentaires, `TableD_STT_Vdev` et `TableC-vdev`. Les deux seules lignes que V21 possédait en propre — `Appli.IA_conv` en CAT=A et CAT=I — figurent déjà dans la Table A normative sous la graphie `IA_Conv`. **V21 est donc superseded et archivé.**

### 6.3 Table D

| Date | Étape |
|---|---|
| 18/03/2026 | `TableD-CatalogueSTT.csv` — première ébauche |
| 22/04/2026 | `Table_D_STT_Consolidee.odt` |
| 26/05/2026 | `tableD_catalogue` puis `tableD_catalogueV2` |
| 04/07/2026 | `tableD_catalogueV2_corrige.csv` |
| **06/08/2026** | **`tableD_catalogueV3_corrige` — référence actuelle**, 72 commandes |

**Changement de vocabulaire du 06/08/2026**, structurant :

| Ancien | Nouveau | Sens |
|---|---|---|
| `LOCAL` | `LocalSE` | Résolu entièrement sur le téléphone |
| `COMPLEXE` | `MACRO` | Déclenche plusieurs trames filles |
| `PLAN` | `CALCUL` | Extraire une valeur numérique de la phrase |
| `PLAN` | `TRADUCTION` | Convertir un mot en code |

Répartition des 72 commandes : SIMPLE 50 · CALCUL 8 · LocalSE 8 · MACRO 4 · TRADUCTION 2. État : 64 opérationnelles, 8 en attente.

### 6.4 Les légendes

Trois légendes ont coexisté, dont deux identiques au bit près sous des noms différents. **La seule valide est `TableVarSyntheseLegendeTableA-Unifiee.csv` v1.0 du 04/07/2026**, qui fusionne la légende de mars et la V21 en vérifiant chaque écart contre le code réel.

---

<a id="7"></a>
## 7. Erreurs rencontrées — journal

Ce journal n'est pas un palmarès de fautes : c'est un dispositif d'économie. Chacune de ces erreurs a coûté du temps, et plusieurs se seraient répétées sans trace écrite.

### 7.1 🔴 L'ordre PROP / INST inversé dans la légende de mars

**L'erreur.** Les légendes `tableA-Legende.csv` et `TableVarSyntheseLegende.csv`, toutes deux de mars 2026, affirment :

```
FORMAT VPIV ; $CAT:VAR:INST:PROP:VAL# ; ⚠ INST avant PROP — ordre inhabituel mais convention projet
```

La formulation est d'autant plus piégeuse qu'elle présente l'erreur comme une convention assumée.

**La vérité.** `$CAT:VAR:PROP:INST:VAL#`. Vérifié contre `fn_parse_vpiv_v3`, les `dispatch_*.cpp` et la Table A elle-même.

**Corrigé** le 04/07/2026 dans la légende unifiée. Les deux fichiers fautifs ont été archivés le 21/10/2026 — ils étaient restés six mois à côté de la bonne légende.

### 7.2 🔴 La chaîne de la Table D débranchée

**Le symptôme.** 70 commandes sur le Doro, 72 dans la table.

**La cause**, trouvée le 21/10/2026 : `original_init.sh` **régénère** le catalogue depuis une section embarquée dans le script lui-même, sous un nom de fichier V2. Le travail fait en V3 dans le classeur n'a aucun chemin pour atteindre le téléphone.

**Non corrigé à ce jour.** C'est le lot I.

### 7.3 ⚠️ `rz_build_system.py` code le nom en dur

Le script lit `tableD_catalogueV2.csv`, nom figé à la ligne 107. Même chose dans `rz_build_grammar.py` ligne 65. **Régénérer le catalogue sans précaution ferait silencieusement disparaître la colonne `id_traitement`.**

### 7.4 ⚠️ La casse de `Fusion` et de `IA_Conv`

Table A écrit `Fusion` et `IA_Conv` avec une majuscule. L'ancienne légende écrivait `fusion`, et le classeur V21 écrivait `IA_conv`. Un code sensible à la casse ne trouverait rien.

### 7.5 ⚠️ Les ACK du module Odom

Relevé le 04/07/2026 : les ACK du module Odom utilisent la valeur `info` au lieu de `ACK` dans le champ NATURE. **À corriger dans la Table A elle-même**, pas seulement dans la légende.

### 7.6 ⚠️ `paraCam` jugé absent à tort

Le 29/06/2026, une vérification fondée sur le seul FORMAT_VALUE avait conclu que `paraCam` n'existait pas dans la norme. Il était documenté dans COMPLEXE. **Toujours croiser les deux champs.**

### 7.7 ⚠️ Lecture d'un ODS — 65 différences imaginaires

Le 18/10/2026, une comparaison entre le `.ods` du 10 août et le `.csv` du 4 juillet avait annoncé 65 cellules divergentes. **Aucune n'était réelle** : le lecteur ODS concaténait sans séparateur les paragraphes multiples d'une même cellule, et ne développait pas les colonnes répétées.

Vérification refaite le 21/10/2026 avec un lecteur corrigé : **250 lignes identiques sur 254**, les quatre écarts se réduisant à trois doubles espaces et à l'intitulé de la colonne 1.

**La leçon vaut pour tout tableur** : un ODS est une archive compressée, pas un fichier texte. Le lire naïvement produit des différences fantômes.

### 7.8 Trois affirmations fausses dans `protocoleVPIV.odt` v2.1

Vérifiées contre le code le 21/10/2026.

| Le document dit | Le code dit | Vérifié dans |
|---|---|---|
| Taille max **100** caractères | **128** | `config/config.h` ligne 18 |
| `CAT=A` = « debug, logs applicatifs, alertes UI », non traité par Arduino | `CAT=A` = **commande applicative Android**, routée vers `fifo_appli_in` → `rz_appli_manager.sh` pour Zoom, NavGPS, IA_Conv, ExprTasker, tasker | `rz_vpiv_parser.sh`, `rz_start.sh` |
| `[a,b]` = plage d'instances générique, « ex. capteurs 1 à 3 » | N'existe que pour `Lrub`, où il désigne une **liste de pixels** | `dispatch_Lrub.cpp` ligne 41 |

Le document omet par ailleurs le `$` dans sa ligne de format canonique, et précède la capsule STT à six champs de juillet.

**Ce document est remplacé par la section 5 du présent descripteur.**

---

<a id="8"></a>
## 8. Points ouverts

**L'intitulé de la colonne 1** diffère entre les deux formats — `SOURCE` en CSV, `Srce` en ODS. À harmoniser.

**Les ACK du module Odom** portent `info` au lieu de `ACK`. À corriger dans la Table A.

**La chaîne de la Table D** reste rompue tant que le lot I n'est pas fait.

**Huit commandes de Table D** sont en attente : `SYS_SET_PILOT`, `SYS_GET_TEMP`, `NAV_GET_HEADING`, `GYRO_GET_PITCH`, `COM_GET_STATE`, `AI_START_CONV`, `VOICE_VOL_SET`, `VOICE_SAY_ID`.

**`CALC_MTR_DISTANCE`** est volontairement inactif dans le registre des traitements : `Mtr.cmd` attend un triplet vitesse/angle/accélération, où une distance n'entre pas. À trancher avec Odom.

---

*Descripteur des tables RZ — v3.0 — 21/10/2026 — Vincent Philippe / Claude*
*Remplace `table_A_structure_regles.md` v2.0 (mars 2026) et `protocoleVPIV.odt` v2.1 (avril 2026).*
