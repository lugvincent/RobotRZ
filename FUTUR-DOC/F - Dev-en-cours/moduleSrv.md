# Module SRV

## Rôle
Pilotage de servomoteurs (angle, vitesse, activation).

## Architecture
dispatch_Srv → srv → srv_hardware

## Fonctionnement
- Mode: direct (pas de mode complexe)
- Lecture: via read
- Publication: angle, speed, act
- Alertes: aucune

## Cycle de vie
- init → applique config
- process → update hardware

## Boucle principale
srv_process()

## Propriétés VPIV
- $V:Srv:TGD:angle:90#
- $V:Srv:*:act:1#

## États
- angle (Etat estimé)
- speed
- act

## Sécurité
- constrain angle 0–180

## Paramètres internes
- cfg_srv_angles[]
- cfg_srv_speed[]
- cfg_srv_active[]

## Remarques
Pas de retour réel → état estimé
