# Document de Référence — Système Embarqué SE du projet RZ

**Projet :** RZ — Robot compagnon autonome  
**Auteur :** Vincent Philippe  
**Version :** 1.0 — Mars 2026  
**Usage :** Document de référence technique complet du Smartphone Embarqué (SE)

---

# PARTIE 1 — GÉNÉRALE

## 1.1 Présentation du projet RZ et architecture logique

### Le projet RZ

RZ est un robot compagnon développé à titre bénévole. Il intègre des capacités de déplacement autonome, de surveillance environnementale, de reconnaissance vocale et d'interaction avec l'utilisateur. Le système repose sur quatre composants matériels communicant via un protocole unifié (VPIV) porté par MQTT.

Le développement mobilise plusieurs langages : C++ (Arduino), JavaScript (Node-RED), Bash (Termux), Python (scripts SE) et JSON (configuration).

### Les trois couches logiques

**Couche système** — infrastructure de base sur chaque appareil :
- Sur Arduino : firmware C++, librairies capteurs/actionneurs
- Sur SE (smartphone) : Android + Termux (environnement Linux), Tasker (automatisation), applications tierces (IP Webcam, PocketSphinx)
- Sur SP (ordinateur) : Node-RED, broker MQTT Mosquitto

**Couche applicative** — scripts et modules métier :
- Managers de capteurs (Bash) : gyroscope, magnétomètre, surveillance système, microphone, caméra
- Module STT/TTS : reconnaissance et synthèse vocale
- Gestionnaire d'état : cohérence globale, transitions de modes
- Bridge MQTT/Série : liaison Arduino ↔ réseau

**Couche protocolaire** — échanges inter-composants :
- Protocole VPIV : messages structurés `$CAT:VAR:PROP:INST:VAL#`
- Transport : MQTT (broker externe DuckDNS)
- Topics principaux : `SE/commande` (SP→SE), `SE/statut` (SE→SP), `SE/arduino/data` (A→SP)

### Schéma d'ensemble

```
[Utilisateur]
     │ voix / interface Node-RED
     ▼
[SP — Node-RED]  ←────────── broker MQTT (robotz-vincent.duckdns.org:1883)
     │                                          │
     │ VPIV MQTT                                │ VPIV MQTT
     ▼                                          │
[SE — Smartphone embarqué] ───────────────────►│
  ├─ Termux (scripts Bash, Python)              │
  ├─ Tasker (automatisation Android)            │
  ├─ IP Webcam (streaming)                      │
  └─ USB-C ──► [A — Arduino Mega]
                 └─ I2C/UART ──► [Arduino UNO — moteurs]
```

---

## 1.2 Les acteurs — appareils et applications

### Arduino Mega (A)

**Rôle :** Contrôleur temps-réel des capteurs et actionneurs physiques.  
**Responsabilités :** lecture gyroscope, magnétomètre, capteurs ultrasons (9), microphones (3), détecteur IR, commande des LEDs (anneau Lring, ruban Lrub), servomoteurs (3), sécurité mouvement.  
**Communication :** port série USB-C vers SE. Le bridge `mqtt_bridge.py` convertit les données série en messages MQTT.  
**Relation :** esclave de SP pour la configuration et les consignes ; publie ses données vers SP via SE.

**Arduino UNO (esclave moteur)**  
Dédié exclusivement au pilotage PWM des moteurs. Reçoit ses ordres de l'Arduino Mega via liaison série I2C/UART. Invisible depuis SE et SP.

### SE — Smartphone embarqué

**Rôle :** Cerveau local de la plateforme mobile. Traite les données capteurs SE, exécute les commandes SP, gère la reconnaissance vocale, pilote les applications Android.  
**Contraintes :** pas de carte SIM, connexion via Wi-Fi (box à domicile ou partage de connexion d'un autre smartphone).

**Applications installées sur SE :**

| Application | Rôle | Déclencheur |
|-------------|------|-------------|
| Termux | Environnement Linux sur Android — exécute tous les scripts Bash et Python | Lancé au démarrage |
| Tasker | Automatisation Android — lance/arrête des apps, gère les intents Android | Déclenché par `rz_tasker_bridge.sh` |
| IP Webcam | Streaming vidéo via HTTP (port 8080) | Géré par `rz_camera_manager.sh` |
| PocketSphinx | Reconnaissance vocale locale (wake word + commandes) | Géré par `rz_stt_manager.sh/.py` |
| Gemini (API cloud) | IA conversationnelle de repli quand commande non reconnue | Appelé par `rz_ai_conversation.py` |
| Termux:API | Accès aux APIs Android depuis Termux (TTS, torch, sensors) | Utilisé par les managers |

### SP — Système Pilote (Node-RED)

**Rôle :** Superviseur maître absolu du système. Décide de tous les modes, envoie la configuration initiale, traite les alertes, fournit l'interface utilisateur.  
**Responsabilités :** configuration au démarrage (courant_init.json), affichage des données, validation des intents STT, gestion des urgences, historisation.  
**Outil :** Node-RED sur ordinateur, flux JavaScript.

### Bridge MQTT/Série (`mqtt_bridge.py`)

**Rôle :** Pont entre le port série USB-C (Arduino Mega) et le broker MQTT.  
**Fonctionnement :** lit les trames série de l'Arduino (ex : données gyroscope `x,y,z`), les parse et les publie sur MQTT topic `SE/arduino/data`. Reçoit les commandes MQTT topic `SE/arduino/commande` et les transmet en série vers l'Arduino.

### Termux:API

Passerelle entre les scripts Bash/Python et les fonctions Android natives :
- `termux-tts-speak` : synthèse vocale (TTS)
- `termux-torch` : contrôle de la torche
- `termux-sensor` : lecture des capteurs Android (gyroscope, magnétomètre, température)
- `termux-tasker` : déclenchement de tâches Tasker

### Tableau de synthèse des responsabilités

| Acteur | Configure | Décide | Exécute | Publie |
|--------|-----------|--------|---------|--------|
| SP | courant_init.json, modes | modeRZ, urgences | interface | commandes MQTT |
| SE | *_runtime.json depuis SP | réponses locales | managers, STT | statuts MQTT |
| A (Mega) | depuis SP via SE | sécurité HW | capteurs/actionneurs | données série |
| Tasker | tâches Android | aucune | apps Android | — |

---

## 1.3 Protocole VPIV

### Principe

VPIV est le protocole de communication unifié du projet RZ. Chaque message est une chaîne texte structurée transportée via MQTT. Son nom est l'acronyme des champs : **V**ariable, **P**ropriété, **I**nstance, **V**aleur — précédés d'une catégorie.

### Structure d'un message VPIV

```
$CAT:VAR:PROP:INST:VAL#
```

| Champ | Description | Exemples |
|-------|-------------|----------|
| `$` | Délimiteur de début | toujours `$` |
| CAT | Catégorie du message (1 lettre) | V, I, E, F, A |
| VAR | Variable ou module concerné | Gyro, CamSE, Mtr, CfgS |
| PROP | Propriété de la variable | modeRZ, active, cmd |
| INST | Instance (ou `*` pour toutes) | rear, front, TGD, * |
| VAL | Valeur | On, 3, 50\|0\|2 |
| `#` | Délimiteur de fin | toujours `#` |

### Catégories (CAT)

| CAT | Nom | Usage | Direction |
|-----|-----|-------|-----------|
| V | Consigne (Valeur) | Ordre direct, à exécuter | SP→SE, SP→A |
| I | Information / ACK | Retour d'état, confirmation | SE→SP, A→SP |
| E | Événement critique | Urgences, erreurs majeures | SE→SP, A→SP |
| F | Flux | Données continues (capteurs) | SE→SP, A→SP |
| A | Application | Commandes applicatives (Tasker, apps Android) | SP→SE |

### Exemples annotés

```bash
# Ordre : passer en mode déplacement
$V:CfgS:modeRZ:*:3#
#  │   │       │  └─ valeur : mode 3
#  │   │       └─── instance : * (unique)
#  │   └──────────── propriété : modeRZ
#  └──────────────── variable : CfgS (config système)

# Flux gyroscope
$F:Gyro:dataCont:*:[1200,-450,9800]#

# Urgence surchauffe (SE vers SP)
$E:Urg:source:SE:URG_OVERHEAT#

# ACK caméra démarrée (SE vers SP)
$I:CamSE:active:rear:On#

# Commande applicative Tasker
$A:Appli:zoom:*:On#
```

### Règles d'écriture VPIV

1. **Toujours `$` en début et `#` en fin** — les parsers utilisent ces délimiteurs.
2. **Exactement 4 `:` séparateurs** — structure stricte, pas de champ vide sauf VAL optionnel.
3. **Instance `*`** désigne toutes les instances ou l'instance unique d'un module.
4. **Commandes ponctuelles** : propriétés entre parenthèses `(reset)`, `(clear)`, `(snap)` — déclenchent un effet immédiat sans état durable.
5. **Pas de float implicite** : utiliser des multiplicateurs documentés (×10, ×100) et les noter dans la Table A.
6. **Objets sérialisés** : pour les blocs de configuration, la valeur est un JSON inline entre `{}` — ex : `$V:Gyro:paraM:*:{...}#`
7. **CAT=E** exclusivement pour les urgences nécessitant une réaction automatique. Les informations sans réaction utilisent CAT=I.

---

## 1.4 Les tables de référence

### Table A — Table mère des variables VPIV

La Table A est la **référence unique du projet**. Elle décrit toutes les variables manipulées par le système, quelle que soit leur localisation (SP, SE, Arduino).

**Rôle :** Sert à la construction des messages VPIV, la validation des intents STT, la cohérence entre l'interface SP, la logique SE et le firmware Arduino.

**Champs de la Table A :**

| Champ | Description | Exemples |
|-------|-------------|----------|
| VAR | Nom de la variable/module | Gyro, CamSE, Mtr, CfgS |
| PROP | Propriété (ou commande entre `()`) | modeGyro, active, (reset) |
| CAT | Catégorie VPIV possible | V, I, E, F, A |
| TYPE | Type logique de la valeur | boolean, number, string, enum, object |
| FORMAT_VALUE | Format attendu dans le message | 0\|1, 0-255, "R,G,B", OFF\|KWS |
| DIRECTION | Qui envoie / qui reçoit | SP→SE, A→SP, SP↔SE |
| INITIAL | Mode d'initialisation | ini, Dyn, Cste |
| STATUT | Nature de la propriété | ETAT, EXPOSEE |

**Règles de la Table A :**

- **TYPE = type logique du VPIV**, pas forcément le type C++ interne. Ex : `boolean` en Table A → `"1"` ou `"0"` dans le VPIV → `bool` en C++.
- **FORMAT_VALUE = contraintes de validation** : valeurs acceptées + format attendu.
- **Commandes ponctuelles** `(prop)` : effet immédiat, CAT=V uniquement. Ne pas confondre avec une propriété à flux.
- **TYPE object** : utilisable pour la configuration (paramètres de calibration, blocs rarement modifiés). Jamais pour le temps réel. Le VPIV transporte la sérialisation JSON — c'est SP/SE qui désérialise.
- **TYPE enum** : le string est normalisé avec valeurs alternatives documentées. Représenté comme string dans le VPIV.
- **Facteurs multiplicateurs** (×10, ×100) : toujours documentés explicitement, jamais implicites.
- **INITIAL=ini** : initialisé par SP au démarrage depuis JSON. Peut évoluer au runtime.
- **INITIAL=Dyn** : produit uniquement en fonctionnement, non initialisé.
- **INITIAL=Cste** : modifiable uniquement hors exécution (flash, calibration).

### Table C — courant_init.json

**Rôle :** Image de la Table A côté SP, contenant toutes les variables SE et Arduino avec leurs valeurs initiales.

**Workflow :**
```
SP charge Table A → extrait valeurs initiales → construit courant_init.json
courant_init.json → transmis à SE au démarrage (ou simulé par courant_init.sh en dev)
SE reçoit → check_config.sh → distribue en *_runtime.json et *_config.json par module
```

**Contenu :** blocs gyro, mag, sys, cam, mic, appli, stt, mtr — chacun contenant les paramètres de son manager.

**En développement :** `courant_init.sh` génère `courant_init.json` localement, simulant l'envoi de SP.  
**En production :** `courant_init.json` est reçu via MQTT depuis SP.

### Table D — Catalogue STT (table_d_catalogue.csv)

La Table D recense toutes les commandes vocales reconnues par RZ. Elle est la source unique de vérité pour le module STT. Voir **Documents B1 et B2** pour le détail complet.

**Structure CSV (13 colonnes) :**

| Colonne | Description |
|---------|-------------|
| ID | Identifiant unique de la commande (ex: MTR_FORWARD) |
| Traitement | SIMPLE \| COMPLEXE \| PLAN \| LOCAL |
| CAT | Catégorie VPIV produite |
| Destinataire | A (Arduino) \| SE \| SP |
| moteurActif | 1 = implique les moteurs (bloqué en mode laisse) |
| Alias | Synonymes prononcés, séparés par virgule |
| ID_fils | Pour COMPLEXE : liste d'IDs enfants |
| VAR, PROP, INSTANCE, VALEUR | Parties de la trame VPIV produite |
| Prepa_Encodage | Pour PLAN : transformation du nombre extrait |
| Alias-TTS | Réponse vocale du robot, variantes séparées par `\|` |

**Maintenance :** La Table D est maintenue dans `original_init.sh` (section balisée `=== SECTION CATALOGUE STT ===`). Après toute modification, relancer le script pour régénérer `stt_catalog.json` et `fr.dict`.

---

## 1.5 Modalités réseau selon le contexte

| Contexte | Connexion SE | Broker MQTT | Remarques |
|----------|-------------|-------------|-----------|
| Domicile | Wi-Fi box | robotz-vincent.duckdns.org | Fonctionnement nominal |
| Déplacement | Partage connexion smartphone tiers | idem via 4G/5G | SE sans SIM propre |
| Développement PC | Wi-Fi local ou même réseau | idem ou broker local | courant_init.sh simulé |

**Broker MQTT :** `robotz-vincent.duckdns.org:1883`  
**Topics :** `SE/commande` (SP→SE), `SE/statut` (SE→SP), `SE/arduino/data` (A→SP via bridge)

---

## 1.6 Articulation entre applications

### Séquence de démarrage sur SE

```
1. Termux démarre → scripts disponibles
2. original_init.sh (si 1ère install) ou courant_init.sh (dev)
3. check_config.sh → génère tous les *_runtime.json / *_config.json
4. mqtt_bridge.py → connexion Arduino ↔ MQTT
5. rz_vpiv_parser.sh → écoute SE/commande (boucle principale)
6. rz_state-manager.sh → gestion états globaux
7. Managers capteurs : rz_gyro_manager.sh, rz_mg_manager.sh,
   rz_sys_monitor.sh, rz_microSE_manager.sh
8. rz_stt_manager (sh ou py) → PocketSphinx en écoute wake word
9. SP envoie courant_init.json → configuration opérationnelle
```

### Dépendances inter-applications

- **Tasker doit être lancé** avant les apps qui en dépendent (Baby, BabyMonitor, NavGPS, Zoom).
- **mqtt_bridge.py doit être actif** avant tout échange avec l'Arduino.
- **rz_vpiv_parser.sh est le point d'entrée** de toutes les commandes MQTT entrantes.
- **rz_stt_manager** dépend de `stt_catalog.json` (généré par `rz_build_system.py`) et `fr.dict` (généré par `rz_build_dict.py`).

### Flux applicatif simplifié

```
SP (Node-RED)
  │ $V:CamSE:active:rear:On#  via MQTT SE/commande
  ▼
rz_vpiv_parser.sh
  │ cas CamSE → appelle directement
  ▼
rz_camera_manager.sh
  │ démarre IP Webcam via am start
  │ publie $I:CamSE:active:rear:On# + $F:CamSE:streamURL:rear:{http://...}#
  ▼
FIFO_TERMUX_IN → rz_state-manager.sh → MQTT SE/statut → SP
```

---

## 1.7 global.json — Variables partagées et sécurité

### Rôle

`global.json` est la mémoire partagée du SE, lue et écrite par tous les managers. Il contient l'état dynamique courant du système. Il est initialisé par `original_init.sh` au démarrage puis mis à jour en continu.

### Les 4 blocs

**CfgS — Configuration système**

| Champ | Type | Valeur initiale | Description |
|-------|------|-----------------|-------------|
| modeRZ | number (0-5) | 1 | Mode global robot. 0=ARRET, 1=VEILLE, 2=FIXE, 3=DEPLACEMENT, 4=AUTONOME, 5=ERREUR |
| typePtge | number (0-4) | 0 | Type de pilotage. 0=Ecran, 1=Vocal, 2=Manette, 3=Laisse, 4=Laisse+Vocal |
| reset | boolean | false | Drapeau de réinitialisation demandée |

**Urg — Urgences**

| Champ | Type | Valeur initiale | Description |
|-------|------|-----------------|-------------|
| source | string (enum) | "URG_NONE" | Code de l'urgence active |
| statut | string (enum) | "cleared" | État : cleared \| active |
| niveau | string (enum) | "warn" | Niveau si actif : warn \| danger |

**COM — Messages**

| Champ | Type | Valeur initiale | Description |
|-------|------|-----------------|-------------|
| debug | string | "" | Message de débogage courant |
| error | string | "" | Dernier message d'erreur |
| info | string | "" | Dernière information |
| warn | string | "" | Dernier avertissement |

**Sys — Métriques système**

| Champ | Type | Valeur initiale | Description |
|-------|------|-----------------|-------------|
| cpu_load | number (%) | 0 | Charge CPU courante |
| temp | number (°C) | 0 | Température du processeur |
| storage | number (%) | 0 | Occupation stockage SDCard |
| mem | number (%) | 0 | Occupation mémoire RAM |
| batt | number (%) | 0 | Niveau batterie SE |
| uptime | number (s) | 0 | Durée depuis boot |
| last_update | string | "" | Horodatage dernière mesure |

### Gestion de la sécurité

Le SE implémente une sécurité à plusieurs niveaux :

**Niveau 1 — Filtrage CPU dans rz_stt_handler.sh**  
Si `cpu_load > 90%` : abandon de la commande vocale en cours, message TTS.

**Niveau 2 — Filtrage modeRZ dans rz_stt_handler.sh**  
Si `modeRZ = 0 (ARRET)` ou `5 (ERREUR)` : toutes les commandes vocales bloquées sauf URGENCY_STOP.

**Niveau 3 — Filtrage laisse dans rz_stt_handler.sh**  
Si `typePtge = 4 (Laisse+Vocal)` et commande moteur (`moteurActif=1`) : commande refusée sauf STOP.

**Niveau 4 — Surveillance système dans rz_sys_monitor.sh**  
Dépasse `cpu.urg` ou `thermal.urg` → envoi `$E:Urg:source:SE:URG_CPU_CHARGE#` vers SP.  
Descend sous `batt.urg` → envoi `$E:Urg:source:SE:URG_LOW_BAT#` vers SP.

**Niveau 5 — Réflexe danger dans rz_vpiv_parser.sh (Point C)**  
Réception de `$E:Urg:etat:SE:danger#` depuis SP → déclenchement immédiat de `safety_stop.sh` :
- Arrêt moteur : `$V:MotA:state:*:Stop#`
- Coupure caméra : `$V:CamSE:active:*:Off#`
- Extinction torche : `$V:TorchSE:power:*:0#`
- Log emergency.log

**Cycle de vie d'une urgence :**
```
Détection (SE ou A)
  → $E:Urg:source:SE:URG_XXX#  vers SP
  → SP analyse → si danger → $E:Urg:etat:SE:danger#
  → rz_vpiv_parser.sh → safety_stop.sh
  → $I:Urg:etat:SE:danger#  (ACK)
  → Résolution → $V:CfgS:(emg):*:clear#  depuis SP
  → urg_clear() Arduino + reset global.json
```

**Codes d'urgence définis :**

| Code | Déclencheur | Action SP |
|------|-------------|-----------|
| URG_NONE | — | Aucune urgence |
| URG_LOW_BAT | Batterie < seuil | Arrêt moteur |
| URG_CPU_CHARGE | CPU > seuil | Alerte affichage |
| URG_OVERHEAT | Température > seuil | Arrêt moteur |
| URG_MOTOR_STALL | Blocage moteur | Arrêt moteur |
| URG_SENSOR_FAIL | Défaillance capteur | Alerte |
| URG_BUFFER_OVERFLOW | Mémoire | Arrêt partiel |
| URG_PARSING_ERROR | Trame invalide | Log |
| URG_LOOP_TOO_SLOW | Boucle lente | Alerte |

---

# PARTIE 2 — DYNAMIQUE DU SE

## 2.1 Arborescence commentée

```
~/scripts_RZ_SE/termux/
│
├── config/                         ← Configurations opérationnelles (générées)
│   ├── courant_init.json           ← Image Table C : config complète SP→SE
│   ├── global.json                 ← État dynamique partagé (mémoire SE)
│   ├── gyro_runtime.json           ← Config gyroscope validée par check_config
│   ├── mag_runtime.json            ← Config magnétomètre validée
│   ├── sys_runtime.json            ← Config surveillance système validée
│   ├── appli_config.json           ← Config applications Tasker
│   └── sensors/
│       ├── cam_config.json         ← Config caméra (instances + profils)
│       ├── mic_config.json         ← Config microphone SE
│       └── stt_config.json         ← Config PocketSphinx (STT)
│
├── scripts/
│   ├── core/                       ← Scripts noyau du SE
│   │   ├── rz_vpiv_parser.sh       ← Point d'entrée MQTT : parse et aiguille
│   │   ├── rz_state-manager.sh     ← Gestion états, transitions modeRZ/typePtge
│   │   └── safety_stop.sh          ← Arrêt d'urgence immédiat (Point C)
│   │
│   └── sensors/                    ← Managers de capteurs et modules SE
│       ├── cam/
│       │   └── rz_camera_manager.sh  ← Pilotage IP Webcam (start/stop/snap)
│       ├── stt/
│       │   ├── rz_stt_manager.sh     ← Boucle PocketSphinx (wake word)
│       │   ├── rz_stt_manager.py     ← Variante Python du manager STT
│       │   ├── rz_stt_handler.sh     ← Cœur décisionnel : catalogue → VPIV
│       │   ├── rz_ai_conversation.py ← Repli IA Gemini
│       │   ├── rz_build_system.py    ← Build : CSV → stt_catalog.json + rz_words.txt
│       │   ├── rz_build_dict.py      ← Build : rz_words.txt → fr.dict
│       │   ├── table_d_catalogue.csv ← Source du catalogue (dans original_init.sh)
│       │   ├── stt_catalog.json      ← Catalogue compilé (généré)
│       │   ├── rz_words.txt          ← Mots du catalogue (généré)
│       │   └── lib_pocketsphinx/
│       │       ├── model-fr/         ← Modèle acoustique français (~50-200 Mo, hors Git)
│       │       ├── lexique_referent.dict  ← Dict source fr (hors Git)
│       │       └── fr.dict           ← Dict réduit catalogue (généré, dans Git)
│       ├── sys/
│       │   ├── rz_sys_monitor.sh     ← Surveillance CPU/temp/batt/mem/stockage
│       │   └── lib/
│       │       ├── urgence.sh        ← Fonctions déclenchement Urg
│       │       └── alerts_sys.sh     ← Logique alertes seuils
│       ├── rz_gyro_manager.sh        ← Manager gyroscope SE (termux-sensor)
│       ├── rz_mg_manager.sh          ← Manager magnétomètre SE
│       ├── rz_microSE_manager.sh     ← Manager microphone + balayage orientation
│       ├── rz_balayage_sonore.sh     ← Algorithme balayage servo TGD
│       ├── rz_appli_manager.sh       ← Gestionnaire apps Android via Tasker
│       └── rz_torch_manager.sh       ← Contrôle torche (natif ou via IP Webcam)
│
├── utils/
│   ├── logger.sh                   ← Journalisation centralisée (niveaux debug/info/warn/erreur)
│   ├── error_handler.sh            ← Gestion Urg et COM (fonctions handle_urgence/handle_message)
│   └── fifo_manager.sh             ← Création/vérification des FIFOs
│
├── fifo/                           ← Files de communication inter-processus
│   ├── fifo_termux_in              ← FIFO principale : messages entrants vers managers
│   ├── fifo_tasker_in              ← FIFO Tasker : commandes Android
│   └── fifo_return                 ← FIFO retour : ACKs et réponses
│
├── logs/                           ← Journaux d'exécution
│   ├── main.log                    ← Log général
│   ├── error.log                   ← Erreurs
│   ├── vpiv_parser.log             ← Log du parser VPIV
│   ├── state_manager.log           ← Log du gestionnaire d'état
│   ├── sys_monitor.log             ← Log surveillance système
│   ├── camera.log                  ← Log caméra
│   ├── stt_handler.log             ← Log handler STT
│   ├── ai_conversation.log         ← Log Gemini
│   ├── emergency.log               ← Log arrêts d'urgence
│   └── mqtt_bridge.log             ← Log bridge MQTT/Série
│
└── mqtt_bridge.py                  ← Bridge port série Arduino ↔ MQTT
```

**Fichiers de configuration source (hors arborescence runtime) :**
```
~/scripts_RZ_SE/termux/config/
  ├── courant_init.sh               ← Source dev (génère courant_init.json)
  ├── original_init.sh              ← Source production (génère tout au démarrage)
  └── check_config.sh               ← Validation et distribution de la config
```

---

## 2.2 Tableau des scripts et fichiers

| Script / Fichier | Catégorie | Rôle synthétique | Lit | Écrit | Déclenché par |
|-----------------|-----------|-----------------|-----|-------|---------------|
| original_init.sh | Config | Init production complète | — | courant_init.json, global.json, catalogue CSV | Manuel (1 fois) |
| courant_init.sh | Config | Init développement | — | courant_init.json | Manuel (dev) |
| check_config.sh | Config | Validation + distribution | courant_init.json | *_runtime.json, *_config.json | original_init, courant_init |
| rz_vpiv_parser.sh | Core | Aiguillage MQTT entrant | global.json | FIFO_TERMUX | MQTT SE/commande |
| rz_state-manager.sh | Core | Gestion modeRZ/typePtge/STT | global.json | global.json, MQTT | FIFO_TERMUX |
| safety_stop.sh | Core | Arrêt d'urgence | — | FIFO_TERMUX, emergency.log | rz_vpiv_parser (danger) |
| rz_gyro_manager.sh | Sensor | Flux gyroscope | gyro_runtime.json | MQTT SE/statut | Démarrage SE |
| rz_mg_manager.sh | Sensor | Flux magnétomètre | mag_runtime.json | MQTT SE/statut | Démarrage SE |
| rz_sys_monitor.sh | Sensor | Surveillance HW | sys_runtime.json | global.json, FIFO_TERMUX | Démarrage SE |
| rz_camera_manager.sh | Sensor | Pilotage caméra | cam_config.json | cam_config.json, FIFO | rz_vpiv_parser |
| rz_microSE_manager.sh | Sensor | Micro + orientation | mic_config.json | MQTT | Démarrage SE |
| rz_appli_manager.sh | Appli | Apps Android via Tasker | appli_config.json | appli_config.json | rz_vpiv_parser |
| rz_torch_manager.sh | Sensor | Torche | cam_config.json | FIFO | rz_vpiv_parser |
| rz_stt_manager.sh | STT | Wake word PocketSphinx | stt_config.json, fr.dict | — | Démarrage SE |
| rz_stt_handler.sh | STT | Catalogue → VPIV | stt_catalog.json, global.json, courant_init.json | FIFO | rz_stt_manager |
| rz_ai_conversation.py | STT | Repli Gemini | keys.json | ai_conversation.log | rz_stt_handler (exit 1) |
| mqtt_bridge.py | Bridge | Série ↔ MQTT | — | MQTT SE/arduino | Démarrage SE |
| global.json | Config | État dynamique partagé | (tous) | rz_state-manager, rz_sys_monitor | original_init.sh |

---

## 2.3 Schémas de fonctionnement

### Schéma 1 — Installation et initialisation

```
[Installation initiale]
  ↓
original_init.sh
  ├─ mkdir -p : crée l'arborescence complète
  ├─ Vérification PocketSphinx (model-fr/, lexique_referent.dict)
  ├─ Génère courant_init.json  ←── variables définies dans le script
  ├─ Génère global.json        ←── état initial sûr (modeRZ=1 VEILLE)
  ├─ Génère table_d_catalogue.csv  ←── section balisée CATALOGUE STT
  ├─ python3 rz_build_system.py  →  stt_catalog.json + rz_words.txt
  ├─ python3 rz_build_dict.py    →  fr.dict
  └─ bash check_config.sh
       ├─ Valide gyro     → gyro_runtime.json
       ├─ Valide mag      → mag_runtime.json
       ├─ Valide sys      → sys_runtime.json
       ├─ Valide cam      → cam_config.json
       ├─ Valide mic      → mic_config.json
       ├─ Valide appli    → appli_config.json
       └─ Valide STT      → stt_config.json
                            + vérification fichiers PocketSphinx
```

### Schéma 2 — Boucle générale d'un manager

```
Démarrage manager
  │
  ▼
Lire *_runtime.json (config validée)
  │
  ▼
Boucle principale
  ├─ Vérifier mode (OFF → sleep et continuer)
  ├─ Acquérir données (termux-sensor / capteur HW)
  ├─ Appliquer filtres (seuilMesure, delta)
  ├─ Comparer aux seuils d'alerte
  │   ├─ Seuil warn dépassé → $I:COM:warn:SE:"message"# vers FIFO
  │   └─ Seuil urg dépassé  → $E:Urg:source:SE:URG_XXX# vers FIFO
  ├─ Publier données → $F:Module:prop:inst:valeur# vers MQTT
  └─ sleep(frequence) → recommencer
```

### Schéma 3 — Circulation d'une commande utilisateur

```
[Utilisateur interface Node-RED]
  │ clique ou saisit une commande
  ▼
SP (Node-RED)
  │ construit trame VPIV : $V:CfgS:modeRZ:*:3#
  │ publie sur MQTT topic SE/commande
  ▼
MQTT broker (DuckDNS)
  ▼
rz_vpiv_parser.sh
  │ validate_vpiv() → parse CAT/VAR/PROP/INST/VAL
  │
  ├─ VAR=Urg + CAT=E + VAL=danger ?
  │   └─► safety_stop.sh (priorité absolue)
  │
  ├─ CAT=V/F + VAR=CamSE → rz_camera_manager.sh
  ├─ CAT=V/F + VAR=CfgS  → FIFO_TERMUX_IN
  ├─ CAT=A + VAR=Appli/Zoom/TTS → FIFO_TASKER_IN
  └─ autres → FIFO_TERMUX_IN
           │
           ▼
        rz_state-manager.sh (lit FIFO_TERMUX_IN)
           │ met à jour global.json
           │ calcule STT.context (Inactif/Cmde/Conv/Mixte)
           └─ publie ACK $I:CfgS:modeRZ:*:3# → MQTT SE/statut → SP
```

### Schéma 4 — Parcours STT (commande vocale)

```
[Utilisateur parle]
  │ "RZ avance"
  ▼
Microphone Android
  ▼
rz_stt_manager.sh  (boucle PocketSphinx KWS)
  │ wake word "rz" détecté
  │ écoute commande complémentaire
  │ "avance" reconnu
  ▼
rz_stt_handler.sh "avance"
  │
  ├─ Étape A : lire global.json (cpu_load, modeRZ, typePtge)
  ├─ Étape B : CPU > 90% ? → abandon, TTS "Je suis trop chargé"
  ├─ Étape C : chercher "avance" dans stt_catalog.json → MTR_FORWARD
  ├─ Étape D : modeRZ=0 ou 5 ? → bloqué (sauf URGENCY_STOP)
  ├─ Étape E : typePtge=4 + moteurActif=1 ? → refus laisse
  └─ Étape F : Traitement = SIMPLE
               construire capsule : $A:STT:intent:*:{V:Mtr:cmd:*:50,0,2}#
               → écrire dans FIFO_TERMUX_IN
               → TTS : "Je fonce !" (Alias-TTS aléatoire)
                         │
                         ▼
                rz_state-manager.sh
                  │ extrait inner VPIV : V:Mtr:cmd:*:50,0,2
                  │ valide contre modeRZ courant
                  └─ publie $V:Mtr:cmd:*:50,0,2# → MQTT → SP → Arduino

[Si commande non reconnue : exit 1]
  ▼
rz_ai_conversation.py "rz avance"
  │ appel API Gemini
  └─ réponse TTS : "Désolé, je n'ai pas compris cette commande."
```

---

# PARTIE 3 — MODULES SE

## 3.1 Module Gyro — Gyroscope

### Objectifs

Mesurer les rotations angulaires du robot sur 3 axes (x, y, z) et détecter les situations de chute ou d'inclinaison anormale. Fournir à SP un flux de données calibré et filtré.

### Architecture

Le gyroscope est lu par `termux-sensor` (capteur Android natif). Le manager `rz_gyro_manager.sh` tourne en boucle continue et publie selon le mode configuré.

### Propriétés (depuis gyro_runtime.json)

| Propriété | Type | Description |
|-----------|------|-------------|
| modeGyro | enum | OFF \| DATACont \| DATAMoy \| ALERTE |
| angleVSEBase | number (deg×10) | Angle BASE+THB de correction tangage |
| freqCont | number (Hz) | Fréquence flux continu |
| freqMoy | number (Hz) | Fréquence flux moyenné |
| nbValPourMoy | number | Mesures avant calcul moyenne |
| seuilMesure {x,y,z} | number (rad/s×100) | Filtrage bruit |
| seuilAlerte {x,y} | number (rad/s×100) | Seuils déclenchement alerte chute |

### Comportement par mode

- **OFF** : manager inactif, aucun flux publié
- **DATACont** : publication `$F:Gyro:dataCont:*:[x,y,z]#` à chaque mesure (filtrée par seuilMesure)
- **DATAMoy** : accumulation de `nbValPourMoy` mesures puis publication de la moyenne
- **ALERTE** : surveillance seuilAlerte uniquement, déclenchement `$E:Urg:source:SE:URG_MOTOR_STALL#` si dépassé

### Fichiers clés

- **Config source :** `courant_init.json` → section `gyro`
- **Config opérationnelle :** `config/gyro_runtime.json` (généré par check_config.sh)
- **Manager :** `scripts/sensors/rz_gyro_manager.sh`

### Articulation

`check_config.sh` valide la cohérence (seuilAlerte > seuilMesure recommandé) et génère `gyro_runtime.json`. Le manager lit ce fichier au démarrage et réévalue dynamiquement si le mode change.

### Règles et contrôles

- `freqCont > 0` et `freqMoy > 0` (erreur fatale si ≤ 0)
- `nbValPourMoy > 0`
- `seuilAlerte.x` et `.y` > 0
- Unités en `rad/s×100` (entiers, pas de float)

### Précautions

Calibrer le capteur Android avant utilisation terrain. Les données brutes de `termux-sensor` peuvent dériver.

### Résumé (5 lignes)

Le module Gyro mesure les rotations 3 axes du robot via le capteur Android. Il opère en 4 modes : inactif, flux continu, flux moyenné, ou surveillance seuils uniquement. La configuration est distribuée dans `gyro_runtime.json` par `check_config.sh`. Les seuils d'alerte permettent la détection de chute ou d'inclinaison anormale. Les données sont publiées vers SP via MQTT en trames `$F:Gyro:dataCont`.

---

## 3.2 Module Mag — Magnétomètre

### Objectifs

Mesurer le champ magnétique et calculer le cap (magnétique ou géographique) du robot. Permettre l'orientation relative et la navigation.

### Architecture

Lecture via `termux-sensor` (capteur Android `MagneticField`). Manager `rz_mg_manager.sh`.

### Propriétés (depuis mag_runtime.json)

| Propriété | Type | Description |
|-----------|------|-------------|
| modeMg | enum | OFF \| DATABrutCont \| DATAGeoCont \| DATAGeoMoy \| DATAMgCont \| DATAMgMoy |
| freqCont | number (Hz) | Fréquence flux continu |
| freqMoy | number (Hz) | Fréquence flux moyenné |
| nbValPourMoy | number | Mesures pour moyenne cap |
| seuilMesure {x,y,z} | number (µT×100) | Filtrage bruit magnétique |

### Comportement par mode

- **DATABrutCont** : publication `$F:Mag:dataBrut:*:[x,y,z]#` en µT×100
- **DATAMgCont** : cap magnétique continu `$F:Mag:dataMg:*:CAP#` en degrés×10
- **DATAGeoCont** : cap géographique (avec correction déclinaison) `$F:Mag:dataGeo:*:CAP#`
- Modes **Moy** : version moyennée des flux correspondants

### Fichiers clés

- `courant_init.json` → section `mag`
- `config/mag_runtime.json`
- `scripts/sensors/rz_mg_manager.sh`

### Résumé (5 lignes)

Le module Mag mesure le champ magnétique sur 3 axes pour calculer l'orientation du robot. Six modes permettent de choisir entre flux bruts, cap magnétique ou géographique, en continu ou moyenné. La configuration est dans `mag_runtime.json`. Les seuils de mesure filtrent le bruit de fond. Les données sont publiées en degrés×10 (entiers) vers SP via MQTT.

---

## 3.3 Module Sys — Surveillance système

### Objectifs

Surveiller en continu les métriques hardware du smartphone SE (CPU, température, stockage, RAM, batterie). Mettre à jour `global.json`, déclencher les alertes et urgences selon les seuils configurés.

### Architecture

Manager `rz_sys_monitor.sh` tourne en boucle permanente. Il lit la configuration depuis `sys_runtime.json`, acquiert les métriques, met à jour `global.json` et envoie les trames VPIV d'alerte si nécessaire.

### Propriétés (depuis sys_runtime.json)

| Propriété | Description |
|-----------|-------------|
| modeSys | OFF \| FLOW \| MOY \| ALERTE |
| freqCont | Période flux continu (secondes) |
| freqMoy | Période flux moyenné (secondes, ≥ freqCont) |
| nbCycles | Mesures accumulées avant publication moyenne |
| thresholds.cpu.warn/urg | % CPU — alerte si dépassé |
| thresholds.thermal.warn/urg | °C — alerte si dépassé |
| thresholds.storage.warn/urg | % stockage — alerte si dépassé |
| thresholds.mem.warn/urg | % RAM — alerte si dépassé |
| thresholds.batt.warn/urg | % batterie — alerte si INFÉRIEUR (sens inversé) |

### Comportement des alertes

Pour chaque métrique (sauf batterie) :
- Valeur > `warn` → `$I:COM:warn:SE:"Surcharge X détectée"#`
- Valeur > `urg` → `$E:Urg:source:SE:URG_XXX#`

Pour la batterie (sens inversé) :
- Valeur < `warn` → `$I:COM:warn:SE:"Batterie faible"#`
- Valeur < `urg` → `$E:Urg:source:SE:URG_LOW_BAT#`

### Fichiers clés

- `courant_init.json` → section `sys`
- `config/sys_runtime.json`
- `scripts/sensors/sys/rz_sys_monitor.sh`
- `scripts/sensors/sys/lib/urgence.sh` et `alerts_sys.sh`

### Règles et contrôles

- `freqMoy ≥ freqCont` (erreur check_config si inférieur)
- `urg > warn` pour cpu, thermal, storage, mem
- `urg < warn` pour batt (contrainte inversée — erreur check_config si non respectée)
- Toutes les valeurs en entiers (pas de float)

### Précautions

Les valeurs de seuils doivent être adaptées au smartphone utilisé. Un CPU à 90% peut être normal sur certains appareils. La température de 85°C est une valeur haute à ajuster selon le téléphone.

### Résumé (5 lignes)

Le module Sys surveille en continu les 5 métriques hardware du smartphone SE. Il met à jour `global.json` à chaque mesure pour informer les autres managers de l'état du système. Les seuils warn/urg déclenchent respectivement des messages COM informatifs ou des urgences VPIV vers SP. La batterie fonctionne en sens inversé (alerte si trop basse). La configuration est distribuée dans `sys_runtime.json` par `check_config.sh`.

---

## 3.4 Module Cam — Caméra

### Objectifs

Piloter la caméra du smartphone SE via l'application IP Webcam (streaming HTTP). Gérer les deux instances (avant/arrière), les profils de qualité selon la charge CPU, et les snapshots.

### Architecture

Manager `rz_camera_manager.sh`. Utilise `am start` (Android Intent) pour démarrer IP Webcam, `curl` pour les commandes HTTP (lumière), `ifconfig` pour récupérer l'IP locale. Deux instances : `rear` (caméra dos) et `front` (caméra frontale).

### Propriétés (depuis cam_config.json)

| Propriété | Description |
|-----------|-------------|
| front/rear.res | Résolution (low, medium, high, 720p, 1080p) |
| front/rear.fps | Images par seconde |
| front/rear.quality | Compression JPEG 0-100 |
| front/rear.urgDelay | Délai avant URG si perte cam (secondes) |
| status.current_side | Instance active (rear/front) |
| profiles.normal | Paramètres qualité CPU ≤ 70% |
| profiles.optimized | Paramètres qualité CPU > 70% |
| locks.zoom_active | Verrou : zoom en cours |
| locks.security_app_active | Verrou : app sécurité active |

### Commandes VPIV acceptées

| Commande | Effet |
|----------|-------|
| `$V:CamSE:active:rear:On#` | Démarre IP Webcam caméra arrière |
| `$V:CamSE:active:*:Off#` | Arrête IP Webcam |
| `$V:CamSE:mode:*:front#` | Bascule caméra frontale |
| `$V:CamSE:(snap):*:1#` | Prend un snapshot (URL retournée en ACK) |

### Comportement

Au démarrage de la caméra :
1. Vérifie la charge CPU → choisit le profil (normal ou optimized)
2. Envoie notification système (CAMERA_START_PRIORITY)
3. Lance IP Webcam via `am start` avec intent personnalisé
4. Récupère l'IP Wi-Fi locale
5. Publie URL flux : `$F:CamSE:streamURL:rear:{http://IP:8080/video}#`
6. Publie état : `$I:CamSE:active:rear:On#`
7. Met à jour `cam_config.json` (status)

### Articulation avec la torche

Si la caméra est active (`status.active = "On"`), la torche est contrôlée via l'API HTTP IP Webcam (`/enablels`, `/disablels`). Sinon, utilisation de `termux-torch`.

### Fichiers clés

- `config/sensors/cam_config.json`
- `scripts/sensors/cam/rz_camera_manager.sh`
- `scripts/sensors/rz_torch_manager.sh`

### Résumé (5 lignes)

Le module Cam pilote l'application IP Webcam pour le streaming vidéo du robot. Il gère deux instances (avant/arrière) avec sélection automatique du profil de qualité selon la charge CPU. Les commandes VPIV permettent le démarrage, l'arrêt, le changement de face et la capture snapshot. L'URL du flux est publiée vers SP dès le démarrage pour affichage dans l'interface. La torche est intégrée au module et bascule automatiquement entre contrôle natif et contrôle HTTP selon l'état de la caméra.

---

## 3.5 Module Mic — Microphone SE

### Objectifs

Analyser le niveau sonore ambiant via le microphone du smartphone SE. Détecter les niveaux d'intensité (faible/moyen/fort), localiser la direction de la source sonore par balayage du servo TGD (tête gauche-droite).

### Architecture

Manager `rz_microSE_manager.sh` avec module spécialisé `rz_balayage_sonore.sh`. Acquisition via enregistrement WAV court (fenêtre glissante), calcul RMS par Python/sox.

### Propriétés (depuis mic_config.json)

| Propriété | Description |
|-----------|-------------|
| modeMicro | off \| normal \| intensite \| orientation \| surveillance |
| freqCont | Période de mesure (ms) |
| winSize | Durée fenêtre d'enregistrement (ms, ≥ freqCont) |
| seuilDelta | Variation RMS min pour envoi (filtre flux) |
| holdTime | Durée maintien seuil avant déclenchement alerte (ms) |
| seuilFaible/Moyen/Fort | Niveaux d'intensité (RMS×10, 0-1000) |
| pasBalayage | Pas du servo TGD par étape (degrés) |
| timeoutOrientation | Durée max balayage orientation (secondes) |

### Comportement par mode

- **normal** : publication `$F:MicroSE:dataContRMS:*:VAL#` à chaque mesure
- **intensite** : publication si delta > seuilDelta, alerte si seuil franchi pendant holdTime ms
- **orientation** : balayage servo TGD de -90° à +90°, publication angle de RMS max `$F:MicroSE:dataOrient:*:ANGLE#`
- **surveillance** : intensite + balayage automatique si RMS > seuilMoyen

### Contrainte fenêtre glissante

`NB_FENETRE = winSize / freqCont` doit être ≥ 2.  
Contrainte check_config : `winSize ≥ freqCont`.

### Durée estimée du balayage

```
nb_positions = 180 / pasBalayage + 1
durée ≈ nb_positions × (200ms stabilisation + winSize)
```
check_config avertit si la durée dépasse `timeoutOrientation`.

### Résumé (5 lignes)

Le module Mic analyse le son ambiant via le microphone SE en mesurant le niveau RMS sur une fenêtre glissante. Cinq modes permettent d'aller du simple flux à la localisation directionnelle par balayage du servo tête. Les seuils d'intensité à 3 niveaux (faible/moyen/fort) permettent des réponses différenciées. Le balayage rotatif du servo TGD identifie la direction de la source sonore dominante. La configuration est validée et distribuée dans `mic_config.json` par check_config.sh.

---

## 3.6 Module STT/TTS — Reconnaissance et synthèse vocale

> Ce module est décrit en détail dans les **Documents B1** (guide utilisateur) et **B2** (guide développeur).

### Objectifs

Permettre à l'utilisateur de contrôler le robot par la voix. Répondre vocalement via synthèse TTS. Basculer vers une IA cloud (Gemini) pour la conversation libre.

### Architecture

```
PocketSphinx (KWS)
  → rz_stt_manager.sh (boucle wake word "rz")
  → rz_stt_handler.sh (catalogue → VPIV)
       → SIMPLE/COMPLEXE/PLAN/LOCAL
       → FIFO → rz_state-manager → MQTT → SP → Arduino
  → Si non reconnu (exit 1) : rz_ai_conversation.py (Gemini)
TTS : termux-tts-speak (partout)
```

### Sources TTS

| Source | Déclencheur | Texte |
|--------|-------------|-------|
| Catalogue STT | Commande reconnue | Alias-TTS de la commande |
| Trames COM SP | `$I:COM:info/warn:SE:"msg"#` | Message SP |
| Urgences | `$E:Urg:*` avec certains codes | Message prédéfini (ex: "Batterie critique") |

### Configuration

- `config/sensors/stt_config.json` : paramètres PocketSphinx
- `scripts/sensors/stt/stt_catalog.json` : catalogue compilé
- `scripts/sensors/stt/lib_pocketsphinx/fr.dict` : dictionnaire phonétique

### Résumé (5 lignes)

Le module STT reconnaît les commandes vocales via PocketSphinx (wake word "rz" + commande). Le handler consulte le catalogue pour construire et envoyer la trame VPIV correspondante. Les commandes non reconnues basculent vers l'IA Gemini pour la conversation libre. Le TTS (termux-tts-speak) assure les retours vocaux du robot depuis trois sources : catalogue, messages SP, urgences. Le pilotage vocal est l'une des 5 modalités de pilotage (typePtge=1 ou 4).

---

## 3.7 Module Appli — Gestionnaire d'applications

### Objectifs

Démarrer, arrêter et surveiller les applications Android tierces (Tasker, Zoom, Baby, BabyMonitor, NavGPS) depuis les commandes VPIV de SP.

### Architecture

Manager `rz_appli_manager.sh`. Utilise `rz_tasker_bridge.sh` pour déclencher les tâches Tasker. Fallback via `am start` si Tasker indisponible. Surveille l'état des apps dans `appli_config.json`.

### Propriétés (depuis appli_config.json)

| Application | Tâche Tasker | Package Android | Dépendances |
|-------------|-------------|-----------------|-------------|
| Baby | RZ_Baby | — | Tasker requis |
| tasker | — | net.dinglisch.android.taskerm | — |
| zoom | RZ_Zoom | us.zoom.videomeetings | Tasker recommandé |
| BabyMonitor | RZ_BabyMonitor | — | Tasker requis |
| NavGPS | RZ_NavGPS | com.google.android.apps.maps | Tasker recommandé |

**ExprTasker :** tâches spéciales pour les expressions faciales (`RZ_Expression`) et messages info (`RZ_Info`).

### Comportement

1. Réception `$A:Appli:NomApp:*:On#` → vérifie Tasker actif → déclenche tâche ou `am start`
2. Mise à jour `appli_config.json` (state, last_change)
3. Timeout configurable (`bridge_timeout` secondes) si Tasker ne répond pas
4. ACK retourné : `$I:Appli:NomApp:*:On#`

### Précaution

Tasker doit être lancé manuellement ou automatiquement au boot Android avant tout autre gestionnaire d'app. Les tâches Tasker doivent porter exactement les noms définis dans `appli_config.json`.

### Résumé (5 lignes)

Le module Appli gère le cycle de vie des applications Android tierces via Tasker ou les intents Android directs. Il maintient l'état de chaque application dans `appli_config.json`. Les commandes de pilotage vocal (APP_ZOOM_ON, APP_GPS_ON...) passent par ce module. Tasker est le mécanisme préféré car il gère les intents complexes et les configurations Android. Un timeout protège contre l'absence de Tasker.

---

## 3.8 Torche

> Module simple, non autonome — intégré au module Cam.

### Rôle

Contrôler le flash LED du smartphone SE depuis les commandes VPIV `$V:TorchSE:active:*:On#`.

### Comportement

- **Caméra inactive** : `termux-torch on/off`
- **Caméra active (IP Webcam)** : commandes HTTP `curl http://IP:8080/enablels` ou `disablels`

Le choix automatique évite les conflits entre IP Webcam et le contrôle natif de la torche.

**Script :** `scripts/sensors/rz_torch_manager.sh`

---

## 3.9 State Manager — Gestionnaire d'état

### Objectifs

Maintenir la cohérence de `global.json`. Gérer les transitions de mode (`modeRZ`, `typePtge`). Calculer le contexte STT selon l'état courant.

### Architecture

`rz_state-manager.sh` écoute MQTT topic `SE/commande` en continu et le FIFO_TERMUX_IN. Il est le seul processus autorisé à écrire dans `global.json.CfgS`.

### Calcul du contexte STT

| modeRZ | typePtge | speed | Contexte |
|--------|----------|-------|---------|
| 0 ou 5 | — | — | Inactif |
| any | ≠1 et ≠4 | — | Inactif |
| any | 1 ou 4 | ≠0 | Cmde (robot en mouvement) |
| 1 (VEILLE) | 1 ou 4 | 0 | Conv (conversation IA) |
| 4 (AUTONOME) | 1 ou 4 | 0 | Mixte |
| 2 ou 3 | 1 ou 4 | 0 | Mixte |

Le contexte est écrit dans `global.json` et lu par `rz_stt_manager` pour adapter le comportement de reconnaissance.

### Résumé (5 lignes)

Le State Manager est le gardien de la cohérence du système SE. Il gère les transitions de modes modeRZ et typePtge, met à jour global.json de façon atomique. Il calcule le contexte STT (Inactif/Cmde/Conv/Mixte) à chaque changement d'état pour adapter la reconnaissance vocale. Il notifie SP via MQTT à chaque changement pour synchronisation de l'interface. C'est le seul processus autorisé à écrire dans le bloc CfgS de global.json.

---

## 3.10 VPIV Parser — Point d'entrée des commandes

### Objectifs

Recevoir et analyser tous les messages VPIV arrivant depuis SP via MQTT. Aiguiller vers les bons managers ou FIFOs. Gérer les urgences prioritaires et les retours vocaux.

### Architecture

`rz_vpiv_parser.sh` écoute en boucle `mosquitto_sub` sur `SE/commande`. Pour chaque message valide, il parse la trame et prend une décision d'aiguillage.

### Logique d'aiguillage

| VAR | CAT | Destination |
|-----|-----|-------------|
| Urg + E + danger | — | safety_stop.sh (priorité absolue) |
| CamSE | V/F | rz_camera_manager.sh |
| Appli/Zoom/TTS | V/F/A | FIFO_TASKER_IN |
| CfgS | V/F | FIFO_TERMUX_IN |
| autres | V/F | FIFO_TERMUX_IN |
| Voice.tts | A | termux-tts-speak direct |
| COM | I/E | FIFO_TERMUX_IN |

### Retours vocaux

Si `typePtge ≠ 0` (pilotage avec retour vocal actif) :
- `COM.error` → TTS "Erreur : message"
- `COM.info` → TTS "message"
- URG_LOW_BAT → TTS "Batterie critique"
- URG_CPU_CHARGE → TTS "Surcharge processeur"

### Résumé (5 lignes)

Le VPIV Parser est le point d'entrée unique de toutes les commandes SP arrivant sur SE. Il valide la structure des trames, les parse et décide de l'aiguillage selon la variable et la catégorie. Les urgences de niveau danger déclenchent immédiatement safety_stop.sh avant tout autre traitement. Les retours vocaux (TTS) sont gérés directement pour les messages COM et urgences. Un ACK est systématiquement retourné à SP pour chaque trame traitée.

---

## 3.11 Sécurité — safety_stop et error_handler

### safety_stop.sh

Déclenché par `rz_vpiv_parser.sh` sur réception de `$E:Urg:etat:SE:danger#`.

**Actions immédiates et inconditionnelles :**
1. `$V:MotA:state:*:Stop#` → arrêt moteur
2. `$V:CamSE:active:*:Off#` → coupure caméra
3. `$V:TorchSE:power:*:0#` → extinction torche
4. Message erreur COM vers FIFO
5. Écriture dans `logs/emergency.log`

### error_handler.sh

Bibliothèque de fonctions centralisées :

```bash
# Déclenchement urgence (réaction automatique attendue de SP)
handle_urgence "URG_OVERHEAT" "danger" "Température CPU: 85°C"
# Publie : $E:Urg:source:SE:URG_OVERHEAT#
#          $V:Urg:statut:*:active#
#          $E:Urg:niveau:SE:danger#
#          $E:Urg:raison:SE:Température CPU: 85°C#

# Message informatif (pas de réaction automatique)
handle_message "warn" "Espace disque faible"
# Publie : $I:Com:warn:SE:Espace disque faible#
```

### Résumé (5 lignes)

La couche sécurité du SE est composée de deux éléments. safety_stop.sh réalise l'arrêt d'urgence immédiat (moteurs, caméra, torche) sur ordre danger de SP. error_handler.sh fournit les fonctions standardisées pour publier les urgences (Urg, CAT=E) et les messages informatifs (COM, CAT=I). La distinction Urg/COM est fondamentale : Urg déclenche une réaction automatique SP, COM est informationnel. Tout arrêt d'urgence est tracé dans emergency.log.

---

*Fin du Document de Référence SE — Projet RZ*  
*Version 1.0 — Mars 2026 — Vincent Philippe*
