# Module IRmvt — Détection de mouvement infrarouge

## Rôle

Détecter des impulsions IR et signaler :

- une valeur instantanée (value)

- une alerte après N détections consécutives (threshold)


## Propriétés

### act (boolean)

Activation du module

### freq (number, ms)

Période de lecture

- 0 → désactivé

> - 0 → lecture périodique

### thd (number)

Nombre d’impulsions consécutives pour déclencher une alerte

### mode (enum)

- idle → désactivé

- monitor → standard

- surveillance → sensible

### value (number)

Valeur brute capteur (0 ou 1)

### alert (boolean)

Détection confirmée (après seuil)

### read (event)

Commande de lecture instantanée


## Comportement

### Lecture instantanée

Commande : $V:IRmvt:\*:read:1\#

Réponse : $I:IRmvt:*:read:OK\#* *$I:IRmvt:*:value:X\#


### Fonctionnement périodique

Si actif et freq \> 0 :

- lecture capteur

- envoi value

- incrément compteur si détection

- envoi alert si seuil atteint


## Notes

- Le compteur est interne (non exposé)

- Le seuil permet d’éviter les faux positifs

