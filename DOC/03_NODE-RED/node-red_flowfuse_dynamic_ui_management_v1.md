# [Node-RED] - Gestion des variables dynamiques et création de formulaires/tableaux éditables avec FlowFuse (V1)

---

## **Contexte/Objectif**
Ce chat traite de la création d'un **tableau dynamique** et d'un **formulaire éditable** dans **Node-RED** en utilisant **FlowFuse Dashboard 2.0**. L'objectif était de gérer des variables stockées dans un fichier JSON (`enrichedVars`), permettant la sélection, l'édition et la sauvegarde des propriétés de ces variables, tout en gardant le flux clair et maintenable.

---

## **Résumé général**

### **Outils et technologies utilisés**
- **Node-RED** : Environnement de développement visuel pour connecter des flux de données.
- **FlowFuse Dashboard 2.0** : Pour créer des interfaces utilisateur dynamiques (tableaux, formulaires).
- **Nœuds clés** :
  - `ui-template` : Pour créer des interfaces personnalisées en HTML/JavaScript.
  - `ui-table` : Pour afficher des données sous forme de tableau.
  - `ui-form` : Pour créer des formulaires interactifs.
  - `link in`/`link out` : Pour centraliser la gestion des messages.
  - `subflow` : Pour regrouper et réutiliser des parties de flux.
- **Bibliothèques externes** :
  - [Tabulator](http://tabulator.info/) : Pour créer des tableaux éditables avancés.

### **Concepts clés**
- **Gestion des variables dynamiques** : Utilisation de `global.get` et `global.set` pour manipuler des variables stockées dans un fichier.
- **Tables et formulaires dynamiques** : Génération de champs ou de colonnes en fonction des données reçues.
- **Centralisation des messages** : Utilisation de nœuds `link` et `switch` pour diriger les messages vers un nœud `ui-text` unique.

---

## **Résumé spécifique au projet**

### **Étapes et solutions techniques**

#### **1. Extraction des propriétés des variables**
- Utilisation d'une fonction pour extraire les propriétés d'une variable sélectionnée (ex: `leds`) à partir de `enrichedVars`.
- Structure des données :
  - `proprietesByInstance` : Pour les propriétés.
  - `valeurs` : Pour les valeurs associées aux instances.

#### **2. Création d'un tableau dynamique**
- Utilisation du nœud `ui-template` avec la bibliothèque **Tabulator** pour créer une table éditable.
- Exemple de code pour générer dynamiquement les colonnes et les lignes en fonction des données.

#### **3. Création d'un formulaire dynamique**
- Utilisation de `ui-template` pour générer un formulaire en fonction des propriétés et des instances.
- Exemple de code pour créer des champs dynamiques en fonction du nombre d'instances.

#### **4. Centralisation des messages d'information**
- Utilisation de nœuds `link in`/`link out` et `switch` pour diriger les messages vers un nœud `ui-text` unique.
- Exemple de structure pour gérer les messages en fonction de `msg.topic` ou `msg.info`.

### **Décisions et évolutions possibles**
- **Utilisation de sous-flux** : Pour regrouper les fonctions réutilisables et simplifier le flux principal.
- **Amélioration de la clarté du flux** : Utilisation de groupes, de couleurs et de commentaires pour organiser visuellement le flux.
- **Perspectives** : Intégration de nouvelles fonctionnalités comme l'ajout/suppression dynamique de variables ou d'instances.

---

## **Liens et références**
- **Documentation FlowFuse** :
  - [ui-table](https://flows.nodered.org/node/node-red-node-ui-table)
  - [ui-template](https://flowfuse.com/node-red/core-nodes/template/)
  - [ui-form](https://dashboard.flowfuse.com/nodes/widgets/ui-form.html)
- **Exemples de flux** :
  - [Dynamic tables using template](https://flows.nodered.org/flow/8a0959d742e9af9517312b2f8b44e500)
  - [Editable table (CRUD) using flow context](https://flows.nodered.org/flow/8635c45d4f54e160649c7b00a075a350)
- **Bibliothèques externes** :
  - [Tabulator](http://tabulator.info/)

---

## **Proposition de répartition**
- **100% [Node-RED]** : Ce chat est entièrement centré sur l'utilisation de Node-RED et FlowFuse pour gérer des variables dynamiques et créer des interfaces utilisateur.

---

## **Changelog**
- **V1** : Version initiale couvrant la gestion des variables dynamiques, la création de tableaux et formulaires éditables, et la centralisation des messages.

---
