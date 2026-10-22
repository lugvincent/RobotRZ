# Installation du RPi serveur — procédure de référence

> **Fichier** : `docs/installation_RPI_serveur.md`
> **Objet** : installer et configurer de zéro le Raspberry Pi 3 B+ qui héberge
> le broker MQTT, Node-RED et le serveur web du projet RobotRZ.
> **Articulation** : complète `termux/install/` (qui couvre SE, le smartphone
> embarqué) et `docs/dev_RZ_HTML/point_reseau_ssh_V1.html` (accès distant).
> La contrepartie côté SE est `original_init.sh`.
> **Précaution** : ce document ne contient **aucun identifiant**. Les mots de
> passe, jetons et adresses publiques sont dans
> `AA-Doc-horsGit\Z - Secret\RZ_Codes_Identifiants.odt`, hors dépôt.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe / Claude
> **Source** : reconstruit à partir du journal de mise au point de
> décembre 2024 – janvier 2025 (54 échanges), réordonné par tâche et corrigé
> des impasses rencontrées en chemin.

---

## 0. Ce qu'il faut savoir avant de commencer

Trois décisions ont été prises à l'usage et n'ont pas à être rejouées.

**Raspberry Pi OS 64 bits, avec bureau.** Le Pi 3 B+ a un processeur 64 bits
(Broadcom BCM2837B0, Cortex-A53) et la machine dispose de 4 Go de RAM : le
32 bits ne saurait pas les exploiter. Le bureau est conservé pour l'accès VNC,
utile en dépannage.

**L'IP fixe se règle sur la box, pas sur le Pi.** Les deux sont possibles, mais
les configurer aux deux endroits crée des conflits dès que l'un des deux change.
La box, qui attribue par adresse MAC, reste la référence.

**⚠️ `dhcpcd.conf` n'existe plus.** C'est le piège qui a coûté le plus de temps,
et tous les tutoriels que vous trouverez en ligne vous y enverront. Voir §2.

---

## 1. Système de base

```bash
sudo apt update && sudo apt upgrade -y
```

Activer VNC si l'accès graphique distant est voulu :

```bash
sudo raspi-config
```

> *Interface Options* → *VNC* → Activer.

---

## 2. Réseau — le point qui fait échouer les tutoriels

### Le piège

Toute la documentation en ligne, et la première moitié du journal d'origine,
indique de déclarer l'IP statique dans `/etc/dhcpcd.conf`. Sur Raspberry Pi OS
récent, **ce fichier n'est plus lu** et le service n'existe pas :

```
$ sudo systemctl restart dhcpcd
Failed to restart dhcpcd.service: Unit dhcpcd.service not found.
```

Le réseau est géré par **NetworkManager**. Éditer `dhcpcd.conf` ne produit
aucun effet et aucun message d'erreur — on croit avoir configuré, et rien
n'a changé.

### Vérifier qui gère le réseau

```bash
ls /etc/systemd/system/multi-user.target.wants/
```

On y cherche `NetworkManager.service` (cas actuel), `dhcpcd.service` (ancien)
ou `systemd-networkd.service`.

### Configurer, avec `nmtui`

```bash
sudo nmtui
```

*Modifier une connexion* → la connexion filaire (`eth0`) → *CONFIGURATION IPv4* :

| Champ | Valeur | Remarque |
|---|---|---|
| Méthode | **Manuel** | au lieu d'*Automatique* |
| Adresses | `192.168.1.40/24` | le `/24` est le masque — 256 adresses de .0 à .255 |
| Passerelle | `192.168.1.1` | la box |
| Serveurs DNS | `8.8.8.8` puis `8.8.4.4` | **un par ligne**, pas de virgule |
| Domaines de recherche | *vide* | n'a d'intérêt qu'en réseau d'entreprise |
| Routage | *vide, aucune case cochée* | la route par défaut suffit |

Puis *Valider*, revenir à l'écran principal, et **désactiver puis réactiver**
la connexion — sans quoi rien n'est appliqué.

> Le champ DNS de `nmtui` refuse la virgule : il faut une adresse par ligne.
> Pour en déclarer plusieurs autrement, voir le fichier direct ci-dessous.

### En alternative, par fichier

```bash
sudo nano /etc/NetworkManager/system-connections/eth0.nmconnection
```

```ini
[ipv4]
method=manual
addresses=192.168.1.40/24
gateway=192.168.1.1
dns=8.8.8.8;8.8.4.4;

[ipv6]
method=ignore
```

```bash
sudo systemctl restart NetworkManager
```

> Modifier `/etc/resolv.conf` directement fonctionne aussi, mais NetworkManager
> écrase ce fichier : la modification ne survit pas au redémarrage.

### Vérifier

```bash
ip a                  # l'adresse doit apparaître sur eth0
ping -c4 192.168.1.1  # la box
ping -c4 8.8.8.8      # Internet par IP
ping -c4 google.com   # Internet avec résolution DNS
```

Les trois doivent passer. Un temps de réponse autour de 0,6 ms vers la box est
normal en Ethernet. Si le troisième échoue seul, le problème est le DNS, pas la
connexion.

### Le cas des deux interfaces

Le Pi a une interface Ethernet et une Wi-Fi, chacune pouvant porter une adresse.
**N'en utiliser qu'une** pour le serveur : en service permanent, c'est
l'Ethernet. Laisser les deux actives produit des comportements déroutants — la
box n'en montre souvent qu'une, et le trafic emprunte celle qu'on n'attendait
pas.

---

## 3. Node.js, puis Node-RED

L'ordre compte : Node-RED dépend de Node.js.

```bash
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs
node -v && npm -v
```

```bash
sudo npm install -g --unsafe-perm node-red
sudo systemctl enable nodered.service
sudo systemctl start nodered.service
```

Test : `http://192.168.1.40:1880`

> `adminAuth` doit être activé dans `settings.js` avant toute exposition vers
> l'extérieur. Le mot de passe y est stocké haché en bcrypt, donc irréversible :
> notez le mot de passe en clair dans le classeur d'identifiants, section 6,
> sinon il est définitivement perdu.

---

## 4. Broker MQTT — Mosquitto

```bash
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

Test en local, dans deux terminaux :

```bash
mosquitto_sub -h localhost -t test
```

```bash
mosquitto_pub -h localhost -t test -m "Hello MQTT"
```

Le message doit apparaître dans le premier terminal. Si rien ne s'affiche,
c'est que l'abonnement n'était pas actif au moment de la publication : MQTT ne
rejoue pas les messages passés, sauf en mode *retained*.

> **`localhost` ou `192.168.1.40` ?** Depuis le Pi lui-même, `localhost` suffit
> et évite de dépendre du réseau. Depuis toute autre machine — le Doro, le PC —
> c'est `192.168.1.40` qu'il faut, car `localhost` y désignerait la machine
> appelante.

### Sécuriser le broker

Le broker ne doit pas rester anonyme. Dans `/etc/mosquitto/mosquitto.conf` :

```
allow_anonymous false
password_file /etc/mosquitto/passwd
listener 1883
```

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd <utilisateur>
sudo systemctl restart mosquitto
```

> Identifiant et mot de passe → classeur d'identifiants, section 5. Côté SE,
> ils sont lus dans `termux/config/keys.json` (`MQTT_USER`, `MQTT_PASS`,
> `MQTT_HOST`), jamais écrits en dur dans les scripts.

---

## 5. Serveur web

Nginx est préférable à Apache pour des pages essentiellement statiques, et plus
léger sur un Pi 3.

```bash
sudo apt install -y nginx
```

Test : `http://192.168.1.40`

PHP et MariaDB **ne sont pas installés** — décision prise à l'usage : le projet
n'en a pas besoin, et ils chargent inutilement la machine.

---

## 6. Accès depuis l'extérieur

### Redirection de ports sur la box

Vers `192.168.1.40` :

| Port | Service |
|---|---|
| 80 | HTTP |
| 443 | HTTPS |

**N'ouvrez que ce dont vous avez l'usage.** Chaque port redirigé est une porte
qui reste ouverte longtemps après qu'on a oublié l'avoir ouverte. SSH et VNC
n'ont pas à être exposés : ils passent par le réseau local, ou par un tunnel.

### DNS dynamique

L'IP publique d'une ligne résidentielle change. Un service de DNS dynamique
maintient un nom stable qui la suit.

DuckDNS est retenu, plutôt que No-IP : gratuit sans condition, sans
renouvellement périodique à confirmer, et sans publicité. No-IP impose une
reconfirmation tous les trente jours sous peine de suppression.

Dans l'interface DuckDNS, le champ « current ip » prend **l'adresse IP publique
seule** — sans `http://` devant, sans barre oblique derrière, et ce n'est pas
l'adresse locale de la machine.

> Le nom DuckDNS et l'IP publique vont au classeur d'identifiants, section 4.
> Associés à la liste des ports ouverts, ils décrivent votre surface d'attaque :
> ils n'ont leur place dans aucun document ordinaire.

### Certificat TLS

```bash
sudo apt install -y certbot python3-certbot-nginx
sudo certbot --nginx
```

Certbot demande : *« the domain name you would like on your certificate »* —
c'est-à-dire **le nom de domaine à inscrire sur le certificat**. On y met le
nom DuckDNS complet, pas une adresse IP : un certificat ne se délivre que pour
un nom.

### IPv6

Pas nécessaire. Une adresse IPv6 est normale et sa longueur aussi — elle
contient l'identifiant de l'interface, dérivé de l'adresse MAC de la carte.
Ne la publiez pas : contrairement à l'IPv4, elle désigne **la machine
elle-même** et ne passe par aucun NAT.

Pour le serveur, s'en tenir à l'IPv4 et mettre IPv6 sur *Ignorer* dans `nmtui`.

### Le piège du NAT hairpin

Tester l'accès distant **depuis le réseau local ne prouve rien** : beaucoup de
box ne savent pas renvoyer vers l'intérieur une requête adressée à leur propre
IP publique. La requête reste bloquée sur `Trying...` alors que tout fonctionne
depuis l'extérieur.

Testez toujours depuis une connexion 4G ou un autre réseau.

---

## 7. Ordre d'installation — récapitulatif

1. Système et mises à jour
2. **Réseau — NetworkManager, pas `dhcpcd`**
3. Node.js
4. Node-RED
5. Mosquitto, puis sécurisation du broker
6. Nginx
7. Redirections de ports, DuckDNS, certificat
8. Vérification depuis l'extérieur, pas depuis le réseau local

---

## 8. Dépannage

| Symptôme | Cause probable |
|---|---|
| `dhcpcd.service not found` | Normal. Le réseau est géré par NetworkManager (§2) |
| L'IP ne change pas après édition de `dhcpcd.conf` | Le fichier n'est plus lu (§2) |
| Ping box OK, ping `google.com` KO | DNS mal renseigné dans `nmtui` |
| `mosquitto_sub` ne reçoit rien | Abonnement postérieur à la publication — MQTT ne rejoue pas |
| Accès distant bloqué sur `Trying...` depuis le Pi | NAT hairpin — tester depuis l'extérieur (§6) |
| Une seule interface visible sur la box | Normal si Ethernet et Wi-Fi sont tous deux actifs (§2) |
| CPU à 100 % sans raison apparente | Firefox resté ouvert sur le bureau. `htop` pour identifier le PID |

```bash
sudo apt install -y htop     # surveillance des ressources
```

---

## 9. Écarts constatés avec l'état actuel

Le journal d'origine décrit le RPi en `192.168.1.38` (Ethernet) et `192.168.1.37`
(Wi-Fi). **L'adresse actuelle est `192.168.1.40`**, confirmée le 22/10/2026, et
c'est celle retenue dans tout ce document. Les anciennes valeurs subsistent dans
`historique.odt` (partie II) et dans de vieilles notes : elles sont périmées.

---

*Reconstruit le 22/10/2026 à partir d'un journal de conversation de 9 066 lignes
et 54 échanges, dont les huit premiers sans rapport avec le projet. Le journal
d'origine procède par essais successifs et se contredit — il commence par
`dhcpcd.conf` et finit par NetworkManager. Ce document donne les conclusions,
pas le chemin qui y mène.*
