# ⚙️ Synthèse du projet — Communication Arduino ↔ Smartphone via protocole VPIV

## 🔸 Contexte général

Ton projet concerne un **robot piloté via un smartphone (SP)** utilisant **Node-RED**.  
Le robot embarque un **deuxième smartphone (SE)** connecté à une **carte Arduino Mega** via **USB**, qui sert d’interface capteurs/actionneurs.

Les deux smartphones communiquent entre eux par **MQTT** :
- **SP (pilote)** → Node-RED (gestion, visualisation, ordres)
- **SE (embarqué)** → reçoit les ordres de SP, relaie vers Arduino, publie les retours capteurs

Le rôle de l’Arduino est donc :
1. **Recevoir des messages** du SE via USB série  
2. **Décoder** un protocole texte compact appelé **VPIV**  
3. **Exécuter** les actions correspondantes (LED, servo, moteur, etc.)  
4. **Répondre** via des messages codés au même format

---

## 🔸 Structure du projet Arduino

Arborescence simplifiée :

```
arduino/
 ├── RZmegaV4.ino              → programme principal
 ├── RZlibrairiesPerso/
 │   ├── library.properties
 │   └── src/
 │        ├── RZlibrairiesPerso.h
 │        ├── communication.cpp / .h
 │        ├── config.cpp / .h
 │        ├── emergency.cpp / .h
 │        ├── leds.*, servos.*, moteurs.*, etc.
```

### Enchaînement des inclusions
```
RZlibrairiesPerso.h
 └── config.h         → constantes globales
 └── communication.h  → protocole VPIV
 └── autres modules   → leds, servos, moteurs, capteurs, etc.
```

Le **fichier principal `RZmegaV4.ino`** :
- appelle la boucle de réception (`recvSeWithStartEndMarkers()`)
- traite les données reçues (`SeTraitementNewV4()`)

---

## 🔸 Format du protocole VPIV

Le protocole VPIV est un **format textuel minimaliste** pour échanger des données structurées sur port série.

### Structure d’un message
```
$<type>:<contenu>:<variable>,<instances>,<propriétés>,<valeurs>#
```

### Marqueurs
- `$` → début du message  
- `#` → fin du message  
- `:` → séparation des grandes sections  
- `,` → séparation d’éléments dans une section  

### Exemple
```
$V::D,*,C,255,128,64#
```

| Élément | Signification |
|----------|----------------|
| `$` / `#` | Délimiteurs de message |
| `V` | Type (Variable) |
| `D` | Variable (ex : LED) |
| `*` | Instance (toutes) |
| `C` | Propriété (Couleur) |
| `255,128,64` | Valeur (RVB) |

---

## 🔸 Types de message existants

| Type | Signification | Usage |
|------|----------------|-------|
| `V`  | Variable | Mise à jour ou lecture de valeur |
| `I`  | Information | Log, accusé de réception, réponse |
| `F`  | Fonction | Exécution d’une action spécifique |
| `E`  | Erreur | Signal d’anomalie ou d’alerte |

> 💡 **Évolution prévue** : typage enrichi (`b`, `s`, `i`, `o`, `t`) pour mieux décrire le contenu des variables.

---

## 🔸 Fonctionnement du module `communication.cpp`

### 🧱 Objectif
Gérer la réception, le décodage et le traitement des messages VPIV échangés sur le port série.

---

### 1. Réception série — `recvSeWithStartEndMarkers()`

**Rôle :**
- Lire les caractères arrivant sur `Serial`
- Reconnaître un message complet (`$ ... #`)
- Appeler `parseVPIV()` une fois le message reçu

**Sécurité :**
- Vérifie la taille du buffer pour éviter débordement
- Déclenche `handleEmergency(EMERGENCY_BUFFER_OVERFLOW)` si dépassement

---

### 2. Décodage du message — `parseVPIV()`

**Rôle :**
- Extraire chaque champ du message selon les séparateurs `: , #`
- Remplir la structure `MessageVPIV`

**Structure :**
```cpp
typedef struct {
  char type;
  char variable;
  bool allInstances;
  int instances[MAX_INSTANCES];
  char properties[MAX_PROPERTIES];
  char values[MAX_PROPERTIES][MAX_INSTANCES][MAX_VALUE_LEN];
  int nbInstances;
  int nbProperties;
} MessageVPIV;
```

**Comportement :**
- `*` → toutes les instances (`allInstances = true`)
- chiffre → identifiant d’instance
- lettre → propriété
- autre → valeur associée à la propriété courante

---

### 3. Traitement du message — `SeTraitementNewV4()`

**Rôle :**
Appliquer une action selon la variable reçue.

**Exemple intégré :** gestion d’une LED RGB

```cpp
if (lastMessage.variable == 'D') {
  for (int p = 0; p < lastMessage.nbProperties; p++) {
    if (lastMessage.properties[p] == 'C') {
      int r,g,b;
      if (sscanf(lastMessage.values[p][0], "%d,%d,%d", &r,&g,&b) != 3) {
        handleEmergency(EMERGENCY_SENSOR_FAIL);
        sendError("Format couleur invalide", "LED", lastMessage.values[p][0]);
        return;
      }
      // Application des valeurs r,g,b à la LED
    }
  }
}
newData = false;
```

---

### 4. Envoi de réponses

#### a) `sendAck()`
Réponse d’accusé de réception :
```
$I:ACK:message#
```

#### b) `sendError()`
Message d’erreur formaté :
```
$E:erreur:module,,,détail#
```

---

## 🔸 Liaison avec Node-RED

- **Côté SE (smartphone embarqué)** :
  - communique via USB série avec l’Arduino  
  - traduit les trames série en messages MQTT

- **Côté SP (smartphone pilote)** :
  - utilise Node-RED pour :
    - recevoir, afficher et traiter les messages
    - piloter le robot via interface graphique
    - transmettre des ordres encodés VPIV

**Structure côté Node-RED :**
```js
{
  variable: "ledring",
  instances: ["*"],
  proprietes: ["z"],
  valeurs: [["128"]],
  type: "V"
}
```

---

## 🔸 Exemple complet d’échange

### 🔹 Commande envoyée
```
$V::D,*,C,255,128,64#
```

### 🔹 Arduino décode
- Type : `V`
- Variable : `D`
- Propriété : `C` (Couleur)
- Valeur : `255,128,64`

### 🔹 Arduino agit
- Modifie la LED RGB correspondante

### 🔹 Arduino répond
```
$I:ACK:LED couleur mise à jour#
```

---

## 🔸 Gestion des erreurs

Les erreurs critiques sont centralisées dans `emergency.cpp` :
- `EMERGENCY_BUFFER_OVERFLOW` → dépassement du tampon série
- `EMERGENCY_SENSOR_FAIL` → format de valeur incorrect
- Autres selon le contexte matériel

Chaque erreur déclenche :
- Un envoi `$E:` sur le port série
- Une réaction interne (log, clignotement, etc.)

---

## 🔸 Points forts du protocole VPIV

✅ Compact et rapide à parser  
✅ Robuste et lisible (délimiteurs clairs)  
✅ Faible empreinte mémoire (pas de `String`)  
✅ Extensible (nouvelles propriétés et types)  
✅ Compatible avec MQTT / Node-RED  
✅ Gestion intégrée des erreurs

---

## 🔸 Exemple d’intégration dans la boucle principale

```cpp
void loop() {
  recvSeWithStartEndMarkers();
  SeTraitementNewV4();
}
```

---

## 🔸 Évolutions prévues

- [ ] Typage étendu des variables (`b`, `s`, `i`, `o`, `t`)
- [ ] Gestion d’ensembles d’instances complexes
- [ ] Accusés de réception enrichis (timestamp, CRC)
- [ ] Compression ou encodage base64 pour longs messages
- [ ] Décodage natif JSON côté Node-RED

---

## 🔸 Résumé

| Fonction | Rôle |
|-----------|------|
| `recvSeWithStartEndMarkers()` | Lecture série et détection de message complet |
| `parseVPIV()` | Décodage du protocole VPIV |
| `SeTraitementNewV4()` | Application des actions selon variable/propriété |
| `sendAck()` | Envoi d’un accusé de réception |
| `sendError()` | Transmission d’un message d’erreur |

---

✳️ *Synthèse réalisée par ChatGPT (GPT-5), octobre 2025 – adaptée à ton projet RZmegaV4 / Node-RED / Arduino Mega.*
