# `outils/vpiv/` — outillage de la campagne de correction d'ordre VPIV

> Ces quatre fichiers sont le **moyen de rejouer**, et donc de vérifier, la correction
> menée le 22/10/2026 sur l'ordre des champs VPIV. Le récit complet — cause racine,
> volumes, pièges restants — est dans `docs/convention_VPIV_ordre.md`.
>
> *V1 — 22/10/2026 — Vincent Philippe / Claude*

---

## Ce que contient ce dossier

| Fichier | Rôle |
|---|---|
| `corriger_vpiv_ex.py` | Audite et corrige la colonne `VPIV_EX` de la Table A (150 cellules sur 251) |
| `corriger_commentaires_vpiv.py` | Remet dans l'ordre les trames d'exemple des **commentaires** du firmware (81 permutations) |
| `genere_classeur_tables.py` | Construit `TablesRZ_reference.ods`, avec la correction `VPIV_EX` intégrée depuis la v1.1 |
| `audit_vpiv_ex.csv` | Journal ligne à ligne : avant, après, verdict. 253 lignes. C'est la pièce de vérification |

---

## Mode d'emploi

```bash
# 1) La Table A — la source n'est jamais modifiee sur place
python3 corriger_vpiv_ex.py  TableA_source.csv  TableA_corrige.csv  audit.csv

# 2) Les commentaires du firmware — SIMULATION D'ABORD, toujours
python3 corriger_commentaires_vpiv.py  TableA_corrige.csv  <racine_depot>
python3 corriger_commentaires_vpiv.py  TableA_corrige.csv  <racine_depot>  --appliquer
```

Le premier affiche un récapitulatif par variable et **liste les cas ambigus sans les
toucher**. Le second refuse de modifier une trame hors commentaire : il la signale.

---

## Les deux précautions qui font la sûreté de ces scripts

### 1. Seule la colonne `PROP` sert de juge

Le réflexe naturel — comparer le 3ᵉ champ à `PROP` **et** le 4ᵉ à `INSTANCE` — est une
erreur, qui produit 48 faux « ambigus ». La colonne `INSTANCE` contient des **énumérations**
(`rear|front`, `TGD|THB|BASE`, `*|0–11`) et parfois des descripteurs (`inst`, `idx`).

`PROP` est vérifiée mono-valuée sur les 253 lignes : aucun `|`, aucune cellule vide.

```
champ3 == PROP                      ->  CORRECT   (intact)
champ4 == PROP  et champ3 != PROP   ->  INVERSÉ   (permuté)
ni l'un ni l'autre                  ->  AMBIGU    (intact, relecture humaine)
```

Résultat sur la Table A : **zéro cas ambigu**.

### 2. Commentaire ou code : un automate, pas une expression régulière

`corriger_commentaires_vpiv.py` parcourt chaque fichier caractère par caractère en
suivant un état : `CODE` / `LIGNE` (`//`) / `BLOC` (`/* */`) / `CHAINE` (`"…"`) /
`CARAC` (`'…'`), échappements compris.

> Une regex se fait piéger par le `//` d'une URL à l'intérieur d'une chaîne, et
> « corrigerait » alors du code vivant. Une trame dans un `sprintf` n'est pas un
> exemple : **c'est un comportement.**

Lors de la campagne, **aucune trame inversée ne se trouvait hors commentaire** — le code
vivant était déjà sain. C'est ce constat qui a autorisé la correction en masse.

---

## Comment la non-régression a été prouvée

Le diff seul ne suffit pas : il montre 89 lignes changées, sans dire si une seule portait
du code. La preuve a été faite en **retirant tous les commentaires** des 17 fichiers, dans
la version d'avant et celle d'après, puis en comparant :

```bash
for f in $(git diff --name-only); do
  git show HEAD:"$f" | python3 code_nu.py > /tmp/a.txt
  python3 code_nu.py < "$f"              > /tmp/b.txt
  diff -q /tmp/a.txt /tmp/b.txt || echo "CODE MODIFIE : $f"
done
```

Résultat : **code nu identique octet pour octet**, et diff équilibré (89 ajouts /
89 suppressions, donc uniquement des remplacements).

---

## Point de retour

```bash
git reset --hard origin/sauvegarde/avant-correction-VPIV-2026-10-22
```

Branche distante poussée sur GitHub avant la première modification de la campagne.
(Une étiquette avait été posée d'abord : le jeton refuse `refs/tags` en HTTP 403.)

---

## Recoupement

Deux implémentations indépendantes de la même règle — `corriger_vpiv_ex.py` d'un côté,
le bloc §2bis de `genere_classeur_tables.py` de l'autre — produisent exactement les
mêmes chiffres :

```
147 permutations + 3 étoiles parasites = 150 corrigées | 101 déjà bonnes | 0 ambiguë
```

C'est ce recoupement, et non la seule lecture du code, qui donne confiance dans le résultat.
