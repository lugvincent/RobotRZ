# Module FS — Capteur de force (HX711)

## 1. Rôle

Le module FS mesure la force exercée sur la laisse et fournit :

- une valeur continue (force)

- un état d’alerte basé sur seuil

Utilisé principalement par Ctrl\_L pour déterminer la direction utilisateur.


## 2. Fonctionnement

### 2.1 Lecture

- Lecture brute via HX711

- Conversion : force = (raw - offset) / calibration


### 2.2 Publication

| PROP | TYPE | DESCRIPTION |
| - | - | - |
| force | flux | valeur normalisée |
| raw | flux | valeur brute |
| status | état | actif / alerte |
| alerte | état | valeur au dépassement |



### 2.3 Gestion alerte

Seuil : force| \> thd 

Transitions :

- actif → alerte

- alerte → actif


## 3. Propriétés VPIV

| PROP | DIR | TYPE | Rôle |
| - | - | - | - |
| act | ↔ | bool | activation |
| freq | ↔ | number | période ms |
| thd | ↔ | number | seuil |
| cal | ↔ | number | calibration |
| tare | → | cmd | remise à zéro |
| read | → | cmd | lecture immédiate |



## 4. États

| Valeur | Description |
| - | - |
| actif | fonctionnement normal |
| alerte | seuil dépassé |



## 5. Cycle

- périodique (freq)

- ou à la demande (read)


## 6. Sécurité

- freq=0 → désactive publication (mais pas alerte)

- act=0 → module inactif


## 7. Paramètres internes

| Paramètre | Rôle |
| - | - |
| cfg\_fs\_active | activation |
| cfg\_fs\_threshold | seuil |
| cfg\_fs\_freq\_ms | période |
| cfg\_fs\_calibration | gain |
| cfg\_fs\_offset | tare |



## 8. Remarques

- unité dépend calibration

- valeurs potentiellement signées

- utilisé pour pilotage directionnel

