# Architecture RobotRZ — Version 2.0
**Version : 2.0 — 2026-04-28 — Vincent Philippe**

---

## Vue d'ensemble

```
SP (Node-RED / RPI)
    ↕ MQTT (SE/statut, SE/commande)
Broker MQTT (RPI — 192.168.1.40:1883)
    ↕ MQTT
SE (Termux / Doro — 192.168.1.12)
    ↕ USB-C (Série)
Arduino Mega
    ↕ I2C/UART
Arduino UNO (moteurs)
```

---

## Format VPIV canonique

```
$CAT:VAR:PROP:INST:VAL#
```

| Champ | Position | Valeurs | Description |
|-------|----------|---------|-------------|
| CAT | 0 | V I E F A | Catégorie |
| VAR | 1 | CfgS, Sys, Gyro... | Variable globale |
| PROP | 2 | modeRZ, dataContCpu... | Propriété |
| INST | 3 | * SE rear front... | Instance |
| VAL | 4 | valeur | Valeur |

> ⚠️ **PROP est en position 3, INST en position 4** — ordre confirmé Table A.

---

## Topics MQTT

| Topic | Direction | Contenu |
|-------|-----------|---------|
| `SE/statut` | SE → SP | Capteurs, ACKs, urgences, états |
| `SE/commande` | SP → SE | Commandes, configuration, urgences |
| `SE/arduino/data` | Arduino → SP | Données Arduino relayées par mqtt_bridge.py |
| `SE/arduino` | SP → Arduino | Commandes Arduino via bridge |

---

## Architecture SE — Termux

### FIFOs

| FIFO | Écrit par | Lu par |
|------|-----------|--------|
| `fifo_termux_in` | Tous les managers | rz_vpiv_parser.sh → MQTT SE/statut |
| `fifo_tasker_in` | rz_vpiv_parser.sh | rz_tasker_bridge.sh → Tasker |
| `fifo_appli_in` | rz_vpiv_parser.sh | rz_appli_manager.sh |
| `fifo_voice_in` | rz_vpiv_parser.sh | rz_voice_manager.sh |
| `fifo_return` | rz_vpiv_parser.sh | Tasker bridge (ACKs) |

### Scripts core

| Script | Rôle | Version |
|--------|------|---------|
| `rz_start.sh` | Démarrage automatique | v1.0 |
| `rz_vpiv_parser.sh` | Daemon MQTT ↔ FIFO | v2.1 |
| `rz_state-manager.sh` | État global (global.json) | v1.4 |
| `mqtt_bridge.py` | Bridge Arduino USB-C | v2.0 |
| `safety_stop.sh` | Urgence danger | v3.0 |
| `error_handler.sh` | Erreurs centralisées | v2.0 |

### Managers capteurs

| Manager | Capteur | Mode défaut | État |
|---------|---------|-------------|------|
| `rz_sys_monitor.sh` | CPU/RAM/Batt/Temp | FLOW | ✅ Testé |
| `rz_gyro_manager.sh` | LSM6DS3C | DATACont | ✅ Testé |
| `rz_mg_manager.sh` | MMC3630KJ | DATAMgCont | ✅ Testé |
| `rz_camera_manager.sh` | IP Webcam Pro | off | ⏳ Partiel |
| `rz_microSe_manager.sh` | Micro ambiance | off | 📋 À tester |
| `rz_stt_manager.sh` | STT PocketSphinx | KWS | 📋 À tester |
| `rz_voice_manager.sh` | TTS | — | 📋 À tester |
| `rz_torch_manager.sh` | Torche | — | 📋 À tester |

---

## Architecture SP — Node-RED

### Onglets principaux

| Onglet | Rôle |
|--------|------|
| `Gtion_Init` | Initialisation — charge enrichedVars (Table B) |
| `RZ_GestionFlux_V3` | Bus VPIV — parsing, routage, encodeur |
| `Gtion_EtatGlobal_V1` | Règles métier — décisions modeRZ |
| `ui_Accueil_v3` | Interface utilisateur |
| `ui_VarG` | Éditeur Table C (enrichedVars) |
| `Diag_SE_Statut` | Diagnostic SE temps réel |

### Contexte global Node-RED

RZ_GestionFlux_V3 alimente `global.RZ_<VAR>` à chaque trame reçue.
Accès : `global.get('RZ_Sys').dataContCpu._last`

---

## Tables de données

| Table | Fichier | Rôle |
|-------|---------|------|
| A | (référence développement) | Normative — toutes variables VPIV |
| B | `node-red/config/globalSave.json` | Runtime Node-RED (enrichedVars) |
| C | `termux/config/courant_init.json` | Init SE + Arduino, modifiable via ui_VarG |
| D | `termux/scripts/sensors/stt/table_d_catalogue.csv` | Catalogue commandes STT |

---

## Sécurité

- Node-RED : `adminAuth` activé dans `settings.js`
- `keys.json` : non versionné (dans `.gitignore`)
- SSH Termux : port 8022, authentification par mot de passe
- NAT box : port 5800 → Termux SSH
