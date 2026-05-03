## **Module Urg (urgence système)**

A revérifier et complété.

### **Rôle**

Gestion centralisée des états d’urgence critiques, côté Arduino (A) et côté Smartphone (SE). Une urgence bloque toute action nécessitant sécurité (moteurs, émissions) jusqu’à effacement explicite par SP.

### **Architecture**

- **Côté A** : `urg.cpp`, `dispatch\_Urg.cpp`

- **Côté SE** : `urgence.sh` (déclenchement), `alerts\_sys.sh` (détection seuils), intégré au monitor système.

### **Fonctionnement**

- **Déclenchement** : toujours interne (jamais via VPIV).

  - A : `URG\_LOW\_BAT`, `URG\_MOTOR\_STALL`, `URG\_SENSOR\_FAIL`, `URG\_BUFFER\_OVERFLOW`, `URG\_PARSING\_ERROR`, `URG\_US\_DANGER`, `URG\_MVT\_DANGER`, `URG\_LOOP\_TOO\_SLOW\_A`.

  - SE : `URG\_CPU\_CHARGE`, `URG\_OVERHEAT`, `URG\_SENSOR\_FAIL` (storage/mem), `URG\_LOW\_BAT`, `URG\_LOOP\_TOO\_SLOW\_SE`.

- **Effacement** : uniquement par commande SP :

  - `$V:Urg:\*:clear:1\#` (A)

  - `$V:Urg:clear:\*:1\#` (SE)

- **Comportement pendant urgence** :

  - A : arrêt immédiat des moteurs (`urg\_stopAllMotors()`), publications d’urgence (`$E:Urg:source`, `$E:Urg:etat`).

  - SE : arrêt des publications de données (flux), poursuite de la surveillance passive (`global.json`). Le script attend un nouveau `modeSys` de SP pour reprendre.

### **Cycle de vie**

- **A** : `urg\_init()` au démarrage ; `urg\_handle()` appelé par les modules détecteurs ; `urg\_clear()` via dispatcher.

- **SE** : `trigger\_urgence()` appelé par `alerts\_sys.sh` ; le monitor entre en boucle d’attente après urgence.

### **Boucle principale**

- A : `urg\_isActive()` consulté par tous les modules critiques avant d’exécuter des actions.

- SE : le monitor continue à mettre à jour `global.json` mais n’émet aucune trame tant que `urgence\_active` est vrai.

### **Propriétés VPIV (extrait table A)**

| **Direction** | **Instance** | **Propriété** | **CAT** | **Description** |
| :-: | :-: | :-: | :-: | :-: |
| SP → A | \* | `clear` | V | Efface l’urgence active (A) |
| SP → SE | \* | `clear` | V | Efface l’urgence active (SE) |
| A → SP | A | `source` | E | Cause de l’urgence (enum) |
| A → SP | A | `etat` | E / I | `active` (E), `cleared` (I) |
| SE → SP | SE | `source` | E | Cause de l’urgence (enum) |
| SE → SP | \* | `statut` | I | `active` ou `cleared` |
| SE → SP | \* | `niveau` | I | `warn` ou `danger` |

### **États**

- **A** : `cfg\_urg\_active` (bool), `cfg\_urg\_code` (uint8\_t), `cfg\_urg\_timestamp` (ms)

- **SE** : variable `urgence\_active` dans le monitor, `global.json` (.Urg.etat, .Urg.source)

### **Sécurité**

- Arrêt moteur immédiat côté A.

- Blocage des publications SE (données flux) tant que l’urgence n’est pas effacée.

- L’effacement est une commande explicite SP ; pas de reprise automatique.

### **Paramètres internes (fixes)**

- A : codes d’urgence définis dans `enum UrgReason` (pas paramétrables par SP).

- SE : seuils paramétrables via `paraSys` (module Sys) – voir section suivante.

### **Remarques**

- Le module Urg est transversal. Côté SE, la détection d’urgence est intégrée au monitor système (`rz\_sys\_monitor.sh`) et utilise les seuils de `paraSys`.

- L’urgence est prioritaire : même si SP envoie des commandes, l’état actif bloque leur exécution (sauf `clear` qui est explicitement autorisé).

- Après `clear`, SP doit renvoyer les modes (par ex. `modeSys`, `modeMicsF`) pour relancer les traitements.

