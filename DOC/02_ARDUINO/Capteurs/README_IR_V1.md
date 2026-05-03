# [Arduino] - Intégration et calibration des capteurs IR (V1)

---

## **1. Résumé Général : Fonctionnement et utilité des capteurs IR**
Les capteurs infrarouges (IR) sont utilisés en robotique pour la **détection d’obstacles** et la **mesure de distances courtes**. Leur fonctionnement repose sur l’émission d’un signal IR et la mesure de son retour après réflexion sur un obstacle. Leur **simplicité**, **coût réduit** et **faible consommation** en font un choix idéal pour les systèmes d’évitement d’obstacles.

**Dans ce projet** :
- Les capteurs IR sont intégrés à une plateforme robotique basée sur une **Arduino Mega**.
- Leur rôle principal est de **détecter les obstacles en temps réel** et de déclencher des actions correctives (arrêt, changement de trajectoire, etc.).
- La bibliothèque **Arduino-IRremote (v2.0.3)** est utilisée pour faciliter la gestion des signaux IR.

---

## **2. Résumé Spécifique au Projet**

### **2.1. Câblage et Code Arduino**
- **Câblage** :
  Les capteurs IR sont connectés aux broches **numériques et analogiques** de l’Arduino Mega :
  - Alimentation : **5V**.
  - Masse : **GND**.
  - Broche de signal : connectée à une entrée numérique ou analogique.
  - Un **schéma de câblage validé** permet d’éviter les interférences entre capteurs.

- **Code Arduino** (fichier : [IR_V1.ino](../Code/Arduino/Capteurs/IR_V1.ino)) :
  - Configuration des broches pour chaque capteur IR.
  - Boucle de lecture continue des valeurs des capteurs.
  - Algorithme de **calibration** pour ajuster la sensibilité en fonction de l’environnement.
  - Logique de détection d’obstacles avec **seuils personnalisables**.
  - Envoi de signaux d’alerte via :
    - Le **port série** (pour débogage).
    - Un **module de communication** (Bluetooth/Wi-Fi) pour la remontée d’informations.

- **Calibration** :
  - Mesure des valeurs **minimales et maximales** retournées par les capteurs dans l’environnement cible.
  - Stockage des valeurs en **EEPROM** pour une réutilisation ultérieure.

### **2.2. Intégration avec Tasker**
- Les alertes générées par l’Arduino (ex: `OBSTACLE_1`) sont transmises au smartphone via **Bluetooth ou MQTT**.
- Un profil Tasker ([Alarmes.task](../../Code/Tasker/Alarmes.task)) écoute les messages entrants et déclenche :
  - Des **notifications vocales ou visuelles**.
  - Des **actions correctives** (ex: arrêt du robot via MQTT).

### **2.3. Problèmes Rencontrés et Solutions**
| Problème                | Solution                                                                                     |
|-------------------------|---------------------------------------------------------------------------------------------|
| Interférences lumineuses | Utilisation de **filtres logiciels** (moyenne mobile) et ajustement dynamique des seuils.     |
| Interférences entre capteurs | Activation **séquentielle** des capteurs et utilisation de délais entre les lectures.       |
| Latence                  | Optimisation du code Arduino et utilisation de **buffers** pour les communications série. |

---

## **3. Liens et Références**
- **Chat associé** : [[Tasker] - Gestion des alertes (V1)](#) (pour la partie traitement des alertes sur smartphone).
- **Bibliothèque** : [Arduino-IRremote v2.0.3](https://github.com/Arduino-IRremote/Arduino-IRremote).
- **Fichiers GitHub** :
  - Code Arduino : [IR_V1.ino](../Code/Arduino/Capteurs/IR_V1.ino).
  - Profil Tasker : [Alarmes.task](../../Code/Tasker/Alarmes.task).

---

## **4. Répartition Thématique**
- **80% [Arduino] - Gestion des capteurs** : Câblage, code, calibration, et traitement des données.
- **20% [Tasker] - Automatisation mobile** : Réception des alertes et déclenchement d’actions sur smartphone.

---

## **5. À Améliorer (Changelog pour V1a)**
- **Robustesse** :
  - Tester la calibration dans des environnements variés (lumière forte, surfaces réfléchissantes).
  - Intégrer un système de **redondance** avec d’autres capteurs (ultrasons, LiDAR).
- **Optimisation** :
  - Réduire la consommation énergétique en désactivant les capteurs inutilisés.
  - Améliorer la précision en combinant les données de plusieurs capteurs (fusion de données).
- **Documentation** :
  - Ajouter un **guide détaillé** pour la calibration manuelle.
  - Documenter les **valeurs de seuil optimales** pour différents scénarios.

---

## **6. Schéma Associé**
| Composant       | Broche Arduino Mega | Remarques                          |
|-----------------|---------------------|------------------------------------|
| Capteur IR 1    | D2 (Signal)         | Alimentation : 5V, GND            |
| Capteur IR 2    | D3 (Signal)         | Activation séquentielle           |
| Module Bluetooth| TX1/RX1             | Communication avec Tasker         |
