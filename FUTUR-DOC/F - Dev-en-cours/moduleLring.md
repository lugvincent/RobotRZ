Module Lring

Voici une **présentation propre, homogène et directement intégrable dans ton livrable** du module **Lring**, structurée sur ~3 pages avec un tableau clair des propriétés. Srce chatGPT


# VIII.X — Module **Lring** (Anneau LED)


## 1. Rôle et positionnement du module

Le module **Lring** assure la **communication visuelle du robot** via un anneau de LEDs adressables.

Il remplit trois fonctions principales :

- **Expression d’état** (mode interne)

- **Affichage piloté** (mode externe via Node-RED / SP)

- **Signalisation d’urgence** (priorité absolue)


### 🧠 Logique fonctionnelle

Le module fonctionne selon une logique de **modes exclusifs** :

| **Mode** | **Description** |
| :-: | :-: |
| `EXPR` | Expressions internes (neutre, sourire, etc.) |
| `EXTERN` | Contrôle direct LED (RGB) |
| `EXPR\_FORCED` | Mode urgence (override total) |


### ⚠️ Règles fondamentales

- Toute commande `expr` → force le mode **EXPR**

- Toute commande `rgb` / `clr` → force le mode **EXTERN**

- Une urgence active → **bloque tout** (override)

- Le module peut revenir automatiquement à `neutre`


## 2. Architecture interne

Le module repose sur trois couches :

### 2.1 Couche logique

- Gestion des expressions (`curExpr`)

- Gestion des modes (`modeExtern`, `exprForcedByUrg`)

- Timeout et clignotement


### 2.2 Couche comportementale

- Mapping nom → expression (`\_exprFromName`)

- Gestion des patterns lumineux :

  - neutre

  - sourire

  - triste

  - voyant

  - urgence


### 2.3 Couche hardware

Interface avec :

```
`lringhw\_setPixel()`

`lringhw\_fill()`

`lringhw\_clear()`

`lringhw\_show()`
```


## 3. Gestion des modes

### 3.1 Mode EXPR (interne)

- Piloté par `expr`

- Animations prédéfinies

- Peut inclure clignotement et timeout


### 3.2 Mode EXTERN

- Piloté par `rgb`, `clr`

- Contrôle direct LED

- Prioritaire sur EXPR (hors urgence)


### 3.3 Mode URGENCE (EXPR\_FORCED)

Déclenché par :

```
`lring\_emergencyTrigger(code)`
```

Effets :

- Clignotement rouge

- Évolution si urgence longue (\>10s)

- Ignore toutes les commandes


## 4. Propriétés VPIV exposées

### 📊 Tableau des propriétés

| **PROP** | **CAT** | **TYPE** | **DIR** | **INSTANCE** | **Rôle** | **Comportement** |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| `act` | I/V | bool | ↔ | \* / 0–11 | Activation | ON → voyant / OFF → neutre |
| `expr` | I/V | string | ↔ | \* | Expression | Force mode EXPR |
| `rgb` | I/V | string | ↔ | \* / idx | Couleur LED | Force mode EXTERN |
| `clr` | I/V | enum | ↔ | \* / idx | Effacement | LEDs à OFF |
| `lumin` | I/V | number | ↔ | \* / idx | Intensité luz | Global hardware |
| `mode` | I/V | enum | ↔ | \* | Mode | expr / extern |
| `init` | I/V | enum | ↔ | \* | Reset | Réinitialisation complète |
| `urg` | I | string | → | \* | État urgence | Notification visuelle |


## 5. Exemples VPIV

### Activation

```
`$I:Lring:\*:act:1\#`
```

→ Active affichage "voyant"


### Expression

```
`$I:Lring:\*:expr:sourire\#`
```

→ Affichage sourire


### RGB (LED spécifique)

```
`$I:Lring:3:rgb:255,0,0\#`
```

→ LED 3 en rouge


### Effacement

```
`$I:Lring:\*:clr:OK\#`
```

→ Éteint tout


### Mode externe

```
`$I:Lring:\*:mode:extern\#`
```

→ Autorise contrôle direct


## 6. Fonctionnement dynamique

### 6.1 Cycle périodique

Fonction :

```
`lring\_processPeriodic()`
```

Rôle :

- Gestion clignotement

- Gestion timeout expression

- Gestion urgence dynamique


### 6.2 Gestion du clignotement

- Voyant → 1 Hz

- Urgence → 2 Hz

- Urgence longue → mode dégradé (LED unique)


### 6.3 Timeout expression

```
`exprExpireAt`
```

Permet retour automatique à :

```
`neutre`
```


## 7. Gestion de l’urgence

### Déclenchement

```
`lring\_emergencyTrigger(code)`
```


### Effets

- Passage en `EXPR\_FORCED`

- Clignotement rouge

- Envoi VPIV :

```
`$I:Lring:\*:urg:\<code\>\#`
```


### Libération

Via module **Urg** :

```
`urg\_clear()`
```

→ retour à `neutre`


## 8. Points d’attention

### ⚠️ Cohérence INSTANCE

- `expr`, `mode`, `init` → toujours global (`\*`)

- `rgb`, `clr`, `int`, `act` → support index


### ⚠️ Intensité

- Actuellement globale (hardware)

- Instance ignorée


### ⚠️ Conflits

| **Cas** | **Résultat** |
| :-: | :-: |
| expr + rgb | rgb prend le contrôle |
| urgence + tout | urgence gagne |
| extern + expr | expr reprend main |


## 9. Synthèse

Le module **Lring** est :

- un **module hybride** (logique + visuel + sécurité)

- pilotable en **mode haut niveau (expr)** ou **bas niveau (rgb)**

- intégré au système d’**urgence globale**


### 💡 Point clé architecture

```
`Lring = Interface Homme-Robot visuelle + sécurité passive`
```


## 10. Recommandations

- Utiliser `expr` pour les états système

- Utiliser `rgb` uniquement en debug ou effets spécifiques

- Ne jamais piloter Lring en urgence côté SP


