# Fiche Rapide A2 — Flux de données et séquences clés du projet RZ

**Projet :** RZ — Robot compagnon autonome  
**Auteur :** Vincent Philippe | **Version :** 1.0 — Mars 2026

---

## Le protocole VPIV en bref

Tout message échangé entre SP, SE et Arduino suit ce format :

```
$CAT:VAR:PROP:INST:VAL#
```

| Champ | Rôle | Exemples |
|-------|------|----------|
| CAT | Type du message | V=Consigne, I=Info/ACK, E=Urgence, F=Flux, A=Appli |
| VAR | Module concerné | Gyro, CamSE, Mtr, CfgS, Urg |
| PROP | Propriété | modeRZ, active, cmd, source |
| INST | Instance (ou `*`) | rear, front, TGD, * |
| VAL | Valeur | On, 3, 50,0,2 |

**Exemples :**
```
$V:CfgS:modeRZ:*:3#        → ordre : passer en mode déplacement
$F:Gyro:dataCont:*:[x,y,z]# → flux : données gyroscope
$E:Urg:source:SE:URG_LOW_BAT# → urgence : batterie faible
$I:CamSE:active:rear:On#    → ACK : caméra démarrée
```

---

## Flux 1 — Démarrage et initialisation

```
[Installation initiale — 1 seule fois]

original_init.sh
    │
    ├─ Crée l'arborescence des répertoires
    ├─ Génère courant_init.json  (config tous modules)
    ├─ Génère global.json        (état initial : modeRZ=1 VEILLE)
    ├─ Génère table_d_catalogue.csv  (commandes vocales)
    │
    ├─► rz_build_system.py  →  stt_catalog.json + rz_words.txt
    ├─► rz_build_dict.py    →  fr.dict
    └─► check_config.sh
            ├─ Valide chaque bloc de courant_init.json
            └─ Génère :
                gyro_runtime.json
                mag_runtime.json
                sys_runtime.json
                cam_config.json
                mic_config.json
                stt_config.json
                appli_config.json

[Démarrage normal — à chaque boot]

Termux se lance automatiquement
    │
    ├─ mqtt_bridge.py        (liaison Arduino ↔ MQTT)
    ├─ rz_vpiv_parser.sh     (écoute MQTT SE/commande)
    ├─ rz_state-manager.sh   (gestion états global.json)
    ├─ rz_sys_monitor.sh     (surveillance CPU/temp/batt)
    ├─ rz_gyro_manager.sh    (flux gyroscope)
    ├─ rz_mg_manager.sh      (flux magnétomètre)
    ├─ rz_microSE_manager.sh (surveillance microphone)
    └─ rz_stt_manager        (écoute wake word "rz")

SP envoie courant_init.json via MQTT → SE opérationnel
```

---

## Flux 2 — Commande depuis l'interface SP (Node-RED)

```
Utilisateur clique dans l'interface Node-RED
    │
    │  $V:CfgS:modeRZ:*:3#
    ▼
Broker MQTT (DuckDNS)
    ▼
rz_vpiv_parser.sh  (topic SE/commande)
    │  parse : CAT=V, VAR=CfgS, PROP=modeRZ, VAL=3
    ▼
FIFO_TERMUX_IN
    ▼
rz_state-manager.sh
    │  met à jour global.json : CfgS.modeRZ = 3
    │  recalcule contexte STT
    │  publie ACK : $I:CfgS:modeRZ:*:3#
    ▼
Broker MQTT (topic SE/statut)
    ▼
SP (Node-RED) — affiche le nouveau mode
```

---

## Flux 3 — Commande vocale de l'utilisateur (STT)

```
Utilisateur dit : "RZ avance"
    ▼
Microphone Android → PocketSphinx
    │  wake word "rz" détecté → écoute la suite
    │  "avance" reconnu
    ▼
rz_stt_handler.sh "avance"
    │
    ├─ CPU > 90% ?    → TTS "Je suis trop chargé" → stop
    ├─ "avance" dans stt_catalog.json ? → MTR_FORWARD  ✓
    ├─ modeRZ=0 ou 5 ? → bloqué
    ├─ laisse + moteur ? → bloqué
    └─ SIMPLE → capsule : $A:STT:intent:*:{V:Mtr:cmd:*:50,0,2}#
                          → FIFO_TERMUX_IN
                          → TTS : "Je fonce !"  (aléatoire)
    ▼
rz_state-manager.sh
    │  valide l'intent (modeRZ ok ?)
    │  extrait trame interne : $V:Mtr:cmd:*:50,0,2#
    ▼
Broker MQTT → SP → valide → publie vers Arduino
    ▼
mqtt_bridge.py → Série USB-C → Arduino Mega
    ▼
Arduino : moteurs en avant à vitesse 50

[Si "avance" non reconnu par PocketSphinx]
    ▼
rz_stt_handler.sh → exit 1
    ▼
rz_ai_conversation.py "rz avance"
    │  API Gemini → réponse textuelle
    ▼
TTS : réponse Gemini
```

---

## Flux 4 — Données capteur (flux continu)

```
rz_gyro_manager.sh (boucle)
    │  termux-sensor -s "Gyroscope"
    │  filtre seuilMesure
    │  calcule si mode DATAMoy : accumule N valeurs
    ▼
$F:Gyro:dataCont:*:[x,y,z]#
    ▼
Broker MQTT (topic SE/statut)
    ▼
SP (Node-RED) → affiche courbes / graphiques
```

---

## Flux 5 — Urgence (sécurité)

```
rz_sys_monitor.sh
    │  CPU_LOAD = 92%  (> seuil urg=90)
    ▼
FIFO_TERMUX_IN :  $E:Urg:source:SE:URG_CPU_CHARGE#
    ▼
rz_vpiv_parser.sh → forward vers broker MQTT
    ▼
SP analyse → décide niveau "danger"
    │  publie : $E:Urg:etat:SE:danger#
    ▼
rz_vpiv_parser.sh  (POINT C — priorité absolue)
    ▼
safety_stop.sh
    ├─ $V:MotA:state:*:Stop#    → FIFO (arrêt moteur)
    ├─ $V:CamSE:active:*:Off#   → FIFO (coupure caméra)
    ├─ $V:TorchSE:power:*:0#    → FIFO (extinction torche)
    └─ log emergency.log

[Résolution]
SP envoie : $V:CfgS:(emg):*:clear#
    ▼
Arduino : urg_clear() — LEDs retour normal
SE : global.json Urg.statut = "cleared"
```

---

## global.json — La mémoire partagée SE

Tous les managers lisent et écrivent dans ce fichier central.

```json
{
  "CfgS": {
    "modeRZ": 1,      ← Mode courant (0=ARRET à 5=ERREUR)
    "typePtge": 0,    ← Type pilotage (0=Ecran à 4=Laisse+Vocal)
    "reset": false
  },
  "Urg": {
    "source": "URG_NONE",    ← Code urgence active
    "statut": "cleared",     ← cleared | active
    "niveau": "warn"         ← warn | danger
  },
  "COM": {
    "debug": "", "error": "", "info": "", "warn": ""
  },
  "Sys": {
    "cpu_load": 0,   ← % — mis à jour par rz_sys_monitor
    "temp": 0,       ← °C
    "storage": 0,    ← % SDCard
    "mem": 0,        ← % RAM
    "batt": 0,       ← % batterie SE
    "uptime": 0,     ← secondes depuis boot
    "last_update": ""
  }
}
```

**Qui écrit quoi :**
- `CfgS` → rz_state-manager.sh uniquement
- `Sys` → rz_sys_monitor.sh uniquement
- `Urg` → rz_vpiv_parser.sh (réception SP) + error_handler.sh
- `COM` → rz_vpiv_parser.sh

---

## Résumé — Rôle des FIFOs

Les FIFOs sont les tuyaux de communication internes entre les processus SE.

| FIFO | Direction | Usage |
|------|-----------|-------|
| `fifo_termux_in` | Tous → rz_state-manager | Commandes internes, ACKs, états |
| `fifo_tasker_in` | rz_vpiv_parser → Tasker | Commandes apps Android |
| `fifo_return` | Managers → parser | Réponses et retours |

---

*Fiche A2 — Flux de données et séquences — Projet RZ v1.0 — Mars 2026 — Vincent Philippe*
