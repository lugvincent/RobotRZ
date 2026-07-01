# config_Et_actualisation_code — Guide d'actualisation RobotRZ

**Chemin :** `~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/`  
**Version :** 1.0 — Juillet 2026 — Vincent Philippe

---

## Rôle de ce dossier

Ce dossier centralise tout ce qu'il faut savoir pour **remettre à jour** les
différents composants du projet RobotRZ après :
- un renouvellement de credential (token GitHub, clé Gemini, mot de passe MQTT)
- un `git clone` sur une nouvelle machine ou instance Termux
- un redémarrage complet du système après une longue absence

Il répond à un besoin réel : le projet comporte de nombreux composants
(Doro/Termux, RPi Node-RED, Arduino, Tasker, GitHub, DuckDNS, Gemini...)
et les credentials sont dispersés si on ne dispose pas d'un point d'entrée unique.

---

## Fichiers de ce dossier

| Fichier | Rôle |
|---------|------|
| `README_config_Et_actualisation_code.md` | Ce fichier — point d'entrée |
| `set_git_remote.sh` | Met à jour le remote Git depuis keys.json |

---

## keys.json — le fichier central des secrets

**Chemin :** `~/robotRZ-smartSE/termux/config/keys.json`  
**Statut :** ⛔ jamais dans Git (`.gitignore`) — à créer manuellement après chaque `git clone`

### Format complet actuel

```json
{
    "GEMINI_API_KEY": "VOTRE_CLE_GEMINI",
    "MQTT_USER":      "rpibrokerUser",
    "MQTT_PASS":      "VOTRE_MOT_DE_PASSE_MQTT",
    "MQTT_HOST":      "192.168.1.40",
    "DUCKDNS_TOKEN":  "VOTRE_TOKEN_DUCKDNS",
    "GITHUB_USER":    "lugvincent",
    "GITHUB_TOKEN":   "VOTRE_TOKEN_GITHUB"
}
```

### Où trouver chaque valeur

| Clé | Source | Où récupérer |
|-----|--------|--------------|
| `GEMINI_API_KEY` | Google AI Studio | `aistudio.google.com/app/apikey` |
| `MQTT_USER` | Config Mosquitto RPi | `/etc/mosquitto/passwd` (sur le RPi) |
| `MQTT_PASS` | Config Mosquitto RPi | Défini lors de l'installation |
| `MQTT_HOST` | IP statique RPi | Toujours `192.168.1.40` (bail DHCP fixe) |
| `DUCKDNS_TOKEN` | Compte DuckDNS | `duckdns.org` → token affiché en haut de page |
| `GITHUB_USER` | Compte GitHub | Toujours `lugvincent` |
| `GITHUB_TOKEN` | GitHub PAT | `github.com/settings/tokens` |

---

## Procédures d'actualisation par composant

### 1. Token GitHub expiré ou révoqué

**Symptôme :** `git push` ou `git pull` échoue avec erreur 401/403 sur le Doro.  
**Fréquence :** selon l'expiration choisie (ex. 90 jours).

```bash
# Étape 1 — Générer un nouveau token sur GitHub
# → github.com/settings/tokens → Fine-grained → RobotRZ → Contents Read/Write

# Étape 2 — Mettre à jour keys.json
nano ~/robotRZ-smartSE/termux/config/keys.json
# Modifier la valeur de "GITHUB_TOKEN"

# Étape 3 — Appliquer le nouveau remote
bash ~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/set_git_remote.sh
```

⚠️ **RÈGLE ABSOLUE** : ne jamais lancer `git remote -v` et coller le résultat
dans une conversation, email ou autre canal. Le token apparaît en clair dans l'URL.

---

### 2. Clé API Gemini expirée ou compromise

**Symptôme :** `rz_autovoice_bridge.sh` retourne une erreur 401/403 de l'API Gemini.

```bash
# Étape 1 — Révoquer l'ancienne clé
# → aistudio.google.com/app/apikey → supprimer l'ancienne clé

# Étape 2 — Générer une nouvelle clé sur le même écran

# Étape 3 — Mettre à jour keys.json sur le Doro
nano ~/robotRZ-smartSE/termux/config/keys.json
# Modifier la valeur de "GEMINI_API_KEY"

# Étape 4 — Vérifier que le script lit bien la nouvelle valeur
# (verification par longueur, jamais par valeur)
GEMINI_KEY=$(jq -r '.GEMINI_API_KEY' ~/robotRZ-smartSE/termux/config/keys.json)
echo "Longueur clé Gemini : ${#GEMINI_KEY} caractères"
# Attendu : environ 39 caractères pour une clé Gemini standard
```

---

### 3. Mot de passe MQTT modifié

**Symptôme :** connexion MQTT refusée depuis le Doro ou Node-RED.

```bash
# Sur le RPi (rzServeur) — modifier le mot de passe Mosquitto
mosquitto_passwd /etc/mosquitto/passwd rpibrokerUser
# Saisir le nouveau mot de passe deux fois

# Redémarrer Mosquitto
sudo systemctl restart mosquitto

# Sur le Doro — mettre à jour keys.json
nano ~/robotRZ-smartSE/termux/config/keys.json
# Modifier la valeur de "MQTT_PASS"

# Dans Node-RED (SP RPi) — mettre à jour le nœud MQTT broker
# → onglet du broker → modifier le mot de passe → déployer
```

---

### 4. Token DuckDNS modifié ou perdu

**Symptôme :** le script de renouvellement DuckDNS échoue (si utilisé en cron).

```bash
# Étape 1 — Récupérer le token sur duckdns.org (affiché en haut de page)

# Étape 2 — Mettre à jour keys.json
nano ~/robotRZ-smartSE/termux/config/keys.json
# Modifier la valeur de "DUCKDNS_TOKEN"
```

---

### 5. Nouveau git clone (nouvelle machine ou réinstallation Termux)

Après un `git clone https://github.com/lugvincent/RobotRZ` :

```bash
# Étape 1 — Créer keys.json (non versionné, absent du clone)
nano ~/robotRZ-smartSE/termux/config/keys.json
# Saisir toutes les valeurs depuis ce README

# Étape 2 — Configurer le remote Git avec le token
bash ~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/set_git_remote.sh

# Étape 3 — Vérifier les permissions des scripts
chmod +x ~/robotRZ-smartSE/termux/scripts/core/*.sh
chmod +x ~/robotRZ-smartSE/termux/scripts/sensors/**/*.sh
chmod +x ~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/*.sh
```

---

## Bonne pratique générale — vérifier un secret sans l'afficher

Dans tous les scripts et dans le terminal, toujours vérifier la présence
d'un secret par sa **longueur**, jamais par sa valeur :

```bash
# ✅ Correct — vérifie que la valeur est présente sans l'afficher
TOKEN=$(jq -r '.GITHUB_TOKEN' ~/robotRZ-smartSE/termux/config/keys.json)
echo "Longueur token : ${#TOKEN} car."   # Affiche le nombre, pas la valeur

# ❌ Interdit — affiche le token en clair
echo "Token : $TOKEN"
echo "$TOKEN"
```

---

## Rappels de sécurité

- `keys.json` ne doit **jamais** apparaître dans `git status`, `git diff` ou `git log`
- Si un secret fuite dans une conversation : le révoquer **immédiatement** sur le service concerné, puis le régénérer
- Les tokens GitHub de type fine-grained sont limités au seul dépôt `RobotRZ` — une fuite n'expose pas l'ensemble du compte
- Modèle Gemini actif : `gemini-2.5-flash` — vérifier la page changelog si une erreur 404 apparaît sur l'API
