# [Arduino] - Migration vers PlatformIO dans VS Code (V1)

---

## **1. Contexte et Objectif**
Migrer un projet Arduino existant (fichiers `.ino`, `.h`, `.cpp`) vers **PlatformIO dans VS Code** pour bénéficier :
- D’outils avancés (débogage, gestion des bibliothèques).
- D’une meilleure intégration avec Git et VS Code.
- D’une automatisation des tâches répétitives.

---

## **2. Résumé Général : Outils et Concepts Clés**

### **Outils/Technologies**
| Outil/Technologie  | Rôle                                                                                     |
|--------------------|------------------------------------------------------------------------------------------|
| **PlatformIO**     | Extension VS Code pour la gestion des projets Arduino/ESP32.                             |
| **VS Code**        | Éditeur de code avec IntelliSense, débogage, et intégration Git.                         |
| **Gestion des bibliothèques** | Installation et mise à jour centralisées via PlatformIO.                              |

### **Concepts Clés**
- **`platformio.ini`** : Configuration des cartes, frameworks, et options de compilation.
- **Débogage avancé** : Points d’arrêt, inspection des variables, monitoring série intégré.
- **Multi-plateforme** : Support Windows, Linux, macOS.

---

## **3. Résumé Spécifique au Projet**

### **3.1. Décisions Techniques**
- Remplacement de l’**IDE Arduino classique** par **PlatformIO** pour :
  - Une meilleure intégration avec **VS Code**.
  - Un débogage avancé et une gestion simplifiée des bibliothèques.
- Utilisation de **Git** pour le versioning des fichiers Arduino.
- Automatisation de la **compilation** et de l’**upload** via `tasks.json`.

### **3.2. Fichiers Concernés**
| Fichier               | Rôle                                                                                     |
|-----------------------|------------------------------------------------------------------------------------------|
| `platformio.ini`      | Configuration pour Arduino Uno/ESP32.                                                    |
| Fichiers `.h`/`.cpp`  | Code source du projet robotique.                                                         |
| `tasks.json`          | Automatisation des tâches de build/upload.                                               |

### **3.3. Problèmes et Solutions**
| **Problème**                          | **Solution**                                                                                     |
|---------------------------------------|-------------------------------------------------------------------------------------------------|
| Incompatibilité des bibliothèques    | Réinstallation des bibliothèques via **PlatformIO**.                                        |
| Débogage limité dans l’IDE Arduino    | Utilisation des outils de débogage avancés de **PlatformIO**.                                |

### **3.4. Évolutions Possibles**
- Intégration de **tests unitaires** pour le code Arduino.
- Utilisation de **CI/CD (GitHub Actions)** pour les builds automatiques.

---

## **4. Liens et Références**
### **Documentations**
- [PlatformIO Docs](https://docs.platformio.org/)
- [VS Code + Arduino](https://code.visualstudio.com/docs/arduino/arduino)

### **Tutoriels**
- [Getting Started with PlatformIO](https://docs.platformio.org/en/latest/ide/vscode.html)

### **Fichiers GitHub Associés**
- Exemple : [`02_ARDUINO/Capteurs/IR_V1.ino`](lien_vers_github)

---

## **5. Répartition Thématique**
- **90% [Arduino]** : PlatformIO, migration, débogage.
- **10% [Application Tiers]** : Intégration VS Code.
