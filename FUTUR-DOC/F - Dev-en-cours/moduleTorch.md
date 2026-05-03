
### Module TorchSE (La Torche)

Le module `TorchSE` est particulier car il possède une intelligence de "bascule" : il détecte si la caméra est déjà utilisée par une application de streaming (comme IP Webcam) pour choisir la méthode d'allumage du flash la plus appropriée.

#### A. Proposition pour la Table C (JSON)

*Note : J'ai harmonisé le nom de la propriété `act` (vue dans tes fichiers JSON) avec la logique de commande du script.*

JSON

```
`"TorchSE": \{`

`  "nomVarG": "TorchSE",`

`  "nomCpletVarG": "Torche Smartphone",`

`  "instances": \["\*"\],`

`  "descriptionVarG": "Gestion du flash LED du smartphone. Supporte le mode natif et le mode Caméra (IP Webcam).",`

`  "dateUpdateVarG": "2026-03-30T16:00:00.000Z",`

`  "proprietes": \[`

`    \{`

`      "nomP": "act",`

`      "nomCpletP": "État Torche",`

`      "descriptionP": "Commande de la torche : 0 (Off) ou 1 (On). Le script traduit automatiquement vers On/Off.",`

`      "typeP": "boolean",`

`      "valeurs": \{`

`        "\*": "0"`

`      \}`

`    \},`

`    \{`

`      "nomP": "level",`

`      "nomCpletP": "Intensité Flash",`

`      "descriptionP": "Niveau de luminosité (si supporté par le matériel). Unité : 0-100.",`

`      "typeP": "number",`

`      "valeurs": \{`

`        "\*": "100"`

`      \}`

`    \}`

`  \]`

`\}`
```


#### B. Documentation Officielle du Module : TorchSE

# Documentation Module : TorchSE (Flash LED)

## 1. Présentation Générale

Le module **TorchSE** permet au robot d'éclairer son environnement immédiat en utilisant la LED flash du smartphone. Il est principalement utilisé lors des phases de navigation nocturne ou pour signaler un état visuel (clignotement d'erreur).

## 2. Architecture et Intelligence de Bascule

La force de `rz\_torch\_manager.sh` est sa gestion du conflit d'accès matériel :

1. **Mode Natif (`termux-torch`)** : Utilisé quand la caméra est libre. C'est la méthode la plus rapide.

2. **Mode Caméra (API HTTP)** : Si le robot diffuse de la vidéo (via IP Webcam), Android interdit l'accès direct au flash. Le script détecte cet état via `cam\_config.json` et envoie alors une requête `curl` à l'API interne de l'application caméra pour allumer la LED sans interrompre le flux vidéo.

## 3. Commandes et Retours (VPIV)

### Commande (SP → SE)

L'ordre est envoyé par le Système Pilote sous la forme :

- `$V:TorchSE:act:\*:1\#` pour Allumer.

- `$V:TorchSE:act:\*:0\#` pour Éteindre.

### Retour d'état (SE → SP)

Conformément au script, le module renvoie un acquittement après exécution :

- `$I:TorchSE:active:SE:On\#` (Confirmation d'allumage).

- `$I:TorchSE:active:SE:Off\#` (Confirmation d'extinction).

*Note : Bien que la commande utilise des booléens (0/1), le retour `active` utilise les chaînes `On/Off` pour une lisibilité directe dans les logs de debug.*

## 4. Sécurité et Limitations

- **Surchauffe** : L'utilisation prolongée du flash à 100% d'intensité peut provoquer une montée en température du smartphone. Le module `Sys` surveillera alors la métrique `thermal` et pourra couper la torche si le seuil d'urgence est atteint.

- **Dépendances** : Nécessite l'installation du paquet `termux-api` sur le smartphone.


### Vérification de cohérence (Annonce pour l'annuaire)

> **Observation importante** : Ton script `rz\_torch\_manager.sh` attend les arguments `"active" "On"` ou `"active" "Off"`.

> Cependant, ta Table A stipule que les types `boolean` doivent circuler en `0` ou `1`.

> **Action recommandée** : Je te conseille de modifier légèrement la réception dans ton script manager pour qu'il accepte `0` et `1` en entrée, afin de rester 100% compatible avec le reste de ton écosystème.

On valide la Torche ? Si c'est ok, nous avons terminé la ronde des capteurs/actionneurs spécifiques du Smartphone.

**Prochaine étape : Le module STT (Speech-To-Text) ?**

