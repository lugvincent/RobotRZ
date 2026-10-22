# SYNTHÈSE D'ÉTAT — RobotRZ — 22 octobre 2026

**Fichier** : `docs/synthese_etat.md` (dépôt Git) — copie archivée de `claude/synthese_etat_oct2026.md` (Project Knowledge)
**Objectif** : donner à toute conversation du projet l'état réel au 22/10/2026, et signaler les points où `instructions_projet_RobotRZ.md` (v1.3, juillet) est périmé.
**Articulation** : complète `instructions_projet_RobotRZ.md`. **En cas de divergence, ce fichier fait foi pour l'état d'octobre 2026.** Le dossier technique complet avec schémas est dans `docs/dev_RZ_HTML/dossier_technique_V1.html`, accessible depuis `index.html`.
**Précaution** : document d'état, pas de référence normative. Les références normatives restent Table A (`TableA_updatedV3_corrige.csv`) et Table D (v3).
**Version** : 3.4 — 22/10/2026 — ajout du §9 (chantier documentation et sécurité des 20-22/10), correction de la Table C, du nom réel de la Table D, et des pièges associés. 3.3 — 19/10/2026

---

## 0. Où vit ce document, et laquelle des deux copies fait foi

Ce document existe à **deux endroits**, avec des rôles distincts. Ne pas les confondre.

| Emplacement | Rôle | Mise à jour |
|---|---|---|
| **Project Knowledge** — `claude/synthese_etat_oct2026.md` | **Copie vivante.** C'est celle que lisent et écrivent les conversations du projet. Toujours la plus à jour. | À chaque séance, réécrite intégralement |
| **Dépôt Git** — `docs/synthese_etat.md` | **Archive datée.** Donne l'historique (`git log`), une copie hors ligne sur le PC, et une sauvegarde si le projet était perdu. | Commit à chaque changement de numéro de version |

**Règle** : en cas d'écart entre les deux, **la copie du Project Knowledge fait foi** — c'est la seule que les conversations mettent à jour. Git peut avoir une version de retard ; c'est normal et sans gravité, le numéro de version en tête permet de le vérifier d'un coup d'œil.

**Pourquoi pas de version dans le nom du fichier** : le chemin doit rester stable pour que les conversations le retrouvent, et pour éviter des copies concurrentes dont on ne saurait plus laquelle fait foi. La version vit dans l'en-tête. Même règle que pour `node-red/functions/`.

---

## 1. À lire en premier si vous reprenez Node-RED

1. **Le `flows.json` est désormais dans le dépôt** : `node-red/flows/flows-RPI-2026-10-18.json`. Lire le `README.md` du même dossier avant toute chose.
2. **Exporter les flux en fin de chaque séance Node-RED, sans exception.** Node-RED ne versionne rien. Procédure dans le README.
3. **Ne pas se fier aux versions de scripts de `instructions_projet_RobotRZ.md`** — trois sont périmées, voir §3.1.

---

## 2. Sources de vérité et fraîcheur

| Source | État | Fraîcheur |
|---|---|---|
| Dépôt Git `lugvincent/RobotRZ` (public) | Lisible par Claude **sans jeton** | **22/10/2026** |
| `docs/tables_RZ_descripteur.md` | **Référence normative des quatre tables.** Remplace `table_A_structure_regles` et `protocoleVPIV.odt` | **21/10/2026** |
| `docs/installation_RPI_serveur.md` | Procédure d'installation du RPi serveur | **22/10/2026** |
| `TableC.json` | **Table C maître — n'est versionnée nulle part**, voir §5.1 | 18/04/2026 |
| `node-red/flows/flows-RPI-2026-10-18.json` | Référence courante, 298 nœuds | **18/10/2026** |
| `node-red/flows/flows-XIAOMI-2026-10-18.json` | Archive, 6 onglets uniques | 18/10/2026 |
| `node-red/functions/` | Copie lisible des nœuds function | 18/10/2026 |
| `instructions_projet_RobotRZ.md` | v1.3 | juillet 2026, **erreurs connues, §3** |
| Scripts SE dans Git | `rz_stt_handler.sh` **v3.2 non déployé** | 18/10/2026 |
| `docs/dev_RZ_HTML/` | 17 pages + dossier technique | 18/10/2026 |

**Accès Git** : le dépôt est public, Claude le lit via le proxy de session, aucun jeton requis. Le `GITHUB_TOKEN` du Doro sert **uniquement à pousser depuis le téléphone**. À renouveler via `set_git_remote.sh`.

### Règles de sécurité — sans exception

- **RÈGLE ABSOLUE : ne jamais coller le résultat de `git remote -v` dans une conversation → token Doro visible en clair dans l'URL. Toujours passer par `set_git_remote.sh`.**
- **JAMAIS coller une clé API en clair dans une conversation ou un log partagé.**
- **Charger les credentials dans une variable shell sans jamais les afficher, vérifier par la longueur (`${#TOKEN}`), jamais par la valeur affichée.**
- `AA-Doc-horsGit/` (ex `RobotRZ-horsIO/`, renommé le 20/10) et `InstallationServeur/` sont exclus du dépôt (`.gitignore` v2.2) — ne jamais lever cette exclusion, `Id-code-clef.txt` s'y trouve. **Si ce dossier est renommé à nouveau, ajouter le nouveau nom au `.gitignore` dans le même geste** : un `.gitignore` ne s'applique qu'aux fichiers que Git ne suit pas déjà.
- **Un seul fichier porte les identifiants** : `AA-Doc-horsGit\Z - Secret\RZ_Codes_Identifiants.odt`, créé le 22/10, à chiffrer dans LibreOffice (*Enregistrer avec mot de passe*, AES-256). Toute valeur recopiée dedans est supprimée de son emplacement d'origine.
- **Ne jamais publier une adresse IPv6 globale.** Contrairement à l'IPv4, elle désigne la machine elle-même et ne passe par aucun NAT. L'adresse MAC la trahit également : l'identifiant d'interface en dérive (EUI-64).

---

## 3. Divergences avec `instructions_projet_RobotRZ.md` v1.3

### 3.1 Versions de scripts SE — le doc est en retard

| Script | Doc v1.3 | Git au 18/10/2026 |
|---|---|---|
| `rz_vpiv_parser.sh` | v2.3 | **v2.5** |
| `rz_state-manager.sh` | v1.6 | **v1.9** |
| `rz_appli_manager.sh` | v1.2 | **v1.3** |
| `rz_stt_handler.sh` | — | **v3.2 — dans Git, PAS sur le Doro** |

v2.5 et v1.9 portent tous deux sur la **connexion MQTT persistante**.

### 3.2 SF_ReglesDynamiques — existe, mais squelette et non câblé

Le subflow `sf_regles_dynamiques_v1` existe (1 entrée / 2 sorties), `R0_LoadRules` est écrit, mais **`R1_ApplyRules_STUB` est un squelette** et le nœud posé dans `Gtion_EtatGlobal_V2` n'est relié à rien. Le Xiaomi porte un **moteur de règles complet** (onglet `ui_Rules`, 31 nœuds) qui est la matière de ce chantier.

### 3.3 Project Knowledge — doublons avec le dépôt

12 documents sont des copies de fichiers du dépôt, dont 6 proviennent de `tasker/archive/` (donc archivés comme obsolètes par le dépôt lui-même). À conserver car absents du dépôt : `instructions_projet_RobotRZ.md`, `Legende-TableA-TableD.md`, les deux CSV.

---

## 4. Node-RED — état au 18/10/2026

### 4.1 Onglets (7)

| Onglet | Nœuds | État |
|---|---|---|
| `Gtion_Init` | 18 + registre | SF_Init_1 réel, **SF_Init_2/3/4 = STUB** |
| `ui_Accueil_v4` | 25 | Reconstruit le 06/08/2026 |
| `Gtion_EtatGlobal_V2` | 31 | Chaîne décisionnelle |
| `RZ_GestionFlux_V3` | 37 | Bus MQTT, opérationnel |
| `ui_VarG` | 50 | Éditeur `enrichedVars`, fonctionnel |
| `Diag_SE_Statut` | 42 | Opérationnel |
| `B2_Variables_Complexes` | 15 | Bac à sable, isolé |

### 4.2 Chaîne décisionnelle réelle

```
Evnt_Raw_In  ←  3 producteurs : Gtion_Init, RZ_GestionFlux_V3, ui_Accueil_v4
   └→ SF_Dispatcher_EtatGlobal
        └→ SF_GestionUrgence
             ├ sortie 0 (URGENCE) → EtatGlobal_Out        [direct, assumé]
             └ sortie 1 (NORMAL)  → SF_ReglesMinimales_V1
                                        ├ sortie 0 → SF_ValidationIntent_V1
                                        │               ├ s0 → SF_Stabilisation_V1 → EtatGlobal_Out
                                        │               └ s1 → SF_EmissionACK
                                        ├ sortie 1 → link out « reset » → SF_Init_1
                                        └ sortie 2 → SF_EmissionACK (msg.ack / FAIL)

EtatGlobal_Out → ui_Accueil_v4 (EtatGlobal_IN) + RZ_GestionFlux_V3 (CMD_FROM_BRAIN)
```

### 4.3 Subflows — inventaire et état

| Subflow | E/S | État |
|---|---|---|
| `SF_Dispatcher_EtatGlobal` | 1/1 | ✅ testé |
| `SF_GestionUrgence` | 1/2 | ✅ **câblage corrigé le 17/10/2026** |
| `SF_ReglesMinimales_V1` | 1/3 | ✅ testé en réel |
| `SF_ValidationIntent_V1` | 1/2 | ✅ testé |
| `SF_Stabilisation_V1` | 1/1 | 🔴 **contient le code de `fn_validate_intent`** |
| `SF_ReglesDynamiques` | 1/2 | 🟣 R1 = STUB, **non câblé** |
| `SF_EmissionACK` | 1/0 | ✅ opérationnel |
| `Sub-Connexion` | 1/1 | ⚠️ 22 nœuds, **décision en attente** : doublon de SF_Init_2 ou surcharge manuelle ? |
| `GererTransitionFlowPage` | 1/2 | non évalué |
| ~~`OldSub-Arrets`~~ | — | supprimé le 18/10/2026 |

### 4.4 ✅ Bug 1 — CORRIGÉ le 17/10/2026

Nœud `d39c7692c09fd2f5`, instance de `SF_GestionUrgence`. **Était** : sortie 0 (urgence) vers un simple debug, donc les urgences ne sortaient pas ; sortie 1 vers `SF_ReglesMinimales_V1` **et** `EtatGlobal_Out`, ce qui court-circuitait les trois derniers étages. **Corrigé** : sortie 0 → `EtatGlobal_Out` ; sortie 1 → `SF_ReglesMinimales_V1` seul.

Note : une urgence n'émet pas d'ACK. Cohérent, mais à confirmer si un accusé est souhaité.

### 4.5 ✅ Bug 2 — CLOS, résolu autrement

`UI_Sync_Etat_Confirme` a été réécrit le 06/08/2026 : il lit `global.RZ_CfgS` au lieu de déduire « confirmé » de l'arrivée d'un message. Le correctif v1.1 proposé en août est sans objet.

### 4.6 🔴 fn_calc_statelevel — confirmé cassé

La fonction « Calcul stateLevel » de `SF_Stabilisation_V1` contient le code de `fn_validate_intent`. Le dernier étage ne calcule rien. **Ce n'est pas un copier-coller à défaire** : la question « que doit valoir `stateLevel` et qui le consomme » n'a jamais été tranchée.

### 4.7 ✅ Chaîne vocale — registre des traitements, 18/10/2026

Découverte du 18/10 : toute la plomberie de la commande vocale existait déjà (SE v3.1, parser v2.5, routeur v1.3). Il ne manquait que la table de conversion. Implémenté et **validé en réel**.

- **`fn_init_traitements`** (nouveau, onglet `Gtion_Init`) charge `global.RZ_Traitements`, un registre **déclaratif** de 8 recettes dont 7 actives. Trois types d'interprétation : `table` (mot → code), `nombre` (bornes + défaut + gabarit), `table_ou_nombre`. Contrôle de cohérence au chargement, témoin vert/rouge sur le nœud.
- **`fn_stt_intent_router` v1.4** : la liste de couleurs en dur a disparu, remplacée par un moteur générique qui lit le registre. 21 cas de test passés.
- **`rz_stt_handler.sh` v3.2** : `MTR_TURN_LEFT` signe l'angle en négatif côté SE. **Sans angle capté, la commande est refusée** avec « De combien de degrés ? » — le défaut de SP étant positif, il ferait tourner à droite. ⚠️ **Non déployé sur le Doro** : faire `git pull` au prochain démarrage.
- **`CALC_MTR_DISTANCE` est volontairement inactif** : `Mtr.cmd` attend un triplet vitesse/angle/accélération où une distance n'entre pas. À trancher avec Odom avant activation.

### 4.8 Nettoyage du canevas — 18/10/2026

13 nœuds morts et le subflow vide `OldSub-Arrets` supprimés après vérification. Faux positifs écartés de l'analyse : un nœud function seul dans un subflow (les ports sont déclarés dans `in`/`out`, pas par `wires`), les nœuds d'interface sans fil, et le placeholder délibéré de `SF_ReglesDynamiques`.

### 4.9 Le Xiaomi n'est pas un clone

Six onglets n'existent **nulle part ailleurs**. Ils sont conservés dans `node-red/flows/flows-XIAOMI-2026-10-18.json`, qui est une **archive à ne jamais supprimer** tant que §4.10 n'est pas soldé.

| Onglet | Nœuds | Contenu |
|---|---|---|
| `ui_Rules` | 31 | Moteur d'exécution de règles complet + modèle de données (règle = activation + conditions + actions) + 3 notes de conception |
| `travail_joystickAccelerationEncodageVPIV` | 8 | Prototype de la page pilotage : nœud joystick, curseur d'accélération, conversion vers vitesse/rotation, retour odométrie, cadre vidéo |
| `demarrage liaison SE-SP` | 5 | Poignée de main `SE_READY` puis envoi de configuration |
| `travail_Calibrage Odometrie` | 6 | Paramètres mesurés réels : diamètre 0,100 m, voie 0,300 m, 256 points par tour |
| `ui_VarRegles` | 48 | Interface de règles concurrente, écartée par décision. Import/export JSON |
| `ui_Ini_Pilotage` | 19 | Page d'options de pilotage, génération de texte dynamique selon mode et périphérique |

### 4.10 Plan de rapatriement du Xiaomi

Rien de tout cela n'est un lot en soi : **chaque élément est une matière première qui entre dans un lot existant**. Le tableau ci-dessous est la réponse à la question « quand est-ce que ça revient ».

| Onglet Xiaomi | Destination sur le RPi | Lot | Conversion Dashboard ? |
|---|---|---|---|
| `ui_Rules` | `SF_ReglesDynamiques` — le moteur est déjà écrit, il sert de base à `R1_ApplyRules` | **C** | oui, partiellement |
| `demarrage liaison SE-SP` | `SF_Init_4` — la poignée de main `SE_READY` + envoi de configuration est exactement ce que le STUB doit faire | **D** | non |
| `travail_Calibrage Odometrie` | Ce sont des **données**, pas des flux : les trois paramètres mesurés entrent dans `enrichedVars` via `ui_VarG`. Ils conditionnent aussi l'activation de `CALC_MTR_DISTANCE` | **F**, puis §7 | non |
| `travail_joystickAccelerationEncodageVPIV` | Page pilotage, à créer — n'existe pas sur le RPi | **K** | oui, lourde |
| `ui_Ini_Pilotage` | Même page pilotage | **K** | oui |
| `ui_VarRegles` | **Aucune.** Interface concurrente écartée par décision ; conservée comme référence de conception uniquement | — | — |

**Obstacle commun, à ne pas découvrir en cours de route.** Le Xiaomi est en **Dashboard 1** (types de nœuds en `ui_xxx`, tiret bas), le RPi en **Dashboard 2 / FlowFuse** (`ui-xxx`, tiret). Tout onglet contenant des nœuds d'interface impose une conversion manuelle, nœud par nœud. Les nœuds de logique — `function`, `switch`, `change`, `link` — se transposent sans rien changer. C'est pourquoi `demarrage liaison SE-SP` et `travail_Calibrage Odometrie` sont les deux rapatriements faciles, et pourquoi le lot K est le plus lourd.

⚠️ **Le nœud joystick n'a pas d'équivalent direct connu en Dashboard 2.** À vérifier avant d'ouvrir le lot K : soit un widget tiers existe, soit il faudra le reconstruire en `ui-template`. Ne pas supposer que le portage sera mécanique.

---

## 5. Invariants — à ne jamais enfreindre

### 5.1 Autorité des tables — règle du 18/10/2026

> **Le PC est l'atelier de conception** : la Table A vit dans LibreOffice Calc, en tête de chaîne, et n'est **jamais chargée à l'exécution**.
> **Le RPi porte l'autorité d'exécution** pour les tables B, C et D. Les modifications d'administration se font sur cette source.
> **Chaque sous-système garde une copie locale persistante** lui permettant de démarrer seul ; SP l'écrase après `SE_READY`. L'autorité n'est pas la copie unique — sans cache local, les 8 commandes `LocalSE` mourraient avec la liaison.
> **Le Xiaomi reçoit un clone daté**, à jour **avant** toute panne du RPi, jamais pendant : au moment du basculement, la source n'est plus là.

Cette règle était déjà à moitié écrite : l'en-tête de `courant_init.sh` dit « *en production, ce fichier JSON est reçu directement depuis SP via MQTT* ».

**Précision du 22/10/2026 — la Table C n'est pas où on la croyait.** Plusieurs documents, dont `architecture_v2.md`, la décrivaient comme résidant sur SE sous le nom `courant_init.json`. C'est faux, et la chaîne réelle est :

```
TableC.json        maître, sur SP, 23 variables, 80 propriétés, modèle enrichedVars
                   éditée par l'onglet ui_VarG
      ↓
globalSave.json    Table B, node-red/config/ — générée depuis la Table C
      ↓
courant_init.json  projection envoyée à SE à l'initialisation
```

`courant_init.json` **n'est pas la Table C**, c'en est la projection. `doc_A1` et `doc_A2` (mars 2026) donnaient déjà la bonne version ; `architecture_v2.md` (28/04) était la source de l'erreur, et il est désormais dans `old\`.

> 🔴 **`TableC.json` n'est versionné nulle part.** Il n'existe que dans `AA-Doc-horsGit\A - Architecture_Tables_Schemas\Tables\`. La table maître du projet n'a donc aucune sauvegarde ni historique. À verser au dépôt.

**Conséquence à traiter** : la Table D n'a aujourd'hui **aucune existence sur le RPi**. Sa vraie source génératrice est une section de `original_init.sh`, un script d'installation qui tourne **sur le Doro** (balise `=== SECTION CATALOGUE STT ===`). Le CSV du projet en est une copie corrigée. C'est très probablement la cause du décalage **70 commandes déployées contre 72 annoncées** (`APP_IA_CONV_ON/OFF` ne sont jamais arrivées). Premier geste : extraire la Table D de `original_init.sh` vers un fichier autonome hébergé sur le RPi, avec un numéro de version pour que SE ne reconstruise ses index que si la version change.

### 5.2 Protocole et exécution

- **VPIV** : `$CAT:VAR:PROP:INST:VAL#`. CAT = V (consigne) · I (info/ACK) · E (urgent) · F (flux) · A (app Android). **PROP en position 3, INST en position 4.**
- **CamSE = CAT=V/F**, jamais CAT=A, malgré la délégation interne à Tasker.
- **Format interne Node-RED** : `msg.vpiv = {cat, var, prop, inst, val}` depuis le 05/07/2026. `msg.origine` vaut `"STT"`, `"UI"` ou est absent.
- **`global.RZ_<VAR>`** est la seule source de vérité à l'exécution. `enrichedVars` sert **uniquement** à l'amorçage, modifiable **uniquement via `ui_VarG`**.
- **Lire `_last` avant `'*'`** pour toute propriété à instance générique : `p._last !== undefined ? p._last : p['*']`. L'inverse lit une valeur figée à l'amorçage. Piège rencontré trois fois.
- **Les nœuds `link` ne traversent pas une frontière de subflow.** Exposer un port sur le contour.
- **SP est maître absolu.** Un seul SP actif : RPi 3 (référence) ou Xiaomi (SP Smart, secours).
- **LockSE** : SE fait autorité sur caméra et micro. Tout-ou-rien.
- **STT Option B** : l'arbitrage Cmde/Conv/Mixte est décidé entièrement côté SE dans `rz_autovoice_bridge.sh`. Ne jamais le dupliquer côté Node-RED.
- **Capsule STT** : `$A:STT:intent:<id_requete>:{<id_traitement>:<cat>:<var>:<prop>:<inst>:<val_brute>}#`. **5 champs = déjà résolu par SE** (on passe) ; **6 champs = à résoudre sur SP** via le registre. L'`id_requete` est porté par l'enveloppe **externe** (`vpiv.inst`), jamais par le contenu.
- **SF_ReglesDynamiques** : jamais avant `SF_ReglesMinimales_V1`, jamais relié directement à l'UI, jamais la possibilité de bloquer le retour en VEILLE.
- **Table A** : toujours croiser `FORMAT_VALUE` **et** `COMPLEXE`.
- **Jamais de flottant sur le fil** : entiers ×10, ×100 ou ×1000 selon la propriété.
- **Aucun manager SE n'appelle `mosquitto_pub` directement** — tout sort par `fifo_termux_in`.
- **Domaine** : `robotz-vincent.duckdns.org`, le Z est volontaire, ne pas « corriger ».
- **NAT hairpin** : la Livebox ne le supporte pas. Ne jamais tester l'IP publique depuis le même LAN. Et l'IPv6 masque les pannes IPv4 : tester en 4G.
- **`globalSave.json`** : deux fichiers distincts — PC `node-red/config/globalSave.json`, RPi `/home/rzServeur/robotRZ/data/globalSave.json`.
- **`flows.json` fait foi** sur `node-red/functions/`. On modifie dans l'éditeur, on déploie, on teste, **puis** on recopie dans Git. Jamais l'inverse.
- **Dashboard 1 et Dashboard 2 ne sont pas compatibles.** Tout import venant du Xiaomi contenant des nœuds d'interface impose une conversion manuelle. Voir §4.10.

### 5.3 Vocabulaire Table D v3 (06/08/2026)

`LOCAL` → **`LocalSE`** · `COMPLEXE` → **`MACRO`** · `PLAN` → **`CALCUL`** (extraire une valeur) ou **`TRADUCTION`** (mot → code).

Répartition des 72 commandes : SIMPLE 50 · CALCUL 8 · LocalSE 8 · MACRO 4 · TRADUCTION 2. État : 64 ✅ / 8 ⛔.

---

## 6. Reste à faire — lots

| Lot | Objet | Apport du Xiaomi | Taille |
|---|---|---|---|
| ~~**A**~~ | ~~Nettoyage du canevas~~ | — | ✅ fait le 18/10/2026 |
| **B** | `fn_calc_statelevel` — décider ce qu'est `stateLevel`, puis l'écrire | — | 1 séance de conception + 1 d'implémentation |
| **C** | `SF_ReglesDynamiques` — écrire R1, câbler | **`ui_Rules`** : moteur complet à reprendre | 2 séances |
| **D** | `Gtion_Init` — SF_Init_2 (IP), SF_Init_3 (contexte INIT), SF_Init_4 (init VPIV Arduino, 9 étapes) | **`demarrage liaison SE-SP`** pour SF_Init_4 | 3 séances min. |
| **E** | `Sub-Connexion` — trancher : doublon de SF_Init_2, ou surcharge manuelle à conserver ? | — | 1 séance |
| **F** | Report Tables B/C via `ui_VarG` : Gyro, Mag, MicroSE, Voice, LockSE, STT. Corriger `enrichedVars.Lrub` (`rgb` → `col`) **via ui_VarG uniquement** | **`travail_Calibrage Odometrie`** : 3 paramètres mesurés à saisir | 1-2 séances, interruptible |
| **G** | Modules `M_Capt_` / `M_Act_` côté SP — 10 VAR Arduino + 4 VAR SE restants | — | **voir §6.1 — interruptible, pas un bloc** |
| **H** | Table D, 8 lignes ⛔ : `SYS_SET_PILOT`, `SYS_GET_TEMP`, `NAV_GET_HEADING`, `GYRO_GET_PITCH`, `COM_GET_STATE`, `AI_START_CONV`, `VOICE_VOL_SET`, `VOICE_SAY_ID` | — | ~30 min chacune |
| **I** | **Table D sur le RPi** — l'extraire de `original_init.sh`, la versionner, la distribuer vers SE à l'init. Prérequis des MACRO atomiques | — | 1-2 séances |
| **J** | **MACRO** (4 commandes) — boucle provisoire côté SE, ou mécanisme atomique côté SP après le lot I | — | voir §7 |
| **K** | **Page pilotage** — n'existe pas sur le RPi. Joystick, curseur d'accélération, encodage VPIV, retour odométrie, cadre vidéo, options selon `CfgS.typePtge` | **`travail_joystick…`** + **`ui_Ini_Pilotage`** : prototypes complets, mais en Dashboard 1 | 3 séances min., dont une d'étude du portage joystick |

**Ordre conseillé sur un temps court** : E (seul inconnu, conditionne D), puis B (ferme la chaîne). H et G sont les deux lots interruptibles, à garder pour les créneaux courts.

### 6.1 Lot G — réévaluation du 19/10/2026

**Correction d'une estimation fausse.** Une version antérieure de ce document annonçait « plusieurs mois » et « 13 Arduino + 4 SE, aucun n'existe ». Les deux sont inexacts.

**Ce qui existe déjà.** `M_Capt_Sys`, `M_Capt_Gyro` et `M_Capt_Mag` sont **opérationnels**, intégrés dans `Gtion_Statut SE`. Restent 10 VAR Arduino (`Mtr`, `US`, `SecMvt`, `Odom`, `Srv`, `Lring`, `Lrub`, `Mic`, `FS`, `IRmvt`) et 4 VAR SE (`CamSE`, `MicroSE`, `Appli` dont IA_Conv, `STT`).

Surtout : **le firmware Arduino et les managers SE existent déjà**, et la chaîne générique de SP — parse, dispatch, règles, validation, émission — traite n'importe quelle variable sans distinction. Un module ne reconstruit rien de tout cela ; il ajoute la supervision et la commande propres à une variable.

**Ce qui n'est pas tranché, et qui décide du volume.** `roadmap_nodered_V1.html` pose explicitement la question, sans y répondre : *« Un flow Node-RED par VAR (13 pour Arduino) ou des flows groupés par catégorie, ex. un seul flow Capteurs Arduino couvrant Mic/US/FS/IRmvt ? Impacte directement le volume de travail. »* L'estimation « plusieurs mois » retenait implicitement l'hypothèse la plus coûteuse. Un flow par VAR donne 14 unités ; un regroupement par famille en donne 5 ou 6.

**Ce qui coûte réellement du temps**, et ce n'est pas Node-RED : la **validation physique**. Vérifier 9 ultrasons, calibrer l'odométrie sur un vrai sol, faire tourner des moteurs. Cela demande le robot alimenté et l'Arduino connecté, séance après séance. C'est du temps de calendrier, pas de l'effort de développement — la nuance change tout quand on travaille par créneaux courts.

**Lecture honnête du lot.** Une quinzaine de petits travaux semblables. Le premier coûte plus cher que les autres parce qu'il fixe le gabarit ; les suivants sont des soirées. La roadmap précise d'ailleurs que **les modules sont indépendants entre eux**. C'est donc l'un des lots les mieux adaptés à un travail fractionné — l'inverse d'un mur.

**Ordre suggéré par la roadmap** : `Mtr` et `US` en premier, parce qu'ils conditionnent les garde-fous, puis `SecMvt`/`Odom`, puis `Srv`/`Lring`/`Lrub`, puis les VAR SE.

### Pièges à ne pas réveiller

- **`rz_build_system.py` hardcode `tableD_catalogueV2.csv`.** Le régénérer tel quel ferait silencieusement disparaître la colonne `id_traitement`. Le nom est codé en dur à **trois** endroits — `original_init.sh` (641), `rz_build_system.py` (107), `rz_build_grammar.py` (65) — à corriger ensemble.
- **`table_d_catalogue.csv` n'existe pas.** Ce quatrième nom de la Table D est né dans `doc_A2` (mars), a été repris par `architecture_v2.md`, et avait atteint la page publiée `tables_donnees_V1.html`. Corrigé le 21/10. Si vous le rencontrez encore quelque part, c'est une erreur.
- **Un `.gitignore` ne protège que ce que Git ne suit pas déjà.** `.env` y figurait depuis février alors qu'il était suivi depuis le commit `7e4ba5c` : la ligne n'avait aucun effet et donnait une fausse assurance. Vérifié le 20/10, il ne contient que trois chemins, aucun secret ; la ligne trompeuse a été retirée et remplacée par une explication.
- **Un `.odt` ou un `.ods` est une archive ZIP.** `grep` et `Select-String` n'y voient rien — ni les mots de passe, ni le contenu. Une recherche de secrets qui ignore ces formats ne trouve rien là où il y en a le plus. `H - Outils\RZ_Chercher_Secrets.ps1` les ouvre.
- **Lire un `.ods` naïvement produit des différences fantômes.** Le 18/10, une comparaison hâtive a fait croire à 65 écarts entre la Table A `.ods` et son `.csv` : les paragraphes multiples d'une cellule étaient concaténés sans séparateur et les `number-columns-repeated` non développés. Revérifié le 21/10 avec un lecteur correct : **250 lignes sur 254 strictement identiques**, les quatre écarts étant trois doubles espaces et l'en-tête (`Srce` contre `SOURCE`).
- **`cfg_urg_messages[]` est désynchronisé de l'énumération `UrgReason`.** Le tableau de `config.cpp` (191) compte 8 entrées avec un `"OVERHEAT"` surnuméraire en position 1 ; l'énumération de `urg.h` en compte 10 et n'a pas d'`URG_OVERHEAT`. Indexer l'un par l'autre donne des libellés décalés d'un cran et **deux lectures hors limites**. Ce n'est pas un bug aujourd'hui — le tableau n'est lu nulle part, la fonction réellement utilisée est `urg_reason_str()` dans `urg.cpp`, qui est correcte. Mais les deux symboles sont déclarés à trois lignes l'un de l'autre dans `config.h` : à supprimer ou à aligner avant que quelqu'un ne les associe.
- **Tester l'accès distant depuis le réseau local ne prouve rien** (NAT hairpin). Voir `docs/installation_RPI_serveur.md` §6.
- **Sur Raspberry Pi OS récent, `/etc/dhcpcd.conf` n'est plus lu** et `dhcpcd.service` n'existe pas. L'éditer ne produit ni effet ni message d'erreur. Le réseau est géré par NetworkManager (`nmtui`).
- **Le catalogue déployé compte 70 commandes, pas 72.** Voir §5.1.
- **Ne jamais supprimer `flows-XIAOMI-2026-10-18.json`** tant que le tableau §4.10 n'est pas entièrement soldé. C'est la seule copie de six onglets.
- **Norton contre Git, sur le PC Windows (18/10/2026).** Deux pannes distinctes du même antivirus, à reconnaître vite si ça recommence.
  - *Symptôme 1* : toute commande Git répond `BUG (fork bomb): C:\Program Files\Git\bin\git.exe`. Norton avait supprimé le vrai binaire ; `bin\` et `cmd\` ne contiennent que des lanceurs, qui finissent par se trouver eux-mêmes. Diagnostic : `Get-ChildItem "C:\Program Files\Git" -Recurse -Filter git.exe` — s'il ne renvoie que ces deux chemins, le binaire manque. Remède : réinstaller Git depuis `git-scm.com`.
  - *Symptôme 2* : `git pull` échoue sur `unable to write file .git/objects/… : Permission denied`. C'est la **Protection des données** de Norton, qui interdit aux programmes inconnus d'écrire dans Documents. Remède : Norton → Paramètres → **Autorisations des applications** → ajouter les exécutables de Git et leur donner l'autorisation « dossiers protégés » (icône en forme de dossier).
  - **Depuis Git 2.56, le vrai binaire est dans `ucrt64\`, plus dans `mingw64\`** : `C:\Program Files\Git\ucrt64\bin\git.exe` et `C:\Program Files\Git\ucrt64\libexec\git-core\git.exe`. C'est le second qui écrit pendant un `pull`.
  - **Troisième incident, 22/10/2026 — PlatformIO cette fois.** L'extension se réinstalle, puis
    l'extension C/C++ signale `Impossible de résoudre la configuration avec le compilerPath
    "…/toolchain-atmelavr/bin/avr-gcc.exe"`. Le paquet `toolchain-atmelavr` est bien présent et son
    arborescence intacte — seuls certains exécutables manquent. **La signature est caractéristique** :
    tout ce qui FABRIQUE un exécutable avait disparu (`avr-gcc`, `avr-g++`, `avr-as`, `avr-ld`,
    `avr-ar`, `avr-objcopy`, `avr-objdump`, `avr-nm`, `avr-ranlib`, `avr-strip`, `avr-readelf`),
    tout ce qui se contente de LIRE était resté (`avr-gdb`, `avr-gcov`, `avr-gprof`, `avr-size`,
    `avr-strings`, `avr-addr2line`, `avr-c++filt`, `avr-cpp`). Détail qui achève la démonstration :
    `avr-gcc-ar.exe` survit, `avr-ar.exe` non. C'est une sélection heuristique, pas un téléchargement
    incomplet. Remède : exclure `C:\Users\<profil>\.platformio` dans Norton (Antivirus → Analyses et
    risques → Éléments à exclure, **et** exclusions SONAR), PUIS réinstaller le paquet —
    `pio pkg uninstall --global --tool "platformio/toolchain-atmelavr"` puis `pio pkg install` idem.
    Sans l'exclusion posée d'abord, le retrait se reproduit.
  - **`pio` n'est pas dans le PATH de Windows.** L'exécutable est dans
    `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`. Poser un alias pour la session :
    `Set-Alias pio "<ce chemin>"`. Et `pio run` doit connaître le projet : depuis un autre dossier,
    utiliser `pio run -d "<chemin du projet>" -e megaatmega2560` plutôt que de se déplacer.
  - Réglages retenus à la réinstallation : fins de ligne **« Checkout as-is, commit Unix-style »** — indispensable, les scripts `.sh` tournent sous Linux et bash refuse les fins de ligne Windows.

**Différé** : branche PLAN de `rz_stt_handler.sh` · caméra en modes 2 et 3 · pages HTML `sp_nodeRED_V1`, `schema_sp_flows_V1`, `roadmap_nodered_V1` (marquées « Page historique », mais leur **contenu de fond reste valable** — c'est leur état d'avancement qui est périmé) · ROS 2 sur RPi 4 · MQTT TLS · erreur IntelliSense `avr-gcc.exe` introuvable (extension C/C++ et PlatformIO, sans effet sur la compilation).

---

## 7. MACRO — décision en attente

Quatre commandes de type MACRO déclenchent plusieurs trames filles. L'ancienne boucle côté SE fonctionnait et **ne perdait rien** : `process_simple` envoyait chaque fille en capsule 5 champs par MQTT, donc à travers toute la chaîne de validation de SP.

Le vrai gain de l'option SP n'est donc pas la validation, c'est **l'atomicité** (décision de cadrage n°3 : une MACRO réussit ou échoue en bloc). Elle suppose trois choses : la Table D sur SP (**lot I**), une émission multi-messages depuis le routeur, et une extraction par règle pure.

Vincent penche pour l'option SP. Deux chemins possibles : rétablir la boucle SE comme palliatif clairement étiqueté, ou attaquer directement le lot I maintenant que l'autorité des tables est tranchée.

---

## 8. Connectivité SE ↔ SP

| Mode | Réseau SE | SP actif | `CfgS.typeReseau` |
|---|---|---|---|
| 1 | Wi-Fi maison | SP RPI `192.168.1.40` direct | `WifiInternetBox` |
| 2 | Wi-Fi d'un tiers | SP RPI via DuckDNS | `WifiInternetBox` |
| 3 | Hotspot Xiaomi + 4G | SP RPI via DuckDNS | `Wifi4KSP` |
| 4 | Hotspot Xiaomi sans 4G | SP Smart, local | `WifiSPSsI` |

---

## 9. Chantier documentation et sécurité — 20 au 22 octobre 2026

Trois jours consacrés à remettre en ordre la documentation et à retirer ce qui n'aurait jamais dû être publié. Ce chantier **n'est pas le cœur du projet** et ne doit pas le rester : ce qui en reste ouvert est listé en fin de section, avec sa destination.

### 9.1 Ce qui est clos

**Le dossier documentaire local est rangé.** 1 752 fichiers et 215 Mo ramenés à 567 fichiers et 74 Mo de documentation réelle. La cause du désordre était structurelle : deux axes de classement concurrents, le temporel dupliquant le thématique à l'intérieur de ses propres sous-dossiers. Règle retenue, écrite dans `AA-Doc-horsGit\LISEZ-MOI.md` et dans `docs/dev_RZ_HTML/vademecum_documentation_V1.html` : **le thème décide du dossier, l'âge décide de la place à l'intérieur**. Une version dépassée descend dans le `old\` de sa propre lettre, elle ne part pas ailleurs.

**Le dossier A est trié et renommé** `A - Architecture_Tables_Schemas`, avec `Tables\`, `Schemas\` et `old\`. Vingt fichiers basculés en `old` après vérification pièce par pièce. Il reste 14 documents à la racine, tous vérifiés utiles et sans recouvrement entre eux : mesuré, le recouvrement maximal entre deux d'entre eux est de 8 %, hors `descriptionRZ` à 32 %. **Il n'y avait pas de redondance dans ce dossier — il y avait un problème de nommage.**

**Trois documents ont été identifiés sous des noms qui ne les désignaient pas :**

| Nom trouvé | Ce que c'est réellement |
|---|---|
| `ListeTT_Module.odt` → `reference_modules_arduino_2026-02.odt` | **La documentation module par module de l'Arduino** : 1 128 blocs, 293 titres, table des matières. Lring, Lrub, FS, Ctrl_L, US, MvtIR, Mic, Mtr, Odom, Srv, SecMvt, Urg, CfgS, Gyro — propriétés VPIV, algorithmes, brochage. Les **12 constantes matérielles citées correspondent exactement à `config.h`**. C'est le document d'assemblage cherché pendant des jours |
| `tableC.csv` → `old\TableA-instantane-18avril2026_ex-tableC.csv` | Une **Table A** d'avril, pas une Table C. Strict sous-ensemble de la Table A actuelle, qui a gagné 8 lignes depuis : `Appli.IA_Conv` ×2, `Voice.output`, `Voice.ttsRate`, `Voice.vol`, `LockSE.camera`, `LockSE.microphone`, `STT.state` |
| `vtravailSyntheseRZV2.odt` | Le document d'assemblage **des documents du dossier A**, pas des condensés de modules. Absorbe `SmartPiloteAutonome` à 88 % |

**Les erreurs de documentation sont corrigées et poussées** : la Table C replacée sur SP, le nom réel de la Table D, l'ordre VPIV. Chaque correction porte un encart daté expliquant l'erreur, selon la convention de journalisation de ces pages.

**`docs/tables_RZ_descripteur.md` v3.0** est la référence normative des quatre tables. Il remplace `table_A_structure_regles` et `protocoleVPIV.odt`, tous deux en `old\`, et porte un journal des erreurs à huit entrées.

**`docs/installation_RPI_serveur.md`** comble un manque : le dépôt documentait l'installation de SE mais rien pour le RPi. Reconstruit depuis un journal de 9 066 lignes conservé hors dépôt.

**Le dépôt public est propre.** Deux pages publiaient l'IPv4 de la box et **l'adresse IPv6 complète du RPi** — la plus exposante des deux. Les neuf occurrences sont remplacées par des adresses réservées à la documentation (RFC 5737 et RFC 3849). Elles restent dans l'historique Git : l'historique n'a volontairement pas été réécrit, l'IPv4 d'une ligne résidentielle de juin ayant selon toute vraisemblance déjà changé. **La vraie mitigation est de revoir les redirections de ports de la box.**

Le reste du code est vérifié sain : `rz_ai_conversation.py`, `mqtt_bridge.py` et `set_git_remote.sh` lisent tout depuis `keys.json`, lui-même exclu du dépôt. Aucun identifiant en dur.

### 9.2 Vérifications utiles à ne pas refaire

- **`ui_VarG` est bien présent sur le RPi** — 50 nœuds, mêmes types et mêmes noms que sur le Xiaomi, page `P_admin-VarG` visible à `/admin_varG`. Seul écart : le groupe « Aide enrichedVars » du Xiaomi n'y est pas rattaché, et le tableau de bord du RPi n'a que 4 pages contre 11, donc rien n'y mène depuis l'accueil.
- **Les quatre fichiers `Diagrap*` de janvier-février 2025 ne sont pas de l'ancienne génération.** Ce sont les schémas de la version actuelle, non actualisés. 47 et 56 boîtes, exploitables comme modèles. Leur poids vient de 86 images incorporées. ⚠ `DiagrapRZCom-Protocole-Appli` contient une boîte « Adresses » avec des identifiants SSH : **il reste hors dépôt**, et cette boîte saute avant tout export.
- **`TableA_updatedV3_corrige.ods` et son `.csv` sont en phase** malgré cinq semaines d'écart de date.

### 9.3 Ce qui reste ouvert, et où cela va

| Chantier | État | Destination |
|---|---|---|
| **`TablesRZ_reference.ods`** — classeur unique, feuilles `_Version`, `TableA`, `TableC`, `TableD`, `Legende`, plus `TablesRZ_travaux.ods` pour les feuilles de travail | Pas commencé | **Chat Documentation centrale** |
| **`genese_et_evolution_RZ.md`** — fusion de `RZ-info-historique` et de la partie I de `historique.odt` (la partie II est de l'architecture, pas de l'histoire, et tous ses chiffres réseau sont faux) | Pas commencé | **Chat Documentation centrale** |
| **Synthèse unique agglomérée** avec index — l'objectif de départ | Pas commencé ; la carte de recouvrement des 9 documents est faite | **Chat Documentation centrale** |
| Remplir et chiffrer `RZ_Codes_Identifiants.odt` ; vider `InstallationServeur\` ; revoir les redirections de la box ; corriger l'anonymiseur (adresses MAC, frontières de mot) | En cours | **Nouveau chat « Sécurité et identifiants »** |
| Rénover les deux schémas `.drawio` | Analysés, non rénovés | À décider le moment venu |
| Verser `TableC.json`, `doc_A1` et `doc_A2` au dépôt | À faire | **Chat Documentation centrale** |

### 9.5 🔴 Arduino — défaut bloquant, vérifié sur le matériel le 22/10/2026

**La Mega ne peut recevoir aucune commande.** Découvert en lisant le code pour rédiger sa
documentation, puis confirmé sur la carte, firmware du dépôt téléversé.

#### Le mécanisme

Le fil transporte `$CAT:VAR:PROP:INST:VAL#` — c'est ce que `mqtt_bridge.py` documente (ligne 149),
ce que valide son `VPIV_REGEX`, et ce que `sendVPIV()` émet. **Mais la réception est construite pour
l'ordre inverse.** `communication.h` (lignes 41-47) l'explique de bonne foi : elle suppose que les
tokens 3 et 4 arrivent dans l'ordre INST puis PROP, et « compense » en permutant les arguments à
l'appel du routeur, lignes 179 et 190 :

```c
dispatchVPIV(msg.module, msg.inst, msg.prop, msg.value);   // actuel
// signature reelle, vpiv_dispatch.h ligne 87 :
// bool dispatchVPIV(const char *var, const char *prop, const char *inst, const char *value)
```

Le fil ayant changé de convention et pas la réception, cette compensation **introduit** l'erreur.
Les commentaires des dispatchers portent la même croyance : `dispatch_CfgS.cpp` s'annote
`// $V:CfgS:*:modeRZ:<n>#`, instance en position 3.

#### La preuve, lisible dans la sortie de la carte

```
envoyé  : $V:Urg:clear:*:1#
reçu    : $E:Urg:*:clear:urgence active#
```

La carte renvoie une erreur où `*` occupe la place de la propriété et `clear` celle de l'instance :
elle recopie les arguments tels qu'elle les a reçus après la permutation.

#### Trois défauts qui s'enchaînent

1. **Le chien de garde verrouille sur un seul dépassement.** `main.cpp` lignes 137-144 : au-delà de
   `LOOP_MAX_MS = 80`, `urg_handle(URG_LOOP_TOO_SLOW_A)`. Ni période de grâce au démarrage, ni
   exigence de répétition. Observé 1,5 s après le boot — mais **la carte était sans aucun capteur
   connecté** : NewPing attend l'expiration pour chacun des 9 ultrasons et le HX711 attend une
   donnée qui ne vient pas. À revérifier robot assemblé avant d'incriminer le seuil.
   Au passage, le `snprintf` qui construit `buf` dans ce bloc ne sert à rien.
2. **L'effacement est impossible**, par la permutation ci-dessus. Le verrou est donc définitif :
   seul le bouton reset agit, et il ramène au même point.
3. **Les deux portes d'effacement divergent.** `vpiv_dispatch.cpp` autorise `CfgS:emg:clear` ET
   `Urg:clear` ; le filtre amont de `communication.cpp` n'autorise que `Urg:clear`. La voie par
   CfgS est condamnée avant d'atteindre le routeur qui l'accepterait.

#### Correction

Lignes 179 et 190 de `communication.cpp` :

```c
dispatchVPIV(msg.module, msg.prop, msg.inst, msg.value);
```

Puis corriger le bloc de commentaires de `communication.h` (41-47) et les annotations des
dispatchers, qui propagent la croyance fausse. Élargir le filtre amont à `CfgS:emg:clear`.
Reconsidérer le chien de garde après essai avec les capteurs.

**Non appliqué à ce jour** : correction firmware, à faire et à téléverser par Vincent.

#### Ce que cela réhabilite

Les documents archivés les 21 et 22/10 pour « ordre VPIV inversé » — les deux fichiers de légende,
`IMP encodage et code arduino.odt` (58 trames sur 88), le `$Var,Instance,Prop,Valeur#` de la genèse
— **décrivaient fidèlement ce que la réception attend réellement**. Ils n'étaient pas négligents :
ils sont les témoins d'une migration de convention jamais achevée côté réception. Leur place reste
`old\` (la convention à suivre est bien PROP:INST), mais leur inversion avait une cause.

### 9.4 Correction à la feuille de route

Le lot F de la §6 et de précédentes versions listaient « `Appli.IA_Conv` : ajouter la ligne Table A manquante ». **Ces lignes existent** dans la Table A normative. Ne restent que l'entrée `ui_VarG` et le déploiement de la Table D.
