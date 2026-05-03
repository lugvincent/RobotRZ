
# Documentation Module : Sys (Moniteur Système)

## 1. Présentation Générale

Le module **Sys** est le gardien matériel du Smartphone Embarqué (SE). Il s'agit d'un processus d'arrière-plan chargé de mesurer en continu la santé physique du téléphone (processeur, température, espace disque, RAM et batterie).

Son but principal est d'éviter les crashs matériels (surchauffe, freeze CPU, coupure de batterie) en prévenant le Système Pilote (SP) ou en débrayant automatiquement les fonctions du robot en cas de criticité.

## 2. Architecture et Fichiers

Le module est découpé en trois fichiers distincts pour séparer la logique de boucle, le traitement mathématique des alertes et l'émission des protocoles d'urgence :

- **`rz\_sys\_monitor.sh`** : Cœur du module. Il gère la boucle principale d'exécution, la lecture/écriture des configurations (`sys\_runtime.json` et `global.json`) et l'envoi des trames VPIV régulières.

- **`alerts\_sys.sh`** : Bibliothèque spécialisée dans le calcul des dépassements de seuils. Elle intègre un système de "debounce" (anti-rebond) temporel pour éviter les fausses alertes sur des pics isolés.

- **`urgence.sh`** : Gestionnaire des codes d'urgence du SE. Il valide les codes d'erreurs avant émission pour garantir le respect strict de la Table A.


## 3. Modes de Fonctionnement (`modeSys`)

Le module propose 4 modes distincts pour s'adapter aux besoins d'économie d'énergie et de diagnostic du robot :

| **Mode** | **Description** | **Impact CPU / Réseau** |
| - | - | - |
| **`OFF`** | Le script s'arrête. Aucune mesure n'est prise. | Nul |
| **`ALERTE`** | Mode par défaut. Le script calcule les données en interne pour vérifier les seuils d'alerte mais n'envoie aucune donnée au SP tant que tout est nominal. | Très faible |
| **`FLOW`** | Envoi continu des métriques brutes au SP à la fréquence définie par `freqCont`. | Moyen (flux réseau) |
| **`MOY`** | Envoi périodique des données après un calcul de moyenne sur `nbCycles` mesures. Fréquence d'envoi définie par `freqMoy`. | Faible |


## 4. Propriétés VPIV et Paramétrage

### Commandes et Configurations (SP → SE)

- **`modeSys`** : Type `enum`. Modifie à la volée le mode actif décrit ci-dessus.

- **`paraSys`** : Objet JSON contenant les réglages et seuils.

  - `freqCont` : Période d'envoi en secondes pour le mode FLOW.

  - `freqMoy` : Période d'envoi en secondes pour le mode MOY.

  - `nbCycles` : Nombre d'échantillons utilisés pour calculer la moyenne.

  - `thresholds` : Contient pour chaque métrique (`cpu`, `thermal`, `storage`, `mem`, `batt`) :

    - `seuil\_warn` : Seuil de pré-alerte.

    - `seuil\_urg` : Seuil critique entraînant l'arrêt des opérations ou une mise en sécurité.

### Données et Événements (SE → SP)

- **`uptime`** : Entier. Émis une seule fois au boot du module. Représente le temps de fonctionnement du smartphone.

- **`dataCont`** / **`dataMoy`** : Objets JSON envoyés au SP regroupant les 5 métriques d'un coup pour limiter le nombre de messages MQTT.


## 5. Algorithme de Gestion des Seuils (Hystérésis Temporelle)

Pour éviter de saturer le Système Pilote d'alertes oscillatoires, `alerts\_sys.sh` applique une double barrière :

1. **Le sens de franchissement** : Les grandeurs `cpu`, `thermal`, `storage` et `mem` déclenchent l'alerte quand elles passent **au-dessus** du seuil. La grandeur `batt` déclenche l'alerte quand elle passe **en-dessous** du seuil.

2. **La temporisation de 10 secondes** : Une métrique qui dépasse un seuil ne lève pas l'alerte immédiatement. Elle doit rester hors limite pendant au moins **10 secondes consécutives** (`ALERT\_DURATION\_S`) pour qu'un message `$I:COM:warn:...` ou `$E:Urg:source:...` soit injecté dans le protocole.


## 6. Codes d'Urgences Système supportés

Lorsqu'une métrique atteint son `seuil\_urg` pendant 10 secondes, le fichier `urgence.sh` est sollicité. Voici les codes spécifiques gérés par le module physique du SE :

- **`URG\_CPU\_CHARGE`** : Surcharge processeur prolongée (risque de freeze des parsers).

- **`URG\_OVERHEAT`** : Température de batterie ou de processeur excessive.

- **`URG\_LOW\_BAT`** : Niveau de batterie insuffisant pour continuer la mission.

- **`URG\_SENSOR\_FAIL`** : Espace de stockage saturé ou mémoire RAM totalement épuisée.

Toute émission d'une de ces trames d'urgence par le SE provoque le passage immédiat des boucles d'envoi en sommeil dans l'attente d'un arbitrage ou d'un acquittement (`Urg:clear` ou `reset`) ordonné par le Système Pilote.
