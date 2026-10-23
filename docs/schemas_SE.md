# Les trois schémas du SE, redessinés depuis le code

> Ces schémas remplacent `SE_diagram`, `Termux_STT_Intent_Routing_Flow` et
> `VPIV_Termux_Sensor_Integration_Architecture`, datés du 23/10/2026 au matin.
> **La structure des originaux était bonne** — c'est ce qui a permis de les refaire vite.
> Ce sont les étiquettes qui avaient vieilli : six trames VPIV sur sept étaient fausses,
> et trois variables n'existaient pas.
>
> Chaque schéma a été **rendu par Mermaid** avant d'être livré, pas seulement écrit.
> Sources : `docs/schemas/*.mmd`, réouvrables dans l'éditeur Mermaid.
>
> *V1 — 23/10/2026 — Vincent Philippe / Claude*

---

## 1. Architecture du SE — le circuit d'une trame

```mermaid
flowchart TB
    subgraph SP["SP — Node-RED, Raspberry Pi 3 — maitre absolu"]
        MOS["Mosquitto"]
        NR["Flux Node-RED"]
    end

    subgraph DORO["SE — Doro sous Termux"]
        PARSER["rz_vpiv_parser.sh<br/>SEULE passerelle MQTT du SE"]
        FT["fifo_termux_in<br/>sortie unique de tous les modules"]
        FA["fifo_appli_in"]
        FK["fifo_tasker_in"]
        FV["fifo_voice_in"]
        FR["fifo_return<br/>creee, aucun lecteur trouve"]
        MA["rz_appli_manager.sh<br/>verrous camera et micro"]
        MV["rz_voice_manager.sh<br/>Voice tts vol output ttsRate"]
        MT["rz_tasker_bridge.sh"]
        CAPT["managers capteurs<br/>gyro · Mg · microSE · sys · torch · cam · STT"]
        TOUT["rz_tasker_out.sh<br/>publie SE/statut"]
        BRIDGE["mqtt_bridge.py<br/>relais Arduino, independant"]
        CFG[("config/global.json<br/>config/courant_init.json")]
        LOG[("logs/")]
    end

    TASKER["Tasker + AutoVoice<br/>via Termux:Tasker"]
    MEGA["Arduino Mega<br/>/dev/ttyACM0 @ 115200"]

    NR --- MOS
    MOS -->|"SE/commande"| PARSER
    PARSER -->|"SE/statut"| MOS
    TOUT -->|"SE/statut"| MOS

    PARSER --> FA
    PARSER --> FK
    PARSER --> FV
    PARSER -.->|"creee au demarrage"| FR

    FA --> MA
    FK --> MT
    FV --> MV
    MT --> TASKER
    TASKER --> TOUT

    MA --> FT
    MV --> FT
    CAPT --> FT
    FT --> PARSER

    MOS <-->|"SE/arduino<br/>SE/arduino/data"| BRIDGE
    BRIDGE <-->|"USB serie · VPIV en texte"| MEGA

    CFG -.-> MA
    CFG -.-> MV
    CFG -.-> CAPT
    PARSER -.-> LOG
    CAPT -.-> LOG

    classDef passerelle fill:#f9f,stroke:#333,color:#000
    classDef fifo fill:#bbf,stroke:#333,color:#000
    classDef manager fill:#ff9,stroke:#333,color:#000
    classDef fichier fill:#ddd,stroke:#333,color:#000
    classDef externe fill:#f96,stroke:#333,color:#000
    class PARSER,BRIDGE passerelle
    class FT,FA,FK,FV,FR fifo
    class MA,MV,MT,CAPT,TOUT manager
    class CFG,LOG fichier
    class TASKER,MEGA,MOS,NR externe
```

### Ce que ce schéma corrige

| L'ancien disait | La réalité |
|---|---|
| trois FIFO | **cinq** : `fifo_termux_in`, `fifo_tasker_in`, `fifo_appli_in`, `fifo_voice_in`, `fifo_return` *(liste de `rz_start.sh` l.264-269)* |
| `/sdcard/rz_vars.json` | `config/global.json` et `config/courant_init.json`, sous `$BASE_DIR` |
| `/sdcard/rz_logs/` | `$BASE_DIR/logs` |
| « Décodeur » et « Encodeur » séparés | **un seul** `rz_vpiv_parser.sh`, qui fait les deux et reste la **seule** passerelle MQTT du SE |
| la caméra parle directement à SP | elle passe par `rz_camera_manager.sh`, comme les autres |

### Et ce qu'il ajoute

**Le relais Arduino est un chemin indépendant.** `mqtt_bridge.py` a ses propres topics
(`SE/arduino` et `SE/arduino/data`) et ne passe pas par le parser. Le Doro est, pour le
trafic Arduino, un simple relais entre MQTT et le port série — il ne l'interprète pas.

**`fifo_return` est créée au démarrage mais aucun script du dépôt ne la lit.** Soit un
lecteur manque, soit la FIFO ne sert plus. À trancher.

**`fifo.manager.sh` est périmé** : il ne liste que quatre FIFO, il oublie `fifo_voice_in`.
C'est `rz_start.sh` qui fait foi.

---

## 2. Routage d'une commande vocale

```mermaid
flowchart TB
    MIC["Microphone du Doro"]
    STTENG["Moteur STT<br/>rz_stt_manager.py"]
    HANDLER["rz_stt_handler.sh<br/>coeur decisionnel"]
    CAT[("stt_catalog.json<br/>Table D operationnelle<br/>copiee de tableD_catalogueV2_corrige.csv")]
    FILTRE{"Filtres de securite<br/>CPU · modeRZ · laisse"}
    CTX{"CfgS.typePtge<br/>vaut-il 1 ou 4 ?"}
    TRAIT{"Colonne Traitement<br/>de la ligne trouvee"}
    LOCAL["Resolu dans le SE<br/>SIMPLE · LocalSE"]
    BRUT["Transmis BRUT a SP<br/>TRADUCTION · CALCUL · MACRO"]
    FIFO["fifo_termux_in"]
    SP["SP — fn_stt_intent_router.js<br/>decapsule et resout via ui_Traitement"]
    ACTION["Action : Mtr · Srv · Lring · Appli ..."]
    IA["rz_ai_conversation.py<br/>bascule IA, reponse vocale"]
    TTS["rz_voice_manager.sh<br/>$A:Voice:tts:*:texte#"]

    MIC --> STTENG
    STTENG -->|"texte reconnu"| HANDLER
    CAT -.->|"recherche de la commande"| HANDLER
    HANDLER --> FILTRE
    FILTRE -->|"refuse"| REFUS["$E:Urg:source:SE:...#<br/>via error_handler.sh"]
    FILTRE -->|"accepte"| CTX
    CTX -->|"non : pilotage vocal inactif"| INACTIF["$I:STT:context:SE:Inactif#"]
    CTX -->|"oui"| TRAIT

    TRAIT --> LOCAL
    TRAIT --> BRUT
    LOCAL --> FIFO
    BRUT --> FIFO
    FIFO -->|"$A:STT:intent:id_requete:{id_traitement:cat:var:prop:inst:val}#"| SP
    SP --> ACTION
    ACTION -->|"accuse"| RETOUR["$I:VAR:PROP:INST:valeur#"]

    HANDLER -->|"commande introuvable — code retour 1"| IA
    IA --> TTS

    classDef decision fill:#ff9,stroke:#333,color:#000
    classDef sp fill:#f9f,stroke:#333,color:#000
    classDef erreur fill:#f99,stroke:#333,color:#000
    classDef fichier fill:#bbf,stroke:#333,color:#000
    class FILTRE,CTX,TRAIT decision
    class SP,ACTION sp
    class REFUS,INACTIF erreur
    class CAT,FIFO fichier
```

### Ce que ce schéma corrige

| L'ancien disait | La réalité |
|---|---|
| `$A:TTS:speak:*:Bonjour#` | **`TTS` n'est pas une variable** : c'est `$A:Voice:tts:*:"Bonjour"#` |
| `$A:Mtr:speed:*:50#` | **`speed` n'existe pas** — `act`, `cmd`, `kturn`, `override`, `read`, `scale`, `state`, `stop`, `targets` |
| `$I:EchVoc:*:1#` | **`EchVoc` n'existe pas**, et la trame n'avait que 4 champs |
| `$E:STT:error:*:unknown#` | **`error` n'est pas une propriété de STT** — `context`, `intent`, `modeSTT`, `paraSTT`, `state` |
| « modePtge » | `modePtge` est un **nom de variable shell** dans `rz_state-manager.sh`, qui lit `CfgS.typePtge`. Ce n'est pas un nom du protocole |
| Table D : `Catégorie / Scope / Context` | modèle à 7 champs, remplacé par la Table D réelle à 16 colonnes |

### Et ce qu'il ajoute — la bifurcation que l'ancien manquait

**La colonne `Traitement` de la Table D décide de tout** *(`rz_stt_handler.sh`, entête)* :

- `SIMPLE` et `LocalSE` → le SE résout seul ;
- `TRADUCTION`, `CALCUL`, `MACRO` → la commande part **brute** vers SP, qui la résout par
  le mécanisme générique `ui_Traitement`.

L'ancien schéma ne montrait qu'un seul chemin. Il en existe deux, et c'est cette colonne
qui choisit.

Le reste de ce qu'il ajoute : les filtres de sécurité en amont (CPU, `modeRZ`, laisse),
la bascule IA quand la commande est introuvable (code retour 1), et la forme réelle de
la trame encapsulée — l'identifiant de requête voyage dans l'**instance**.

---

## 3. Capteurs et verrous de ressources

```mermaid
flowchart TB

    HGY["Centrale inertielle<br/>LSM6DS3C"] --> GY["rz_gyro_manager.sh<br/><b>Gyro</b>"]
    HMG["Magnetometre"] --> MG["rz_mg_manager.sh<br/><b>Mag</b>"]
    HSYS["CPU · memoire<br/>batterie · thermique"] --> SYS["rz_sys_monitor.sh<br/><b>Sys</b>"]
    HTOR["Torche LED"] --> TOR["rz_torch_manager.sh<br/><b>TorchSE</b>"]
    HMIC["Microphone"] --> MIC["rz_microSe_manager.sh<br/><b>MicroSE</b>"]
    HCAM["Cameras<br/>avant et arriere"] --> CAM["rz_camera_manager.sh<br/><b>CamSE</b>"]
    HHP["Haut-parleur<br/>jack · Bluetooth"] --> VOI["rz_voice_manager.sh<br/><b>Voice</b>"]
    APP["rz_appli_manager.sh<br/><b>Appli</b> — Zoom ..."]

    GY --> CIRCUIT
    MG --> CIRCUIT
    SYS --> CIRCUIT
    TOR --> CIRCUIT
    MIC --> CIRCUIT
    CAM --> CIRCUIT
    VOI --> CIRCUIT
    APP --> CIRCUIT

    CIRCUIT["fifo_termux_in<br/>puis rz_vpiv_parser.sh<br/>puis MQTT vers SP"]

    MIC -.-> LOCK
    CAM -.-> LOCK
    VOI -.-> LOCK
    APP -.->|"REFUS si deja detenu"| LOCK
    LOCK[("resource_locks.json<br/>camera · microphone<br/>un seul detenteur a la fois")]

    SYS -->|"seuils depasses"| EH["error_handler.sh"]
    EH -->|"$E:Urg:source:SE:...#"| CIRCUIT

    classDef materiel fill:#f96,stroke:#333,color:#000
    classDef manager fill:#ff9,stroke:#333,color:#000
    classDef verrou fill:#f99,stroke:#333,color:#000
    classDef circuit fill:#bbf,stroke:#333,color:#000
    class HGY,HMG,HMIC,HCAM,HTOR,HSYS,HHP materiel
    class GY,MG,MIC,CAM,TOR,SYS,VOI,APP manager
    class LOCK,EH verrou
    class CIRCUIT circuit
```

### Ce que ce schéma corrige

| L'ancien disait | La réalité |
|---|---|
| variable `Torch` | **`TorchSE`** |
| variable `Light` | **n'existe pas** — aucune variable de luminosité dans la Table A |
| `CamSE.front.resolution=720p` | `resolution` est une clé **à l'intérieur** du JSON `paraCam`, pas une propriété |
| `Gyro.active=On` | les propriétés de `Gyro` sont `angleVSEBase`, `dataContGyro`, `dataMoyGyro`, `modeGyro`, `paraGyro`, `statusGyro` |

### Et ce qu'il ajoute — l'arbitrage, qui était absent

**La caméra et le microphone sont des ressources exclusives.** `resource_locks.json` ne
tolère qu'un détenteur à la fois, et `rz_appli_manager.sh` (l.161-190) **refuse** d'ouvrir
Zoom si la caméra ou le micro sont déjà pris.

L'ancien schéma montrait caméra, micro, Zoom et TTS comme des branches parallèles qui
s'ignorent. **Elles se disputent le matériel**, et c'est précisément ce que le code
s'applique à arbitrer.

Second ajout : `rz_sys_monitor.sh` passe par `error_handler.sh` quand un seuil est
dépassé, et c'est de là que partent les trames `Urg` du SE.

---

## 4. Ce que ces schémas ont permis de trouver

Les refaire a mis au jour **un script mort** : `termux/rz_mqtt_listener.sh`, supprimé le
23/10. Il écoutait `$A:TTS:speak:` — donc le schéma ne mentait pas, il décrivait ce
script. Mais rien ne le lançait, il appelait `mosquitto_sub` et `mosquitto_pub` en direct
contre la règle absolue du projet, et sa branche Zoom ouvrait l'application sans vérifier
aucun verrou, là où `rz_appli_manager.sh` arbitre.

---

*Sources Mermaid : `docs/schemas/`. Les trois ont été rendus avant livraison.*
