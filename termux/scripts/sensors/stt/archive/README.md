# Versions archivées de rz_stt_handler.sh

**Chemin** : `termux/scripts/sensors/stt/archive/`
**Objectif** : conserver les versions de `rz_stt_handler.sh` qui ne figurent pas dans l'historique Git.
**Version** : 1.0 — 20/10/2026 — Vincent Philippe / Claude

---

## Pourquoi ce dossier existe

En temps normal, l'historique d'un fichier vit dans Git : `git log --follow` suffit à le remonter, et
un dossier d'archive serait un doublon inutile.

Ce dossier ne contient donc **que les versions absentes de l'historique** — celles dont le code n'a
jamais été commité, et qui seraient perdues sans cette copie.

⚠️ **Rien ici n'est exécuté.** La version vivante est à la racine de `sensors/stt/`. Ces fichiers sont
de la mémoire, pas du code en service.

## Contenu

### `rz_stt_handler_v3.0_2026-07-07.sh`

Version 3.0, datée du 7 juillet 2026. **Introduit le format de capsule à six champs** pour les
traitements `TRADUCTION`, `CALCUL` et `MACRO` :

```
$A:STT:intent:<id_requete>:{<id_traitement>:<cat>:<var>:<prop>:<inst>:<val_brute>}#
```

C'est l'étape de conception sur laquelle repose toute la résolution vocale actuelle : SE transmet la
valeur brute, SP résout via le registre `global.RZ_Traitements`. Le dépôt passait directement d'une
version antérieure à la v3.1, sautant ce moment.

**Retrouvée le 20/10/2026** dans un `files.zip` enfoui au fond du dossier documentaire local, lors du
rangement. Son empreinte a été cherchée dans l'intégralité de l'historique Git — elle n'y figurait pas.

Pour comparer avec la version en service :

```
git diff --no-index termux/scripts/sensors/stt/archive/rz_stt_handler_v3.0_2026-07-07.sh termux/scripts/sensors/stt/rz_stt_handler.sh
```

## Quand ajouter un fichier ici

Seulement après avoir vérifié qu'il est **réellement absent de l'historique**. La commande qui tranche,
depuis la racine du dépôt :

```
H=$(md5sum <le_fichier> | cut -d' ' -f1)
for b in $(git rev-list --all --objects -- <chemin/suivi> | awk 'NF==2{print $1}' | sort -u); do
  [ "$(git cat-file -t $b)" = blob ] && [ "$(git cat-file blob $b | md5sum | cut -d' ' -f1)" = "$H" ] && echo "DEJA DANS L'HISTORIQUE : $b"
done
```

Si elle n'affiche rien, la version est bien inédite et mérite sa place ici.
