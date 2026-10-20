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
