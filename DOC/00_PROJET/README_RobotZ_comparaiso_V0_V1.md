
---

### **3. Fichier `RobotZ_Evolution_V0_V1.md`**
```markdown
# **Robot Z – Comparaison V0 vs V1 et contexte d'évolution**

---
## **1. Résumé des évolutions**
| **Élément**                | **V0**                                      | **V1**                                      | **Évolution**                                                                 |
|----------------------------|---------------------------------------------|---------------------------------------------|-------------------------------------------------------------------------------|
| **Architecture**           | Raspberry Pi + Arduino Mega                 | SE (Smartphone) + Arduino Mega              | Simplification et centralisation sur SE.                                    |
| **Reconnaissance de forme**| Caméra RPI + OpenCV                         | Caméras du SE + OpenCV                      | Suppression du matériel dédié (RPI).                                         |
| **Microphones**            | 1 micro                                     | 3 micros                                    | Ajout pour localisation des sons.                                            |
| **LED**                    | Ring de LED + LED RGB                       | Ring de LED + Ruban de LED                  | Communication lumineuse améliorée.                                           |
| **Écran LCD**              | Écran LCD (I2C)                             | Supprimé                                   | Simplification.                                                              |
| **Protocoles**             | Bluetooth, I2C, USB-UART, Wi-Fi             | MQTT, Wi-Fi, USB-UART                       | Suppression de l'I2C et simplification des échanges.                         |
| **Programmes**             | Monolithique (.ino)                         | Factorisé (.ino, .h, .cpp)                  | Meilleure maintenabilité et modularité.                                      |
| **Encodage des données**   | Format personnalisé                         | Format uniformisé (`$Var,Instance,Prop,Valeur#`) | Standardisation pour Node-RED.                                               |

---
## **2. Contexte pour l'évolution des programmes**
### **2.1. Transition matérielle → logicielle**
- **Suppression du Raspberry Pi** :
  - **Impact** : Tous les traitements (OpenCV, IA) sont désormais réalisés sur le SE.
  - **Adaptation** : Réécriture des scripts Python (OpenCV) pour Android/iOS.
- **Nouveau protocole** :
  - **Avantage** : Uniformisation des messages pour Node-RED et facilité de paramétrage.
  - **Exemple** :
    ```json
    // Extrait de encryptedVars.json (V1)
    {
      "US": {
        "instances": [
          {"id": 1, "properties": {"Distance": {"type": "int", "default": 0}}}
        ]
      }
    }
    ```

### **2.2. Recommandations pour la migration**
1. **Adapter les flux Node-RED** :
   - Utiliser le fichier `encryptedVars.json` pour définir les variables globales.
2. **Factoriser le code Arduino** :
   - Séparer la logique des capteurs (`UltrasonicSensor.h`) et des actionneurs (`Actuators.h`).
3. **Optimiser la communication SE ↔ Arduino** :
   - Utiliser des bibliothèques comme **SerialCommand** pour parser les messages `$Var,...#`.

---
## **3. Prochaines étapes**
- **Documenter** :
  - Les nouvelles classes Arduino (ex: `UltrasonicSensor`).
  - Les flux Node-RED pour le traitement des données.
- **Tester** :
  - La localisation des sons avec les 3 micros.
  - La "laisse électronique" avec le capteur de force.
