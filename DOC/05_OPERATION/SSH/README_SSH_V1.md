# [SSH] - Résolution des problèmes de connexion entre un PC et des smartphones (V1)

---

## **1. Résumé Général : SSH et son utilité**
**SSH (Secure Shell)** est un protocole utilisé pour se connecter de manière sécurisée à un appareil distant via une **clé privée et publique**. Il est largement utilisé pour :
- **Administrer des appareils à distance** (smartphones, serveurs, etc.).
- **Exécuter des commandes** ou **gérer des fichiers** de manière sécurisée.

**Dans ce contexte** :
- Permet à l’utilisateur de se connecter depuis un **PC vers des smartphones** pour effectuer des tâches à distance (ex : gestion de fichiers, exécution de commandes).

**Outils utilisés** :
- **OpenSSH** (client SSH sur Windows).
- **Clés SSH** (`id_rsa` et `id_rsa.pub`) pour l’authentification.

---

## **2. Résumé Spécifique au Projet**

### **2.1. Configuration Initiale**
- **Chemin de la clé privée** :
  `C:\Users\Vincent.VINCENTPHACER\.ssh\id_rsa`
- **Commande SSH utilisée** :
  ```bash
  ssh -i %SSH_KEY% u0_a154@192.168.1.12 -p 8022
