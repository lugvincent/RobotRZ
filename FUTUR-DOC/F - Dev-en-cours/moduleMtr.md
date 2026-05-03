# Module MTR

## Rôle
Pilotage moteurs DC différentiel.

## Architecture
dispatch_Mtr → mtr → mtr_hardware → URG

## Fonctionnement
- Mode: XY (avance/rotation)
- Lecture: state
- Publication: targets, state
- Alertes: stop via URG

## Cycle de vie
- init
- commande continue

## Boucle principale
mtr_process()

## Propriétés VPIV
- $V:Mtr:*:targets:100,0#
- $V:Mtr:*:act:1#

## États
- targets
- state

## Sécurité
- URG
- timeout
- clamp

## Paramètres internes
- cfg_mtr_active
- cfg_mtr_scale

## Remarques
Très critique → sécurité prioritaire
