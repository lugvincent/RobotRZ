# Référence SE — Termux et Tasker

> **Fichier** : `docs/reference_SE_termux_tasker.md`
> **Objet** : permettre de relire et de retoucher le code du smartphone
> embarqué sans le redécouvrir. Qui démarre quoi, par où passe une trame, où
> vit l'état, et quels sont les pièges du code tel qu'il est.
> **Comment il a été écrit** : **depuis les sources du dépôt**. Les entêtes des
> scripts SE sont riches et à jour — ce document les met en perspective plutôt
> que de les répéter. Chaque affirmation technique a été vérifiée dans le code.
> **Articulation** : pendant SE de `docs/reference_firmware_arduino.md` ·
> `docs/tables_RZ_descripteur.md` pour la norme · `docs/synthese_etat.md` pour
> l'avancement.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe / Claude
> **Périmètre** : 42 scripts, environ 11 000 lignes, dans `termux/`.

---

## 1. Ce qu'est SE

Un smartphone Android — le Doro — **sans carte SIM**, qui sert de cerveau local
au robot. Il porte un Linux complet grâce à Termux, et accède aux applications
Android grâce à Tasker.

Il se connecte au réseau par le Wi-Fi de la maison, ou par le partage de
connexion d'un autre téléphone. Il est relié à l'Arduino Mega par son **port
USB-C**.

### Les deux voies, et pourquoi elles sont séparées

| | Porte | Pourquoi |
|---|---|---|
| **Termux** | Communication, capteurs du téléphone, logique, reconnaissance vocale | Un Linux : scripts, processus longs, outils réseau |
| **Tasker** | Applications Android — Zoom, caméra de surveillance, GPS, expressions à l'écran | Seul à savoir piloter Android de l'intérieur |

La frontière est nette et elle tient : Termux ne lance pas d'application
Android, Tasker ne fait pas de réseau.

---

## 2. Les quatre règles absolues

Elles sont écrites dans les entêtes des scripts, et tout le reste en découle.

**1. Jamais de `mosquitto_pub` direct.** Toute trame sortante passe par
`fifo_termux_in` → `rz_vpiv_parser.sh` → MQTT. Un seul point de sortie, donc un
seul endroit où déboguer, journaliser ou filtrer.

> Exception assumée : `rz_tasker_out.sh`, qui publie ce que Tasker écrit dans
> `vpiv_out.txt`. C'est l'autre extrémité, elle ne passe pas par le parser.

**2. SE ne commande jamais l'Arduino.** `safety_stop.sh` le dit explicitement :
même en urgence, SE **signale** à SP, et c'est SP qui décide de l'arrêt. SE n'a
pas d'autorité sur le matériel.

**3. Le format VPIV est `$CAT:MODULE:PROP:INST:VALUE#`** — propriété en
position 3. Vérifié dans tous les entêtes SE et dans la validation de
`mqtt_bridge.py`. **SE est du bon côté de la convention**, contrairement à ce
qu'était la réception Arduino jusqu'au 22/10/2026.

**4. Une ressource physique, un détenteur.** La caméra et le micro sont
protégés par des verrous — §11.

---

## 3. L'arborescence

```
termux/
 ├── config/          original_init.sh, courant_init.sh, check_config.sh,
 │                    courant_init.json, global.json, keys.json 🔒
 ├── fifo/            les cinq tubes nommés (créés à l'exécution)
 ├── scripts/
 │    ├── core/       rz_start, rz_vpiv_parser, rz_state-manager,
 │    │               mqtt_bridge.py, safety_stop, rz_tasker_out
 │    ├── sensors/    un dossier par capteur : sys, gyro, Mg, mic, cam,
 │    │               torch, voice, stt
 │    ├── appli/      rz_appli_manager — routage des trames CAT=A
 │    ├── utils/      fifo.manager, rz_resource_lock, error_handler,
 │    │               logger, rz_tasker_bridge
 │    └── tests/      simulateurs gyro et magnétomètre
 └── taskerScripts/
```

---

## 4. Le démarrage — `rz_start.sh`

Sept étapes, dans un ordre qui n'est pas arbitraire :

1. Vérification des dépendances — `jq`, `mosquitto`
2. Test de connexion au broker, avec réessais
3. Création des FIFOs manquantes
4. **Lancement du noyau** : `rz_vpiv_parser.sh`, `rz_state-manager.sh`,
   `rz_appli_manager.sh`
5. **Lancement des managers**, selon `courant_init.json` — chacun n'est lancé
   que si son mode n'est pas `OFF`
6. Annonce à SP : `$I:COM:info:SE:SE_READY#`
7. Attente infinie — les démons tournent en arrière-plan

```
bash rz_start.sh            démarrage
bash rz_start.sh --stop     arrêt propre
bash rz_start.sh --status   état des processus
```

> ⚠ **`rz_voice_manager.sh` est lancé avant `rz_stt_manager.sh`**, et
> inconditionnellement. La synthèse vocale doit être prête pour que la
> reconnaissance puisse annoncer qu'elle l'est.

> ⚠ **`SE_READY` déclenche l'initialisation côté SP.** C'est à sa réception
> que SP envoie la configuration. Un SE qui ne l'émet pas reste sur ses valeurs
> locales.

### La caméra et la torche ne sont pas des démons

`rz_camera_manager.sh` et `rz_torch_manager.sh` sont appelés **à la demande**,
en one-shot, par `rz_vpiv_parser.sh`, avec trois arguments : propriété, valeur,
instance.

Les lancer comme des démons — ce que `rz_start.sh` faisait avant juin 2026 —
produisait un `exit 1` immédiat, propriété vide, et polluait les journaux sans
conséquence fonctionnelle. Le piège est documenté dans l'entête de
`rz_start.sh` ; ne pas le réintroduire.

---

## 5. Le cœur — `rz_vpiv_parser.sh`

503 lignes, et c'est le passage obligé de tout ce qui entre et sort.

### Ce qu'il fait, dans l'ordre

1. **Écoute** MQTT sur `SE/commande`
2. **Valide** le format
3. **🔴 Point C — l'urgence**, avant tout le reste :
   `CAT=E` + `MODULE=Urg` + `PROP=etat` + `VALUE=danger` → `safety_stop.sh`
   immédiatement
4. **Retour vocal**, si `typePtge != 0` : les `COM:error`, `COM:info` et
   `Urg:etat` sont annoncés à voix haute
5. **Aiguillage** selon CAT et MODULE
6. **ACK** sur `SE/statut` pour chaque trame traitée

### La table d'aiguillage

| CAT | MODULE | Destination |
|---|---|---|
| `A` | tous | `fifo_appli_in` → `rz_appli_manager.sh` |
| `A` | `Voice` | `fifo_tasker_in` |
| `V` `F` | `CamSE` | `rz_camera_manager.sh`, en one-shot |
| `V` `F` | `Appli`, `Zoom`, `TTS` | `fifo_tasker_in` |
| `V` `F` | `CfgS` et le reste | `fifo_termux_in` |
| `I` `E` | tous | `fifo_termux_in` |
| `A` | `STT` + `PROP=intent` | **fichier de corrélation dédié** — exception v2.4 |

> L'exception STT mérite attention : ces trames **ne passent pas** par
> `fifo_termux_in` et **ne reçoivent pas d'ACK**. Elles suivent un circuit à
> part, lié à la corrélation des intentions vocales.

---

## 6. Les cinq FIFOs

Les tubes internes entre processus, sans passer par MQTT.

| FIFO | Écrit par | Lu par |
|---|---|---|
| `fifo_termux_in` | tous les managers, `safety_stop`, `error_handler` | `rz_vpiv_parser.sh` → MQTT |
| `fifo_tasker_in` | `rz_vpiv_parser.sh` | `rz_tasker_bridge.sh` → Tasker |
| `fifo_appli_in` | `rz_vpiv_parser.sh` | `rz_appli_manager.sh` |
| `fifo_voice_in` | `rz_vpiv_parser.sh` | `rz_voice_manager.sh` |
| `fifo_return` | `rz_vpiv_parser.sh` | Tasker — ACK, bascule IA |

**`fifo_termux_in` est de loin la plus sollicitée** : 73 références dans le
code, contre 15 à 23 pour les autres. C'est le point de sortie unique de la
règle n°1.

> ⚠ **`fifo.manager.sh` est périmé** : il ne déclare et ne crée que quatre
> FIFOs, `fifo_voice_in` lui manque. Sans conséquence à l'exécution —
> `rz_start.sh`, `rz_vpiv_parser.sh` et `rz_voice_manager.sh` la créent chacun
> de leur côté — mais l'outil de diagnostic `fifo_manager.sh check` rendra un
> verdict incomplet. Voir §13.

---

## 7. L'état — `rz_state-manager.sh` et `global.json`

`global.json` est **la mémoire partagée de SE**. Tous les managers y lisent ;
le state-manager est le seul à y écrire, en écriture atomique.

| Bloc | Champs | Qui écrit |
|---|---|---|
| `CfgS` | `modeRZ` (0-5), `typePtge` (0-4) | `rz_state-manager` exclusivement |
| `Urg` | `source`, `statut`, `niveau` | `rz_vpiv_parser`, `error_handler` |
| `COM` | `debug`, `error`, `info`, `warn` | `rz_vpiv_parser` |
| `Sys` | `cpu_load`, `temp`, `batt`… | `rz_sys_monitor` exclusivement |

> Après chaque écriture, `global.json` est **recopié vers
> `/sdcard/Tasker/RobotRZ/`** — c'est ainsi que la tâche Tasker `RZ_OuvreEtat`
> lit l'état de SE. Sans cette copie, Tasker travaille sur une version figée.

### Le calcul de `STT.context`

Le state-manager décide du contexte vocal à partir du mode du robot :

| `modeRZ` | Contexte |
|---|---|
| 0 ARRÊT · 5 ERREUR | `Inactif` |
| 1 VEILLE · 2 FIXE · 4 AUTONOME | `Mixte` |
| 3 DÉPLACEMENT | `Cmde` seulement — sécurité et rapidité |

Et si `typePtge` n'est ni Vocal (1) ni Laisse+Vocal (4), le contexte est
`Inactif` quoi qu'il arrive.

**Un raffinement de sécurité** est prévu : si `modeRZ=1` (VEILLE) **et** qu'un
mouvement est détecté — `Mtr.state` avec L ou R non nul — le contexte est forcé
à `Cmde`, pour qu'une conversation avec Gemini ne distraie pas pendant un
déplacement.

> ⚠ **Ce raffinement est inactif.** Il suppose que SP relaie `Mtr.state` vers
> SE sur `SE/commande`, câblage qui n'est probablement pas fait côté Node-RED.
> L'entête du script le signale lui-même. Sans danger — L et R restent à 0 —
> mais la protection n'existe pas.

---

## 8. Les managers de capteurs

### Le modèle commun

Tous suivent la même forme, ce qui rend l'ensemble lisible :

1. Charger `<capteur>_runtime.json`, généré par `check_config.sh`, avec repli
   sur `courant_init.json`
2. Envoyer les ACK de configuration à SP
3. Boucler selon le mode actif
4. Publier dans `fifo_termux_in`

### Les quatre modes

| Mode | Comportement |
|---|---|
| `OFF` | Aucune surveillance, le script se termine normalement |
| `FLOW` / `DATACont` | Publie à chaque cycle, période `freqCont` |
| `MOY` / `DATAMoy` | Accumule `nbCycles` mesures, publie la moyenne |
| `ALERTE` | Aucun flux, surveillance des seuils uniquement |

### Démon ou appel ponctuel

| Démon | Appelé à la demande |
|---|---|
| `rz_sys_monitor` · `rz_gyro_manager` · `rz_mg_manager` · `rz_microSe_manager` · `rz_voice_manager` · `rz_stt_manager` | `rz_camera_manager` · `rz_torch_manager` |

### Deux cas particuliers

**La caméra** est pilotée par Tasker, via `rz_tasker_bridge.sh` → `RZ_CamStart`
et `RZ_CamStop`. Deux instances, `front` et `rear`. Termux ne touche pas à la
caméra lui-même.

**La torche** bascule entre deux mécanismes selon l'état de la caméra : si IP
Webcam tourne, elle passe par son API HTTP (`enablels` / `disablels`) ; sinon
par `termux-torch`. Le même bouton, deux chemins.

---

## 9. La chaîne vocale

La partie la plus élaborée de SE, et celle qui a le plus évolué.

```
  micro
    ↓   PocketSphinx, mot-clé « rz »
  rz_stt_manager.py            reconnaissance, supervisée par le .sh
    ↓
  rz_autovoice_bridge.sh       arbitrage selon STT.context : Cmde / Conv / Mixte
    ↓
  rz_stt_handler.sh            cœur décisionnel
    ├── trouvé dans le catalogue  →  trame VPIV → FIFO
    └── exit 1, non trouvé        →  rz_ai_conversation.py → Gemini → TTS
```

### `rz_stt_handler.sh` — le cœur

1. Cherche la phrase dans `stt_catalog.json`, la Table D opérationnelle
2. Applique **cinq filtres de sécurité** : charge CPU, `modeRZ`, laisse…
3. Agit selon la colonne *Traitement* de la ligne trouvée :
   - **`SIMPLE` / `LocalSE`** → résolu entièrement sur SE
   - **`TRADUCTION` / `CALCUL` / `MACRO`** → transmis **brut** à SP, qui résout
     par le mécanisme générique `ui_Traitement`
4. Dépose les trames dans le circuit FIFO
5. Rend un code de retour — `exit 1` déclenche la bascule vers l'IA

> Côté SP, `fn_stt_intent_router` doit distinguer une capsule **à 5 champs**
> — déjà résolue par SE — d'une capsule **à 6 champs** — à résoudre.

### La construction du catalogue

```
tableD_catalogueV2.csv  →  rz_build_system.py  →  stt_catalog.json
                                               →  rz_words.txt
                                                     ↓  rz_build_dict.py
                                                   fr.dict  (PocketSphinx)
```

`rz_build_system.py` filtre les lignes séparatrices et **les lignes marquées
`⛔`** — entrées en erreur connue, non déployées. C'est l'origine de l'écart
entre le nombre de commandes annoncées et déployées.

### Le repli vers l'IA

`rz_ai_conversation.py` interroge **Gemini 2.5 Flash**, avec une personnalité
de robot compagnon, et joue la réponse en synthèse vocale. Délai d'attente de
10 secondes. La clé API est lue dans `config/keys.json`, **jamais en dur** —
vérifié.

---

## 10. Le pont Tasker

Deux sens, deux mécanismes différents.

**Termux → Tasker** : `rz_tasker_bridge.sh` lit `fifo_tasker_in` et déclenche
les tâches Tasker.

**Tasker → Termux → SP** : Tasker écrit dans `vpiv_out.txt` ;
`rz_tasker_out.sh` surveille ce fichier avec `inotifywait`, filtre les doublons
et les lignes vides, et publie sur `SE/statut`.

C'est ce qui remonte les commandes du menu Tasker et les accusés de réception
d'ouverture ou de fermeture d'application.

---

## 11. Les verrous de ressources

Deux éléments physiques ne peuvent servir à deux usages à la fois : **la caméra**
— Zoom contre IP Webcam — et **le micro** — Zoom contre la reconnaissance
vocale.

`rz_resource_lock.sh` tient `resource_locks.json` :

```json
{ "camera":     { "locked_by": null, "since": null },
  "microphone": { "locked_by": null, "since": null } }
```

> **La règle d'or** : seul celui qui détient un verrou peut le lever.
> `unlock_resource()` vérifie `locked_by` avant de libérer.

Ce fichier n'est pas exécuté : il est **sourcé** par les managers qui en ont
besoin.

---

## 12. La chaîne d'initialisation

```
original_init.sh     installation complète, une seule fois — 823 lignes
   ├─ crée l'arborescence
   ├─ génère courant_init.json   configuration de tous les modules
   ├─ génère global.json         état initial, modeRZ = 1 VEILLE
   ├─ génère le catalogue STT    section « === SECTION CATALOGUE STT === »
   ├─ lance rz_build_system.py puis rz_build_dict.py
   └─ lance check_config.sh

check_config.sh      distribue — 733 lignes
   └─ courant_init.json  →  gyro_runtime.json, mag_runtime.json,
                            sys_runtime.json, cam_config.json,
                            mic_config.json, stt_config.json,
                            appli_config.json
```

Les fichiers `*_runtime.json` et `*_config.json` sont **générés, donc exclus du
dépôt**. À régénérer après toute modification de `courant_init.json`.

> 🔴 **Le catalogue STT est enfermé dans `original_init.sh`.** Sa source
> génératrice est une section de ce script d'installation, qui tourne sur le
> Doro. Le CSV du projet n'en est qu'une copie corrigée, et la V3 à
> 72 commandes n'a **aucun chemin vers le téléphone**. C'est le lot I de la
> feuille de route.

---

## 13. 🔴 Les pièges et les points ouverts

**1. Six fichiers du dépôt sont vides** — et le sont depuis toujours.
`termux/docs/api_vpiv.md`, `architecture.md`, `installation.md`, et les trois de
`termux/install/` — `README.md`, `setup_python.sh`, `setup_termux.sh`. Zéro
octet.

> ⚠️ **Correction du 22/10/2026** : j'avais d'abord écrit que le Project
> Knowledge en détenait le contenu et que le dépôt l'avait perdu. **C'est
> faux** — vérification faite, ils y sont vides aussi, tous créés le 11 mars
> 2026 dans un même lot. Ce sont des **emplacements réservés jamais remplis**,
> pas des fichiers perdus. Rien n'a disparu.

Reste à décider : les écrire, ou les supprimer. Des fichiers vides dans un
dépôt donnent l'illusion d'une documentation qui n'existe pas.

**2. `fifo.manager.sh` ne connaît que quatre FIFOs sur cinq.** `fifo_voice_in`
lui manque. Sans effet à l'exécution, mais `fifo_manager.sh check` répond à
côté.

**3. Le raffinement de sécurité `Mtr.state` est inactif.** Le câblage SP → SE
n'est probablement pas fait côté Node-RED. Une conversation avec l'IA peut donc
se poursuivre pendant un déplacement initié en mode VEILLE.

**4. Le nom de la Table D est codé en dur à trois endroits** :
`original_init.sh` ligne 641, `rz_build_system.py` ligne 107,
`rz_build_grammar.py` ligne 65. Tout renommage doit les corriger ensemble.

**5. `rz_stt_handler.sh` v3.2 n'est pas déployé sur le Doro.** La version du
dépôt est en avance sur celle du téléphone.

**6. L'exception STT du parser est silencieuse.** Les trames
`CAT=A` + `MODULE=STT` + `PROP=intent` ne reçoivent pas d'ACK. Normal et
documenté, mais déroutant quand on cherche un accusé qui ne viendra pas.

**7. La caméra et la torche ne sont pas des démons.** Les lancer comme tels
produit un `exit 1` silencieux. Piège déjà tombé une fois, en juin 2026.

---

## 14. Pour retoucher un script SE

1. **Lire son entête.** Ils sont riches et tenus à jour — objectif,
   articulation, précautions. C'est la meilleure source du projet.
2. **Vérifier le nom de la propriété dans la Table A**, pas dans un commentaire.
3. **Ne jamais appeler `mosquitto_pub` directement.** Passer par
   `fifo_termux_in`.
4. **Ne jamais commander l'Arduino depuis SE.** Signaler à SP, c'est tout.
5. **Si le script touche la caméra ou le micro**, poser un verrou.
6. **Si la configuration change**, relancer `check_config.sh` pour régénérer
   les `*_runtime.json`.
7. **Tester avec les simulateurs** quand c'est possible :
   `scripts/tests/test_gyro_simulator.sh`, `test_mg_simulator.sh`.

---

*V1 — 22/10/2026. Écrit depuis les sources, dans le prolongement de
`reference_firmware_arduino.md`. Les deux documents couvrent désormais les deux
sous-systèmes qui portent du code embarqué ; restent SP et Tasker.*
