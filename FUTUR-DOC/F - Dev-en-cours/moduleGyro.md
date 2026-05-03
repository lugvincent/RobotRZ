
# Documentation Module : Gyro (Gyroscope SE)

## 1. Présentation Générale

Le module **Gyro** est responsable de la lecture des capteurs de rotation du smartphone embarqué. Il utilise l'API Termux-Sensor pour récupérer les vitesses angulaires sur les axes X, Y et Z. Ces données sont essentielles pour le maintien de l'assiette du robot, la détection de basculement ou l'aide à la navigation.

## 2. Architecture Technique

Le script `rz\_gyro\_manager.sh` pilote le capteur en arrière-plan.

- **Source de données :** Commande `termux-sensor -n 1 -s "gyroscope"`.

- **Traitement :** Extraction et formatage via `jq`.

- **Transmission :** Les valeurs transmises via le protocole VPIV sont multipliées par 100 (`rad/s \* 100`) pour éviter les nombres flottants, conformément à la **Table A**.

## 3. Modes de Fonctionnement (`modeGyro`)

Le module supporte quatre modes d'opération sélectionnables par le Système Pilote (SP) :

| **Mode** | **Action** | **Fréquence d'envoi** |
| - | - | - |
| **`OFF`** | Le capteur est arrêté. Économie d'énergie maximale. | Aucune |
| **`DATACont`** | Envoi des données brutes instantanées. Idéal pour la stabilisation active. | `freqCont` (Hz) |
| **`DATAMoy`** | Calcul d'une moyenne glissante sur `nbCycles`. Idéal pour la navigation. | `freqMoy` (Hz) |
| **`ALERTE`** | Le SE surveille les seuils en local. Il n'envoie rien sauf en cas de dépassement. | Événementiel |

## 4. Propriétés et Paramétrage (VPIV)

### Configuration (SP → SE)

- **`modeGyro`** : Commande du mode actif.

- **`paraGyro`** (Objet JSON) :

  - `freqCont` / `freqMoy` : Fréquences de publication en Hertz (ex: 10 = 10 messages/s).

  - `nbCycles` : Fenêtre de lissage pour le mode `DATAMoy`.

  - `thresholds` : Seuils de détection (en `rad/s \* 100`) déclenchant l'état `ALERTE`.

- **`angleVSEBase`** : Définit l'inclinaison physique du smartphone dans son support pour compenser les mesures.

### Retours de Données (SE → SP)

- **`dataContGyro` / `dataMoyGyro`** : Tableaux JSON `\[x, y, z\]`.

- **`statusGyro`** : Événement (`$E:Gyro:statusGyro:...`) envoyé lors d'un changement d'état ou d'une anomalie.

## 5. Formatage des Données

Le gyroscope Android renvoie des données en `rad/s`. Pour la compatibilité VPIV :

1. Lecture brute (ex: `0.1234 rad/s`).

2. Multiplication par 100 (ex: `12`).

3. Envoi de l'entier.

*Note : Le SP doit diviser par 100 pour retrouver la valeur réelle.*

## 6. Sécurité et Alertes

Si le module est en mode `ALERTE` ou si une mesure dépasse les `thresholds` définis, le SE émet immédiatement une trame d'événement. Cela permet au Système Pilote de détecter :

- Un choc ou un mouvement brusque.

- Une chute ou un basculement du robot.

- Un dysfonctionnement du capteur (état `ERROR`).

