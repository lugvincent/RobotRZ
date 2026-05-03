# Module Ctrl\_L — Contrôle par laisse

## 1. Rôle

Le module **Ctrl\_L** permet au robot de suivre un utilisateur via une laisse instrumentée :

- Maintien d’une distance cible (capteurs US avant)

- Orientation par capteur de force (traction gauche/droite)


## 2. Principe de fonctionnement

### 2.1 Régulation distance

- Mesure : moyenne glissante capteurs US arrière

- Calcul : delta = dist\_mesurée - dist\_cible

- Si |delta| ≤ dead → aucune correction

- Sinon : speed = clamp(delta / K, ±vmax)


### 2.2 Régulation orientation

- Mesure : capteur de force

- Zone morte : ±5

- Calcul : turn = dir × |force|


## 3. Conditions d’activation

Le module est actif si :

- `cfg\_typePtge = 3` (laisse)

- `cfg\_typePtge = 4` (laisse + vocal)

Ou via VPIV :


## 4. Propriétés VPIV

| PROP | TYPE | DIR | Rôle |
| - | - | - | - |
| act | bool | ↔ | Activation |
| dist | number (mm) | ↔ | Distance cible |
| dead | number (mm) | ↔ | Zone morte |
| vmax | number | ↔ | Vitesse max |
| status | enum | ↔ | État module |



## 5. États

| Valeur | Description |
| - | - |
| OFF | module désactivé |
| OK | fonctionnement normal |
| FceKO | force excessive |
| VtKO | utilisateur trop rapide (non impl.) |



## 6. Fonctionnement périodique

Fréquence : 5 Hz

Étapes :

1. Lecture capteurs US

2. Calcul distance moyenne

3. Calcul vitesse

4. Lecture force

5. Calcul rotation

6. Commande moteurs

7. Publication état


## 7. Sécurité

- Arrêt si urgence active

- Arrêt si module désactivé

- Désactivation automatique si mode change


## 8. Paramètres internes

| Paramètre | Rôle |
| - | - |
| target\_dist\_mm | distance cible |
| max\_speed | vitesse max |
| max\_turn | rotation max |
| us\_window | filtre |
| dead\_zone\_mm | zone morte |



## 9. Remarques

- Toutes les distances sont en mm

- Publication status uniquement si changement

- Mode test disponible

