# [VS Code] - Migration et intégration Arduino/Node-RED/Python/SSH (V1)

---

## **1. Contexte et Objectif**
Centraliser le développement d’un projet robotique (Arduino, Node-RED, Python, SSH) dans **Visual Studio Code** pour :
- Éviter la fragmentation des outils.
- Optimiser la productivité grâce à un workflow unifié.

---

## **2. Résumé Général : Outils et Concepts Clés**

### **Outils/Technologies Abordés**
| Outil/Technologie       | Rôle                                                                                     |
|-------------------------|------------------------------------------------------------------------------------------|
| **Visual Studio Code**  | Éditeur multi-plateforme (Windows, Linux, macOS) avec extensions pour tous les langages. |
| **PlatformIO**          | Remplace l’IDE Arduino classique : compilation, upload, et débogage avancé.               |
| **Remote-SSH**          | Extension pour se connecter à des machines distantes (Raspberry Pi, Termux sur smartphone). |
| **Node-RED**            | Édition des flows en JSON directement dans VS Code.                                      |
| **Python**              | Environnements virtuels, exécution et débogage intégrés.                                  |
| **Git**                 | Versioning et collaboration via l’extension Git intégrée.                                 |

### **Concepts Clés**
- **Multi-root workspaces** : Gestion de plusieurs dossiers de projet dans un seul espace de travail.
- **Automatisation** : Utilisation de `tasks.json` pour automatiser les tâches répétitives (compilation, upload, redémarrage de Node-RED).
- **Snippets** : Modèles de code réutilisables pour Arduino et Python.

---

## **3. Résumé Spécifique au Projet**

### **3.1. Décisions Techniques**
- **Migration des projets Arduino** :
  - Fichiers `.ino`, `.h`, `.cpp` migrés vers **PlatformIO** dans VS Code.
- **Configuration de Remote-SSH** :
  - Accès au **Raspberry Pi** et à **Termux** depuis VS Code.
- **Édition des flows Node-RED** :
  - Modification directe des fichiers JSON et automatisation du redéploiement.
- **Structuration du workspace** :
  - Dossiers dédiés : `Arduino/`, `Node-RED/`, `Python/`, `Documentation/`.

### **3.2. Fichiers Concernés**
| Fichier                  | Rôle                                                                                     |
|--------------------------|------------------------------------------------------------------------------------------|
| `platformio.ini`         | Configuration des cartes Arduino et dépendances.                                          |
| `tasks.json`             | Automatisation des tâches (compilation, upload, redémarrage de Node-RED).                  |
| `requirements.txt`       | Gestion des dépendances Python.                                                          |
| Fichiers JSON Node-RED   | Définition des flows pour l’échange de données.                                          |

### **3.3. Problèmes et Solutions**
| **Problème**                          | **Solution**                                                                                     |
|---------------------------------------|-------------------------------------------------------------------------------------------------|
| Bibliothèques Arduino manquantes      | Utilisation du gestionnaire de bibliothèques de **PlatformIO**.                              |
| Redémarrage manuel de Node-RED        | Script Python + tâche VS Code pour **automatiser le redémarrage**.                           |

### **3.4. Évolutions Possibles**
- Intégration de **GitHub Actions** pour le déploiement continu.
- Utilisation de **Docker** pour uniformiser les environnements de développement.

---

## **4. Liens et Références**
### **Documentations Officielles**
- [VS Code Docs](https://code.visualstudio.com/docs)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Node-RED Docs](https://nodered.org/docs/)

### **Tutoriels Recommandés**
- [PlatformIO + VS Code pour Arduino](https://docs.platformio.org/en/latest/ide/vscode.html)
- [Remote-SSH avec VS Code](https://code.visualstudio.com/docs/remote/ssh)

### **Extensions VS Code Utilisées**
- PlatformIO IDE, Remote-SSH, Python, Node-RED, GitLens.

---

## **5. Répartition Thématique**
- **40% [Application Tiers]** : VS Code, workflows.
- **30% [Arduino]** : PlatformIO, migration.
- **20% [Node-RED]** : Édition des flows.
- **10% [Python]** : Environnements virtuels, scripts.

---
