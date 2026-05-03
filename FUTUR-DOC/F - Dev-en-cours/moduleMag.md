
# Documentation Module : Mag (Magnétomètre / Boussole)

## 1. Présentation Générale

Le module **Mag** gère le magnétomètre du smartphone embarqué pour fournir une boussole numérique au robot. Il permet de connaître l'orientation absolue du robot par rapport au champ magnétique terrestre.

Contrairement au Gyroscope qui mesure une vitesse de rotation (relative), le Magnétomètre fournit une référence fixe (absolue), indispensable pour recaler la navigation à long terme.

## 2. Architecture Technique

Le script `rz\_mg\_manager.sh` exploite le capteur via `termux-sensor`.

- **Calcul de l'Azimut :** Le script calcule l'angle en utilisant la fonction `atan2(y, x)` sur les axes horizontaux du smartphone.

- **Conversion d'Unités :** - Les valeurs d'axes (XYZ) sont envoyées en **µT \* 100** (MicroTesla).

  - Les angles (Cap) sont envoyés en **degrés \* 10** (ex: 1805 pour 180,5°).

- **Déclinaison :** Le module peut intégrer une `declination` (configurable dans `paraMg`) pour compenser l'écart entre le Nord Magnétique et le Nord Géographique.

## 3. Modes de Fonctionnement (`modeMg`)

Le module propose une granularité fine selon les besoins de l'algorithme de pilotage :

| **Mode** | **Sortie VPIV** | **Description** |
| - | - | - |
| **`OFF`** | - | Capteur arrêté. |
| **`DATABrutCont`** | `dataContBrut` | Envoi du vecteur `\[x, y, z\]` brut. Utilisé pour la calibration. |
| **`DATAMgCont`** | `DataContMg` | Cap par rapport au Nord Magnétique (flux rapide). |
| **`DATAGeoCont`** | `DataContGeo` | Cap corrigé de la déclinaison (flux rapide). |
| **`DATAMgMoy`** | `DataMoyMg` | Cap magnétique lissé sur `nbCycles` (stable pour la route). |
| **`DATAGeoMoy`** | `DataMoyGeo` | Cap géographique lissé (référence de navigation finale). |

## 4. Propriétés et Paramétrage (VPIV)

### Configuration (SP → SE)

- **`modeMg`** : Sélection du flux de données.

- **`paraMg`** (Objet JSON) :

  - `freqCont` : Fréquence des flux continus (Hz).

  - `freqMoy` : Fréquence des flux moyennés (Hz).

  - `nbCycles` : Taille de la fenêtre de calcul de la moyenne.

  - `declination` : Valeur de correction locale (en degrés).

### Flux de Données (SE → SP)

- Les données de cap (`Geo` ou `Mg`) sont normalisées entre **0 et 3599** (0° à 359,9°).

- Le format des trames suit la nomenclature : `$F:Mag:Propriété:Instance:Valeur\#`.

## 5. Précautions d'Usage et Perturbations

Le magnétomètre est extrêmement sensible aux masses métalliques et aux champs électromagnétiques (moteurs du robot, haut-parleurs).

1. **Calibration :** Il est recommandé d'utiliser le mode `DATABrutCont` pour vérifier la sphère magnétique lors de l'intégration physique du smartphone sur le châssis.

2. **Installation :** Le smartphone doit être éloigné autant que possible des moteurs de propulsion pour éviter les déviations dynamiques lors des déplacements.


**Note technique : p**our le mode `DATABrutCont`, le script envoie un objet JSON `\[x,y,z\]`, alors que pour les caps, il envoie une valeur numérique simple. C'est reflété dans la Table C proposée.

