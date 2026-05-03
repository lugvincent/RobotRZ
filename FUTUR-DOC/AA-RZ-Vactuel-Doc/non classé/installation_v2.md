# Installation SE RobotRZ
**Version : 2.0 — 2026-04-28 — Vincent Philippe**

---

## Prérequis

- Téléphone Android (Doro) avec accès Play Store et F-Droid
- Connexion WiFi (même réseau que RPI broker 192.168.1.40)
- PC avec Git et VS Code + PlatformIO

---

## 1. Applications Android à installer

| Application | Source | Package | Rôle |
|-------------|--------|---------|------|
| Termux | **F-Droid** | com.termux | Shell Linux |
| Termux:API | **F-Droid** | com.termux.api | API Android |
| Termux:Tasker | **F-Droid** | com.termux.tasker | Bridge Tasker |
| Tasker | Play Store (payant) | net.dinglisch.android.taskerm | Automatisation Android |
| IP Webcam Pro | Play Store (payant) | com.pas.webcam.pro | Streaming caméra |

> ⚠️ **Termux, Termux:API et Termux:Tasker doivent tous venir de F-Droid.**
> Mélanger Play Store et F-Droid empêche la communication inter-apps.

---

## 2. Configuration Termux

```bash
# Permissions stockage
termux-setup-storage

# Mise à jour packages
pkg update && pkg upgrade -y

# Dépendances essentielles
pkg install -y git jq openssh curl mosquitto python

# Packages Python
pip install paho-mqtt pyserial --break-system-packages

# Démarrer SSH (port 8022)
sshd
```

Connexion SSH depuis le PC :
```bash
ssh rzServeur@192.168.1.12 -p 8022
```

---

## 3. Clone du dépôt Git

```bash
cd ~
git clone https://github.com/lugvincent/RobotRZ robotRZ-smartSE

git config --global user.name "lugvincent"
git config --global user.email "vincentphcom@gmail.com"

# Token GitHub (Personal Access Token)
git remote set-url origin https://lugvincent:VOTRE_TOKEN@github.com/lugvincent/RobotRZ
```

---

## 4. Création keys.json (OBLIGATOIRE — non versionné)

```bash
nano ~/robotRZ-smartSE/termux/config/keys.json
```

```json
{
    "GEMINI_API_KEY": "VOTRE_CLE_GEMINI",
    "MQTT_USER":      "rpibrokerUser",
    "MQTT_PASS":      "VOTRE_MOT_DE_PASSE_MQTT",
    "MQTT_HOST":      "192.168.1.40"
}
```

> ⚠️ Ce fichier est dans `.gitignore`. À recréer manuellement à chaque nouvelle installation.

---

## 5. Génération des fichiers de configuration

```bash
cd ~/robotRZ-smartSE/termux/config
bash check_config.sh
```

Fichiers générés :
- `gyro_runtime.json`
- `mag_runtime.json`
- `sys_runtime.json`
- `config/sensors/cam_config.json`
- `config/sensors/mic_config.json`
- `config/sensors/stt_config.json`
- `config/sensors/appli_config.json`

---

## 6. Démarrage du système

```bash
# Démarrage complet
bash ~/robotRZ-smartSE/termux/scripts/core/rz_start.sh

# Vérifier l'état
bash ~/robotRZ-smartSE/termux/scripts/core/rz_start.sh --status

# Arrêt propre
bash ~/robotRZ-smartSE/termux/scripts/core/rz_start.sh --stop
```

---

## 7. Configuration IP Webcam Pro

1. Ouvrir IP Webcam Pro sur le Doro
2. Menu → Paramètres de connexion → Port : 8080
3. Menu → Démarrer le serveur
4. Vérifier l'URL affichée : `http://192.168.1.12:8080`

Test depuis Termux :
```bash
curl -s --max-time 3 -I "http://192.168.1.12:8080/video"
```

---

## 8. Commandes de diagnostic

```bash
# État des processus
ps | grep -E "rz_|python"

# Test connexion MQTT
mosquitto_pub -h 192.168.1.40 -p 1883 \
    -u "rpibrokerUser" -P "MOT_DE_PASSE" \
    -t "SE/statut" -m "ping"

# Logs en temps réel
tail -f ~/robotRZ-smartSE/termux/logs/vpiv_parser.log
tail -f ~/robotRZ-smartSE/termux/logs/state_manager.log

# Vérifier les FIFOs
ls -la ~/robotRZ-smartSE/termux/fifo/

# Debug script bash
bash -x ~/robotRZ-smartSE/termux/scripts/core/rz_vpiv_parser.sh 2>&1 | head -40
```

---

## Notes importantes

- **Port SSH Termux** : 8022 (pas 22)
- **Port NAT box** : 5800 (accès distant)
- **`keys.json`** : jamais dans Git, toujours à recréer manuellement
- **`check_config.sh`** : à relancer si `courant_init.json` est modifié
- **`rz_start.sh`** : point d'entrée unique pour démarrer SE
