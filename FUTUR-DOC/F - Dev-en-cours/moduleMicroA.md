## **Module Mic (microphone Arduino)**

### **Rôle**

Acquisition sonore sur 3 canaux (avant, gauche, droite) avec deux familles de modes indépendantes :

- **Flux** : publication périodique de données (niveau moyen, angle, crête)

- **Événements** : détection de bruit par niveaux (InfoNuisce) ou présence sonore (DetectPresence)

### **Architecture**

`dispatch\_Mic.cpp` → `mic.cpp` → `mic\_hardware.cpp` → `analogRead()`

### **Fonctionnement**

- L’échantillonnage hardware est permanent et non‑bloquant.

- **Mode flux (`modeMicsF`)** : un seul actif parmi `OFF`, `MOY` (niveau moyen RMS), `ANGLE` (direction du son), `PIC` (valeur crête).

  - Publication aux fréquences `freqMoy` / `freqPic` (ms).

  - Anti‑bruit : publication uniquement si la variation depuis la dernière valeur ≥ `deltaSeuil` (MOY/PIC) ou `deltaAngle` (ANGLE).

- **Mode événements (`modeMicsI`)** : exclusif parmi `OFF`, `InfoNuisce` (bruitNiv1, bruitNiv2), `DetectPresence`.

  - Détection soutenue : signalement après un temps continu dans la zone (`dureeNuisce` / `dureePresence`).

  - Cooldown : délai minimum entre deux signalements du même type (`cooldown`).

### **Cycle de vie**

- `mic\_init()` appelé au démarrage.

- `mic\_processPeriodic()` appelé à chaque tour de `loop()` : échantillonnage, publications flux, détection événements.

### **Boucle principale**

`mic\_processPeriodic()` doit être exécutée à chaque itération de la boucle principale.


### Propriétés VPIV

| **Direction** | **Propriété** | **CAT** | **Description** |
| :-: | :-: | :-: | :-: |
| SP → A | `modeMicsF` | V | Mode flux (`OFF`, `MOY`, `ANGLE`, `PIC`) |
| SP → A | `modeMicsI` | V | Mode événement (`OFF`, `InfoNuisce`, `DetectPresence`) |
| SP → A | `paraMicsF` | V | Paramètres flux (6 entiers CSV) |
| SP → A | `paraMicsI` | V | Paramètres détection (7 entiers CSV) |
| A → SP | `micMoy` | F | Niveau moyen RMS |
| A → SP | `micAngle` | F | Angle source sonore (‑180…+180°) |
| A → SP | `micPIC` | F | Valeur crête |
| A → SP | `bruitNiv1` | I | Bruit soutenu de niveau 1 |
| A → SP | `bruitNiv2` | I | Bruit soutenu de niveau 2 |
| A → SP | `detectPresenceSon` | I | Présence sonore détectée |
| A → SP | `modeMicsF`, `modeMicsI`, `paraMicsF`, `paraMicsI` | I | ACK de configuration |

### **États**

- `mic\_lastRMS\[3\]`, `mic\_lastPeak\[3\]` (hardware)

- `modeF`, `modeI`

- paramètres flux : `freqMoy`, `freqPic`, `winMoy`, `winPic`, `deltaSeuil`, `deltaAngle`

- paramètres événements : `seuil0Parasite`, `seuil1Bruit`, `seuil2Bruit`, `seuilPresence`, `dureeNuisce`, `dureePresence`, `cooldown`

- traqueurs de durée soutenue et cooldown par zone

### **Sécurité**

- Anti‑bruit sur les flux pour limiter les publications inutiles.

- Détection soutenue évite les faux positifs (claquements brefs).

- Cooldown évite les rafales de messages en cas de signal durable.

- Le niveau de référence (`niveauGlobal = moyenne des 3 RMS`) est cohérent entre flux et événements.


### Paramètres internes (valeurs par défaut)


```
`freqMoy = 200 ms      // cadence MOY/ANGLE`

`freqPic = 100 ms      // cadence PIC`

`winMoy = 200 ms       // fenêtre hardware MOY/ANGLE`

`winPic = 100 ms       // fenêtre hardware PIC`

`deltaSeuil = 5        // variation mini (ADC) pour MOY/PIC`

`deltaAngle = 10       // variation mini (°) pour ANGLE`


`seuil0Parasite = 30   // plancher matériel (ADC)`

`seuil1Bruit = 300     // seuil bruitNiv1 (ADC)`

`seuil2Bruit = 600     // seuil bruitNiv2 (ADC)`

`seuilPresence = 100   // seuil détection présence (ADC)`

`dureeNuisce = 500 ms  // durée soutenue InfoNuisce`

`dureePresence = 300 ms// durée soutenue DetectPresence`

cooldown = 2000 ms    // délai entre deux signalements
```

### **Remarques**

- Les paramètres doivent être envoyés **avant** d’activer les modes correspondants.

- La fenêtre hardware s’ajuste automatiquement selon le mode flux actif (PIC utilise `winPic`, les autres `winMoy`).

- Le calcul de l’angle est lissé par un filtre du premier ordre (75% historique, 25% instantané).

### **CONFIGURATION RECOMMANDÉE**

### **🎯 Mode “robot intelligent”**

```
modeMicsF = ANGLE

modeMicsI = DetectPresence
```

✔ performant  
✔ utile  
✔ léger

### **🔍 Mode “monitoring”**

```
modeMicsF = MOY

modeMicsI = InfoNuisce
```

✔ stable  
✔ peu coûteux

Amélioration possible : créer : micDirPresence = niveau + angle → évite double traitement par SP et double vpiv.
