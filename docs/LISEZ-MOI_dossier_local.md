# LISEZ-MOI — dossier documentaire local

> **Ceci est la copie de référence, versionnée.** Le fichier utile est sa copie déposée à la racine
> de `Documents\ProjetRZ\AA-Doc-horsGit\`, sous le nom `LISEZ-MOI.md`. Ce dossier étant exclu du
> dépôt, sa copie ne peut pas être versionnée — d'où ce double. Si vous modifiez l'un, recopiez l'autre.

---

Vous êtes dans le **dossier documentaire local** du projet RobotRZ. Il est volontairement **hors du
dépôt Git**, qui est public et contiendrait sinon vos identifiants.

## Comment c'est rangé

**Le thème décide du dossier. L'âge décide de la place à l'intérieur du dossier.**

Une version dépassée ne part pas ailleurs : elle descend dans le sous-dossier `old\` de sa propre
lettre, au plus près de ce qu'elle complète.

| | |
|---|---|
| **A** | Architecture, règles, tables |
| **B** | SP — Node-RED |
| **C** | SE — Termux et Tasker |
| **D** | Arduino |
| **E** | ⚠ *Exception* — sas d'échange ponctuel, **fait pour être vidé** |
| **F** | ⚠ *Exception* — le travail du moment, rassemblé en un endroit |
| **H** | Outils |
| **I** | Archive — sujets clos |
| **J** | Électronique et câblage — traversent les générations du robot |
| **K** | Documents utilisateurs |
| **L** | RZ-v1 sur RPi4 — génération antérieure, **encore en service** |
| **M** | Bilans de séance — par date, restent locaux |
| **Z** | 🔒 Secret — clés, identifiants, codes |

`CODE\` reste à part : ce sont des copies de travail de code, pas de la documentation.

## Quand un document existe en deux formats

Vous avez souvent un `.odt` et un `.md` portant le même nom. Ou un `.ods` et un `.csv`.
La règle est simple, et le risque qu'elle évite est réel.

**Le `.odt` et le `.ods` font foi.** Ce sont vos documents de travail, ceux que vous éditez
dans Writer et Calc. Le `.md` et le `.csv` qui les accompagnent sont des **copies de lecture**.

**Pourquoi garder une copie texte quand même.** Trois raisons, toutes vérifiées à l'usage.
Git sait montrer ce qui a changé dans un `.md` ligne par ligne, là où un `.odt` n'est pour lui
qu'un bloc binaire dont il dit seulement « ça a changé ». Une recherche de mot traverse trente
fichiers `.md` instantanément, alors qu'il faut ouvrir trente `.odt` un par un. Et un fichier
texte se lit sans intermédiaire, alors qu'un `.ods` doit être décompressé et reconstitué —
c'est exactement ce détour qui a fait croire, le 18/10/2026, à 65 différences inexistantes
dans Table A.

⚠ **Le danger est la divergence silencieuse.** Si vous éditez le `.odt` et oubliez de
régénérer le `.md`, rien ne vous avertit : les deux fichiers coexistent, l'un périmé, et
celui qui le lit raisonne faux. Le cas s'était produit — `vtravailSyntheseRZV2.md` d'avril
contre son `.odt` de juillet, deux mois et demi d'écart.

**En pratique :**

| Situation | Que faire |
|---|---|
| Le `.md` est dans le dépôt, l'`.odt` ici | Garder les deux — c'est le bon partage |
| Les deux sont ici et vous n'éditez que l'`.odt` | Supprimer le `.md`, il ne fera que vieillir |
| Les deux sont ici et vous éditez les deux | Choisir, descendre l'autre dans `old\` |

Pour repérer les paires et comparer leurs dates, depuis n'importe où :

```
$D="$HOME\Documents\ProjetRZ\AA-Doc-horsGit" ; Get-ChildItem $D -Recurse -File -Include *.md,*.odt,*.ods,*.csv -Force | Group-Object BaseName | Where-Object { ($_.Group.Extension | Select-Object -Unique).Count -gt 1 } | ForEach-Object { "=== $($_.Name)" ; $_.Group | Sort-Object LastWriteTime | ForEach-Object { "   {0}  {1,6:N0} Ko  {2}" -f $_.LastWriteTime.ToString('yyyy-MM-dd'), ($_.Length/1KB), $_.FullName.Replace($D+"\","") } }
```

La question qui tranche est toujours la même : **lequel avez-vous ouvert en dernier pour y écrire ?**

## Les deux règles à ne pas oublier

**Ce dossier est exclu du dépôt par son nom.** Si vous le renommez, ajoutez le nouveau nom au
`.gitignore` **dans le même geste** — sans quoi l'exclusion ne s'applique plus et vos clés
deviennent publiables, sans aucun avertissement.

**Rien d'autre que des secrets ne va dans `Z`**, et aucun secret ne va ailleurs.

## Pour tout le reste

Le vade-mecum complet est dans le dépôt :

```
Documents\ProjetRZ\docs\dev_RZ_HTML\vademecum_documentation_V1.html
```

Il répond à : où sont les quatre emplacements de la documentation, lequel fait foi en cas d'écart,
où ranger un document qu'on vient de produire, et que faire quand on ne trouve pas.

Et pour savoir où en est le projet : `docs\synthese_etat.md`, ou sa copie vivante dans le
Project Knowledge, qui fait foi des deux.

---

*V1 — 20/10/2026 — Vincent Philippe / Claude*
