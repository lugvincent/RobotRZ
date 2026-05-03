# Module LRUB

## Rôle
Pilotage ruban LED RGB.

## Architecture
dispatch_Lrub → lrub → hardware

## Fonctionnement
- Mode: static, extern, effet
- Lecture: état courant
- Publication: col, lumin, act
- Alertes: timeout

## Cycle de vie
- init
- timeout loop

## Boucle principale
lrub_processTimeout()

## Propriétés VPIV
- $V:Lrub:*:col:255,0,0#
- $V:Lrub:*:lumin:128#

## États
- col
- lumin
- act

## Sécurité
- timeout extinction

## Paramètres internes
- cfg_lrub_brightness

## Remarques
Timeout dépend de loop
