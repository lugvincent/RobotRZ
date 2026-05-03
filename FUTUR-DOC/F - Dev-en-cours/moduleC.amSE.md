
# Module : CamSE (Camera Smartphone)

## Présentation

Le module `CamSE` pilote les caméras physique du smartphone embarqué. Il s'appuie sur l'application Android **IP Webcam** pour transformer le smartphone en caméra IP accessible via HTTP.

## Fonctionnement technique

Le script `rz\_camera\_manager.sh` gère l'interface entre les commandes VPIV reçues et les Intents Android (`am start`).

- **Instances :** `front` (avant) et `rear` (arrière).

- **Priorité CPU :** Si la charge CPU du SE dépasse 70%, le module bascule automatiquement sur un profil "optimized" (basse résolution/FPS) défini dans `cam\_config.json`.

## Tableau des Propriétés (VPIV)

| **Propriété** | **Direction** | **Type** | **Valeurs / Format** | **Description** |
| - | - | - | - | - |
| **modeCam** | SP → SE | enum | `off`, `stream`, `snapshot` | Commande principale de démarrage/arrêt. |
| **paraCam** | SP → SE | JSON | `\{res, fps, quality, urgDelay\}` | Configuration de la capture. |
| **snap** | SP → SE | bool | `1` | Force une prise de vue immédiate. |
| **StreamURL** | SE → SP | string | `http://\<IP\>:8080/video` | URL du flux ou de l'image (ACK). |
| **error** | SE → SP | Event | `CODE\_ERREUR` | Signal d'erreur critique (CAT=E). |

## Gestion des Erreurs et Sécurité

Le module intègre une logique de sécurité liée au mode de pilotage (`CfgS:typePtge`) :

1. **Modes Visuels (0:Ecran, 2:Joystick) :** En cas d'erreur caméra (`CAM\_START\_FAIL`), le système attend `urgDelay` secondes avant de déclencher une urgence globale `URG\_SENSOR\_FAIL`.

2. **Autres modes :** L'erreur est simplement remontée comme une information `COM:error`.

## Fichiers de configuration associés

- `config/sensors/cam\_config.json` : Stocke l'état courant et les profils CPU (normal/optimized).

- `config/global.json` : Lecture de `.Sys.cpu` et `.CfgS.typePtge`.
