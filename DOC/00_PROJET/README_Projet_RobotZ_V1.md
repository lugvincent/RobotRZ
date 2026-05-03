# **Robot Z Version (V1) – Synthèse complète et optimisée**

---

## **0. Source et contexte**
- **Sources** :
  - Présentation initiale : `Résumé du Projet RobotZ - Refactorisation et Optimisation (V1)`.
  - Structure de dépôt Git : `RobotRZ/` (détails ci-dessous).
  - Documents techniques : `Robot Zv13.docx`, schémas d'architecture, et fichiers de configuration.
- **Contenu** :
  - Objectifs et fonctionnalités de la V1.
  - Architecture matérielle et logicielle refactorisée.
  - Protocoles de communication et flux de données optimisés.
  - Structure des dossiers et fichiers pour GitHub.
  - Comparaison avec la version V0.

---

## **1. Objectif global**
### **1.1. Refactorisation et optimisation**
- **Centralisation** :
  - Remplacement du **Raspberry Pi** par un **smartphone embarqué (SE)** pour simplifier l'architecture.
  - Utilisation de **Node-RED** et **Tasker** pour le pilotage et l'automatisation.
- **Optimisation** :
  - Refactorisation du code Arduino en modules (.ino, .h, .cpp).
  - Suppression des protocoles **I2C** et **Bluetooth** (sauf pour l'enceinte JBL).
  - Uniformisation du format des données pour Node-RED.

### **1.2. Architecture du projet**
| **Composant**            | **Rôle**                                                                                     |
|--------------------------|----------------------------------------------------------------------------------------------|
| **Arduino Mega**         | Gestion des capteurs (ultrasons, IR), servos, moteurs, et LEDs NeoPixel.                      |
| **Smartphone SE**        | Échange de données via **MQTT** avec un smartphone pilote (SP) utilisant **Node-RED**.     |
| **Node-RED**             | Tableau de bord pour définir des objets (ex: capteurs) avec des propriétés standardisées.  |
| **Tasker**               | Automatisation des tâches sur SE pour l'envoi des données.                                  |

---

## **2. Avancées récentes**
### **2.1. Nouveaux codes et refactorisation**
| **Composant**      | **Avancées**                                                                                     |
|--------------------|-----------------------------------------------------------------------------------------------|
| **Node-RED**      | Structure de données pour les objets (ex: capteurs ultrasons) avec des propriétés uniformisées. |
| **Arduino**        | Refactorisation du code pour une meilleure modularité et intégration avec le nouveau format de données. |
| **Tasker**         | Automatisation des échanges MQTT entre SE et SP.                                               |

### **2.2. Exemple de structure de données**
- **Format** : `$<variable>[.<instance>][.<propriété>][=<valeur>]#`
- **Exemple** :
  - `$US.1.DIST=150#` : Capteur ultrason 1, propriété `DIST`, valeur 150 cm.
  - `$LED.1.COLOR=RED#` : Ruban de LED 1, propriété `COLOR`, valeur `RED`.

---

## **3. Architecture matérielle (V1)**
| **Composant**               | **Modèle/Description**                          | **Rôle**                                                                                     |
|-----------------------------|------------------------------------------------|----------------------------------------------------------------------------------------------|
| **Microcontrôleur**         | Arduino Mega + SE (Smartphone)                 | Gestion des capteurs (Mega) et traitement centralisé (SE).                                  |
| **Caméra**                  | Caméras du SE                                   | Reconnaissance de forme via OpenCV sur SE.                                                   |
| **Microphones**             | 3 micros                                        | Localisation des sons.                                                                       |
| **Capteurs Ultrason**       | 9 capteurs (HC-SR04)                            | Détection d'obstacles.                                                                       |
| **LED**                     | Ring de LED + Ruban de LED                      | Communication visuelle avancée.                                                             |
| **Enceinte**                | Enceinte JBL Bluetooth                         | Qualité audio améliorée.                                                                    |
| **Communication**           | MQTT, Wi-Fi, USB-UART                           | Suppression de l'I2C et du Bluetooth (sauf pour l'enceinte JBL).                             |
| **Alimentation**            | Batterie plomb 12V                              | Évolution prévue vers Li-Ion.                                                                |

---

## **4. Modes de fonctionnement (V1)**
- **Autonome passif/actif** :
  - Surveillance via capteurs et actionneurs (suivi de pilote, évitement d'obstacles).
- **Piloté/Télé-présence** :
  - Connexion automatique vers **Système Pilote (SP)** via Internet.

---

## **5. Protocoles de communication (V1)**
| **Protocole**       | **Utilisation**                                                                                     |
|---------------------|-----------------------------------------------------------------------------------------------------|
| **MQTT**            | Échanges entre SE et Arduino.                                                                      |
| **USB-UART**        | Communication SE ↔ Arduino (plus complexe que RPI ↔ Arduino).                                      |
| **Wi-Fi**           | Transmission des données des capteurs.                                                             |

---

## **6. Organisation logicielle (V1)**
### **6.1. Structure des programmes**
- **Arduino** :
  - Code factorisé en fichiers `.ino`, `.h`, et `.cpp`.
  - Exemple :
    ```cpp
    // Exemple de gestion des capteurs ultrason (V1)
    #include "UltrasonicSensor.h"
    UltrasonicSensor us[9];
    void loop() {
      for (int i = 0; i < 9; i++) {
        Serial.print("\$US,");
        Serial.print(i);
        Serial.print(",DIST,");
        Serial.print(us[i].readDistance());
        Serial.println("#");
      }
    }
    ```

### **6.2. Node-RED (SE)**
- **Variables globales** :
  - Définies dans `encryptedVars.json` :
    ```json
    {
      "US": {
        "instances": [
          {"id": 1, "properties": {"DIST": {"type": "int", "default": 0}}}
        ]
      }
    }
    ```

---

## **7. Structure du dépôt Git `RobotRZ`**

```bash
RobotRZ/
│   .gitignore
│   structure.md
│
├───.vscode/
│       settings.json
│
├───CODE/
│   ├───arduino/
│   │   ├───RZlibrairiesPerso/
│   │   │   │   library.properties
│   │   │   │
│   │   │   └───src/
│   │   │           communication.cpp
│   │   │           communication.h
│   │   │           config.cpp
│   │   │           config.h
│   │   │           emergency.cpp
│   │   │           emergency.h
│   │   │           force_sensor.cpp
│   │   │           force_sensor.h
│   │   │           ir_motion.cpp
│   │   │           ir_motion.h
│   │   │           leds.cpp
│   │   │           leds.h
│   │   │           microphone_array.cpp
│   │   │           microphone_array.h
│   │   │           moteurs.cpp
│   │   │           moteurs.h
│   │   │           odometry.cpp
│   │   │           odometry.h
│   │   │           RZlibrairiesPerso.h
│   │   │           servos.cpp
│   │   │           servos.h
│   │   │           sound_tracker.cpp
│   │   │           sound_tracker.h
│   │   │           ultrasonic.cpp
│   │   │           ultrasonic.h
│   │   │
│   │   ├───RZmegaV4/
│   │   │       RZmegaV4.ino
│   │   │
│   │   └───RZunoV5/
│   │           RZunoV5.ino
│   │
│   ├───node-red/
│   │       Last-OldV-Node-Red-SP-Tout-flows-23082025.json
│   │       Node-Red-SP-Tout-flows-23082025.json
│   │       Node-Red-SP-Tout-flows-29082025.json
│   │
│   ├───python/
│   │       A lire python.txt
│   │       ball_tracking_vseven.py
│   │       xboxok9.py
│   │
│   └───tasker/
│       │   A Lire SE pour smartphone embarqué _ SP pour smartphone pilote.txt
│       │   script_Tasker_Alire.txt
│       │
│       ├───RZ-SE-Tasker-Doro/
│       │       2_Valide_Connexion.tsk.xml
│       │       CommandeVocale.prj.xml
│       │       NewRz.prj.xml
│       │       RZ_Projet.prj.xml
│       │
│       │   └───Doro 8062-20250823T212341Z-1-001/
│       │           backup.xml
│       │
│       └───RZ-SP-Tasker-Xiaomi/
│               backup.xml
│
└───DOC/
    ├───00_PROJET/
    │       ModeFonctionnement-RZ-Miro.jpg
    │       README_Projet_RobotZ_V1.md
    │       Robot Zv13.docx
    │       RobotZ Version V0.md
    │       syntheseNewRobot.odt
    │       TypeContenuProjet.txt
    │
    ├───01_ARCHITECTURE/
    │       Communication.md
    │       CommunicationSE_SP.odt
    │       DiagrapProjetRobotNewV1Fok.drawio
    │       DiagrapProjetRobotNewV1Fok.drawio.png
    │       DiagrapRZCom-Protocole-Appli.drawio
    │       DiagrapRZCom-Protocole-Appli.drawio.png
    │       Installation.odt
    │       Listes-orga-code-var-pin-RZ-New.xlsx
    │       Listes-orgaArduino-code-pin-cable.xlsx
    │       robotrzfritzingv10.fzz
    │       robotrzfritzingvjuillet2024.fzz
    │       Rz old-organisation-initiale.mm
    │       Rz organisation.html
    │       RZ-fritzingV10_Old-circuitElec.jpg
    │       TypeContenuArchitecture.txt
    │       Vinitial-schemaFonctionnement.odg
    │
    │   └───AideNoteAppliUtilisé/
    │           RzCamera.docx
    │           Synthese_Dashboard_Robot_NodeRED.docx
    │
    ├───02_ARDUINO/
    │       a lire.txt
    │       decoupageOldCodeArduino.odt
    │       notes2.odt
    │       README_PlatformIO_Migration_V1.md
    │       Ressource-FactorisatonArduino.odt
    │       RobotZ_Documentation.md
    │       RobotZ_Documentation.odt
    │
    │   └───Capteurs/
    │           README_IR_V1.md
    │
    ├───03_NODE-RED/
    │       node-red_flowfuse_dynamic_ui_management_v1.md
    │       Ressource-New-node-red.odt
    │
    ├───04_TASKER/
    ├───05_OPERATION/
    │       A Lire - Aide utilisateurs.txt
    │       RzAide.odt
    │       RZaideVN.odt
    │
    │   └───SSH/
    │           README_SSH_V1.md
    │
    ├───06_Application-Tiers/
    │       README_VSCode_Integration_V1.md
    │
    └───99_ARCHIVES/
