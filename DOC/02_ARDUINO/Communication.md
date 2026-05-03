# [Arduino] - Optimisation des communications série et gestion centralisée des erreurs pour le robot Z (V1)

---

## **Contexte et objectif**
Ce document décrit les optimisations apportées aux **communications série** entre Arduino et Node-RED pour le projet de robot Z. L'objectif principal est d'assurer une **transmission fiable des données** tout en intégrant un **système de gestion centralisée des erreurs** (débordements de buffer, erreurs de parsing) via le module `emergency.h`.

---

## **Partie générale**

### **Concepts clés et outils**
- **Protocole VPIV** :
  Format personnalisé pour échanger des données structurées entre Arduino et Node-RED.
  Exemple de message : `$V:US:U,3,D,45#` (Type:V, Variable:U, Instances:3, Propriété:D, Valeur:45).

- **Gestion des erreurs** :
  Utilisation de `sscanf` pour le parsing des données et vérification des retours pour éviter les comportements imprévisibles.
  Détection des **débordements de buffer** et des **erreurs de format** (ex: comparaison incorrecte de `char` avec des chaînes littérales).

- **Bibliothèques et fonctions C/C++** :
  - `string.h` : Pour les manipulations sécurisées de chaînes (`strncpy`, `memset`, `strcmp`).
  - `sscanf` : Pour lire des données formatées depuis une chaîne de caractères.

### **Outils et technologies utilisés**
| Catégorie       | Détails                                                                 |
|-----------------|-------------------------------------------------------------------------|
| **Matériel**    | Arduino Mega, capteurs (ultrasons, IR, etc.).                          |
| **Logiciel**    | IDE Arduino 2.3.6, bibliothèques personnalisées (`RZlibrairiesPerso`). |
| **Protocole**   | Communication série personnalisée (format `VPIV`).                     |

---

## **Partie spécifique au projet**

### **Structure des messages**
La structure `MessageVPIV` est définie dans `communication.h` pour stocker les données reçues :

```cpp
struct MessageVPIV {
    char type;                     // Type de message (I, F, V, E)
    char variable;                 // Variable (ex: U pour ultrason)
    char properties[MAX_PROPERTIES]; // Propriétés (ex: D, C)
    char values[MAX_PROPERTIES][MAX_INSTANCES][MAX_VALUE_LEN]; // Valeurs associées
    uint8_t instances[MAX_INSTANCES]; // Instances concernées
    int nbProperties;              // Nombre de propriétés
    int nbInstances;               // Nombre d'instances
    bool allInstances;             // true si "*" (toutes les instances)
};
