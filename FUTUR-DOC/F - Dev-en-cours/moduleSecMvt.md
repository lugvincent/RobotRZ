**Détail module SecMvt**

Bonne question — c’est **un point clé de ton architecture sécurité**.  
Je te fais une explication **claire, concrète et alignée avec ton système**.


# 🛑 Modes `idle / soft / hard` — Module **SecMvt**

## 🎯 1. Rôle global

Le module **SecMvt (mvtSafe)** agit comme un **filtre de sécurité entre les capteurs (US) et les moteurs (MTR)**.

👉 Il décide :

- si on laisse passer la commande

- si on la réduit

- ou si on bloque tout


## ⚙️ 2. Les 3 modes

## 🟢 `idle` — Mode inactif

👉 **Aucune sécurité appliquée**

- SecMvt est **désactivé**

- Les commandes MTR passent **sans modification**

### ✔ Comportement

- vitesse normale

- aucune limitation

- aucun arrêt automatique

### ⚠️ Risque

- collision possible

### 🧠 Usage typique

- tests

- debug

- environnement maîtrisé


## 🟡 `soft` — Mode préventif (intelligent)

👉 **Réduction progressive de la vitesse**

Le module :

- surveille les distances US

- calcule un **facteur de réduction (`scale`)**

- limite la vitesse envoyée à MTR


### 🔧 Principe

```
`distance ↓  →  scale ↓  →  vitesse ↓`
```

Exemple :

| **Distance** | **Scale** | **Effet** |
| :-: | :-: | :-: |
| 100 cm | 1.0 | vitesse normale |
| 50 cm | 0.6 | ralenti |
| 20 cm | 0.2 | très lent |


### ✔ Comportement

- mouvement fluide

- ralentissement naturel

- pas d’arrêt brutal

### 🧠 Usage

- navigation normale

- interaction humaine

- confort utilisateur


## 🔴 `hard` — Mode sécurité critique

👉 **Arrêt immédiat + déclenchement URG**

Dès qu’un seuil est dépassé :

- arrêt moteur instantané

- déclenchement **URG**

- blocage du système


### 🔧 Principe

```
`distance \< seuil critique → STOP → URG`
```


### ✔ Comportement

- coupure nette

- priorité absolue

- système verrouillé


### 🔗 Interaction avec URG

- appel à `urg\_handle(code)`

- LED ring activé

- seules commandes `clear` autorisées


### 🧠 Usage

- sécurité physique

- protection matériel

- présence humaine proche


## 📊 3. Comparaison rapide

| **Mode** | **Effet** | **Action** | **URG** |
| :-: | :-: | :-: | :-: |
| idle | rien | aucune | ❌ |
| soft | ralentit | scaling | ❌ |
| hard | stop | arrêt total | ✅ |


## 🔁 4. Position dans la chaîne de commande

```
`Commande → MTR → SecMvt → (modifiée ou bloquée) → Hardware`
```

👉 SecMvt est un **intercepteur**, pas un générateur.


## ⚠️ 5. Point important (architecture validée)

- `soft` agit **AVANT MTR**

- `hard` déclenche **URG (niveau système)**

👉 Donc :

- `soft` = correction locale

- `hard` = sécurité globale


## 🧠 6. Lecture “intention système”

- `idle` → “je fais confiance”

- `soft` → “je m’adapte”

- `hard` → “je protège”


## ✅ Conclusion

Le triplet :

```
`idle / soft / hard`
```

n’est pas juste un paramètre — c’est :

👉 **le niveau de sécurité du robot**

- `idle` = OFF

- `soft` = intelligent

- `hard` = critique


## 👉 Si tu veux aller plus loin

Je peux te détailler :

- 🔧 le **calcul exact du scale** (fonction distance → vitesse)

- 🔗 le **flux complet US → SecMvt → MTR → URG**

- 🧪 ou te proposer un **réglage terrain optimal (seuils)**

Tu me dis 👍

