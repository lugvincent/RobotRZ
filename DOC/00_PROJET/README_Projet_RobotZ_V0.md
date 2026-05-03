# **Robot Z Version (V0) – Synthèse complète**

---
## **0. Source et contenu**
- **Sources** :
  - Document original : `Robot Zv13.docx`.
  - Schémas et fichiers techniques associés.
- **Contenu** :
  - Objectifs et fonctionnalités de la V0.
  - Architecture matérielle et logicielle.
  - Protocoles de communication et flux de données.
  - Modes de fonctionnement et interactions.

---
## **1. Présentation générale de la version V0**
### **1.1. Objectifs initiaux**
- Développer un robot **évolutif**, **polyvalent** et **pédagogique**, capable de :
  - Fonctionner en mode **télécommandé** ou **autonome**.
  - Se déplacer dans un environnement en évitant les obstacles.
  - Porter une charge utile de **1 kg** (mode "serviteur").
  - Servir de plateforme sensorielle (capteurs multiples).
  - Suivre un humain à **4 km/h** pendant 1 heure.
  - Être un "compagnon" interactif (jeux, présence).
- **Contraintes physiques** :
  - Diamètre : **30–40 cm** (forme ronde ou rectangulaire arrondie).
  - Poids et volume permettant une **portabilité**.
  - Résistance aux chocs.

---
## **2. Architecture matérielle (V0)**
| **Composant**               | **Modèle/Description**                          | **Rôle**                                                                                     |
|-----------------------------|------------------------------------------------|----------------------------------------------------------------------------------------------|
| **Microcontrôleur**         | Arduino Mega + Raspberry Pi 4 B+               | Gestion des capteurs (Mega), traitement vidéo/IA (Raspberry Pi).                             |
| **Caméra**                  | Raspberry Pi Camera Module                     | Reconnaissance de forme via OpenCV.                                                          |
| **Microphones**             | 1 micro                                         | Détection de sons (version antérieure au ReSpeaker).                                         |
| **Capteurs Ultrason**       | 9 capteurs (HC-SR04)                            | Détection d'obstacles.                                                                       |
| **LED**                     | Ring de LED + LED RGB (émotions)               | Communication visuelle et émotions.                                                          |
| **Écran LCD**               | Écran LCD (I2C)                                 | Affichage des données (mode, vitesse, capteurs).                                             |
| **Buzzer**                  | Actif                                           | Alertes sonores.                                                                             |
| **Enceinte**                | Enceinte JBL Bluetooth                         | Qualité audio améliorée.                                                                    |
| **Communication**           | Bluetooth (HC-05), I2C, USB-UART, Wi-Fi         | Pilotage et échanges de données.                                                            |
| **Alimentation**            | Batterie plomb 12V                              | Alimentation principale (évolution prévue vers Li-Ion).                                      |

---
## **3. Modes de fonctionnement (V0)**
- **Autonome passif** : Surveillance via capteurs.
- **Autonome actif** : Surveillance + actionneurs (suivi de pilote).
- **Piloté** : Commande via manette, clavier, écran tactile, commande vocale.
- **Télé-présence** : Pilotage à distance via tablette/smartphone.

---
## **4. Protocoles de communication (V0)**
| **Protocole**       | **Utilisation**                                                                                     |
|---------------------|-----------------------------------------------------------------------------------------------------|
| **Bluetooth**       | Pilotage via tablette/smartphone.                                                                  |
| **I2C**             | Communication entre Raspberry Pi, Arduino, et écran LCD.                                           |
| **USB-UART**        | Échanges directs entre Raspberry Pi et Arduino.                                                    |
| **Wi-Fi**           | Transmission vidéo/audio et commandes via réseau local.                                            |
| **MQTT**            | Échanges de données entre Raspberry Pi et Arduino.                                                 |

---
## **5. Organisation logicielle (V0)**
- **Structure** : Programme Arduino monolithique (.ino).
- **Exemple de code** :
  ```cpp
  // Exemple de gestion des capteurs ultrason (V0)
  #include <NewPing.h>
  #define SONAR_NUM 9
  NewPing sonar[SONAR_NUM] = {
    NewPing(32, 33, 200), // AvG
    NewPing(34, 35, 200), // AvC
    // ...
  };
  void loop() {
    for (int i = 0; i < SONAR_NUM; i++) {
      Serial.print(sonar[i].ping_cm());
    }
  }
