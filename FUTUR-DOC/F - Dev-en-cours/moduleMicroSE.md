
# Documentation Module : MicroSE (Analyseur d'Ambiance Sonore)

## 1. Présentation Générale

Le module **MicroSE** utilise le microphone du smartphone pour doter le robot RZ d'une "ouïe" analytique. Il ne s'agit pas ici de reconnaissance de mots (STT), mais de la mesure de l'intensité sonore (volume) et de la capacité à orienter la tête du robot vers une source de bruit (claquement de mains, voix, choc).

## 2. Architecture Technique

Le module repose sur deux scripts travaillant de concert :

- **`rz\_microSe\_manager.sh`** : Le daemon principal. Il capture l'audio via `termux-microphone-record` et utilise `ffmpeg` avec le filtre `astats` pour extraire la valeur RMS (Root Mean Square).

- **`rz\_balayage\_sonore.sh`** : Un script de spécialisation lancé par le manager. Il prend le contrôle du servo **TGD** (Tête Gauche-Droite) pour effectuer un balayage de -90° à +90° et identifier l'angle où le volume est le plus élevé.

## 3. Modes de Fonctionnement (`modeMicro`)

| **Mode** | **Comportement** |
| - | - |
| **`off`** | Aucune capture. Le microphone est libéré pour d'autres applications. |
| **`normal`** | Flux constant de la valeur `dataContRMS` vers le Système Pilote à la fréquence `freqCont`. |
| **`intensite`** | Envoi de données uniquement si le volume change de façon significative (`seuilDelta`) ou dépasse un seuil fixe (`seuilMoyen`). |
| **`orientation`** | Déclenche un cycle unique de balayage physique. Le robot tourne la tête, cherche le bruit max, et envoie la `direction`. |
| **`surveillance`** | Mode hybride : le robot écoute en statique. Si un bruit dépasse le seuil pendant `holdTime` ms, il lance automatiquement un balayage d'orientation. |

## 4. Propriétés et Paramétrage (VPIV)

### Configuration (SP → SE)

- **`paraMicro`** (Objet JSON) :

  - `freqCont` : Intervalle entre deux mesures (en ms).

  - `winSize` : Durée de l'échantillon audio enregistré pour chaque mesure (en ms). Plus elle est grande, plus la mesure est précise mais lente.

  - `seuilMoyen` : Seuil d'intensité au-dessus duquel on considère qu'il y a un "bruit" notable.

  - `holdTime` : Temps de maintien au-dessus du seuil nécessaire pour valider un événement sonore.

  - `pasBalayage` : Précision du scan (ex: 10° par 10°).

### Retours de Données (SE → SP)

- **`dataContRMS`** : Valeur entière (0-1000). Représente le volume.

- **`direction`** : Angle final retourné après un scan.

  - *Note technique : Conformément à la Table A, la valeur envoyée est l'angle $\\times$ 10 (ex: 450 pour 45.0°).*

## 5. Logique de Localisation (Balayage)

Lorsqu'un balayage est actif :

1. Le manager suspend ses mesures normales et lance `rz\_balayage\_sonore.sh`.

2. Le script envoie des commandes VPIV `$V:Srv:TGD:\*:angle\#` vers l'Arduino.

3. À chaque pas, il mesure le RMS.

4. Il identifie l'angle `max\_rms`.

5. En fin de cycle, il repositionne la tête à 0° (face) et publie la direction trouvée au SP.


### Points d'attention pour la Table A (Vérification)

> **Codage VPIV** : J'ai noté une petite divergence potentielle dans tes fichiers. Ton script `rz\_balayage\_sonore.sh` envoie l'angle brut (ex: `45`), mais ta **Table A (Légende)** demande un codage `deg \* 10` (ex: `450`).

> **Attention :** Pour la cohérence du futur "annuaire", le script multiplie par 10 avant l'envoi de l’angle.


