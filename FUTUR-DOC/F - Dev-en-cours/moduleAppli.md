
# Documentation Module : Appli (Android App Manager)

## 1. Présentation Générale

Le module **Appli** permet au Système Pilote (SP) de prendre le contrôle de l'interface utilisateur du smartphone. Il transforme le SE en une plateforme multi-usages capable de basculer dynamiquement entre une caméra de surveillance, une interface de visioconférence ou un outil de navigation GPS.

## 2. Architecture Technique

Le module fonctionne comme un **daemon (service d'arrière-plan)** via le script `rz\_appli\_manager.sh`.

- **Écoute :** Il surveille en continu le tube nommé `fifo\_appli\_in`.

- **Exécution :** Il délègue les tâches lourdes au script `rz\_tasker\_bridge.sh` qui communique avec l'application **Tasker**.

- **Fallback :** Si Tasker n'est pas requis, il utilise directement la commande Android `am start` (Activity Manager).

## 3. Liste des Applications Supportées (Catalogue v1)

| **Nom Interne** | **Application Cible** | **Méthode** |
| - | - | - |
| **`Baby`** | Caméra de surveillance bébé | Tasker (Scène dédiée) |
| **`tasker`** | Interface de configuration Tasker | `am start` direct |
| **`zoom`** | Application Zoom (Visioconférence) | Tasker + Intent |
| **`BabyMonitor`** | Moniteur audio/vidéo distant | Tasker |
| **`NavGPS`** | Google Maps / Navigation | Tasker |

## 4. Gestion des Conflits et Priorités

Avant chaque lancement, le manager exécute la fonction `check\_conflits()`.

- **Logique :** On ne peut pas lancer deux applications utilisant la caméra ou le micro simultanément (ex: lancer *Zoom* alors que *Baby Cam* est active).

- **Résolution :** Si une application est déjà en cours, le manager envoie un message d'erreur `$I:COM:error:SE:"Application X déjà active"\#` au lieu d'exécuter la commande.

## 5. Flux de Commande VPIV

Le module utilise une nomenclature spécifique pour différencier les commandes système des commandes applicatives :

1. **Commande (SP → SE) :** `$A:Appli:launch:\*:Baby\#` (Note l'utilisation du **A** pour Applicatif).

2. **Acquittement (SE → SP) :**

   - `$I:Appli:launch:\*:OK\#` : L'application a été lancée avec succès.

   - `$I:Appli:launch:\*:FAIL\#` : Échec du lancement (conflit ou erreur système).

## 6. Dépendances Matérielles

- **Android Debug Bridge (adb)** ou accès **Shell root/privilégié** via Termux.

- **Tasker** avec le plugin "Termux:Tasker" installé et configuré.

- Fichier de configuration `appli\_config.json` présent dans le répertoire `/config/`
