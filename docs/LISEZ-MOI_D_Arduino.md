# LISEZ-MOI — dossier `D - Arduino`

> **Ceci est la copie de référence, versionnée.** Le fichier utile est sa copie déposée dans
> `AA-Doc-horsGit\D - Arduino\`, sous le nom `LISEZ-MOI.md`. Ce dossier étant exclu du dépôt,
> sa copie ne peut pas être versionnée — d'où ce double. Si vous modifiez l'un, recopiez l'autre.

---

## 🔴 La référence Arduino n'est plus dans ce dossier

Elle est dans le dépôt :

```
Documents\ProjetRZ\docs\reference_firmware_arduino.md
```

**Écrite depuis les sources du code, pas depuis la documentation.** C'est la distinction qui
compte, et voici pourquoi elle a été faite.

## Ce qui s'est passé le 22 octobre 2026

En relisant `Synthèse-Arduino-GenereParChatGPT.odt` pour en tirer une référence, la confrontation
au code du dépôt a donné ceci :

| Vérification | Résultat |
|---|---|
| Annexe B, tables des propriétés par module | **Fausse sur 5 modules sur 10** |
| `Ctrl_L` | 6 propriétés annoncées sur 8 n'existent nulle part |
| `Mic` | les 3 propriétés annoncées sont fausses |
| Le « mode legacy » de `Mtr`, cité 7 fois | **n'existe pas dans le firmware** |
| Le sigle VPIV | développé de deux façons différentes dans le même document |
| La catégorie `E` | « Événement » au §3.3, « Erreur » au §13.2 |

Et dans `IMP encodage et code arduino.odt` : **58 trames VPIV sur 88 écrites dans l'ordre inverse**,
plus des noms de variables qui n'existent pas (`EMG`, `Emergency`).

Ces documents ne sont pas négligents. Ils sont les témoins d'une **convention VPIV qui a changé
sans que tout suive** — et cette même croyance avait contaminé le code de réception du firmware,
où elle empêchait purement et simplement la Mega de recevoir la moindre commande. Le défaut est
corrigé depuis (commit `bf0e422`), et l'histoire complète est en §9.5 de `docs/synthese_etat.md`.

## La règle, désormais

> **Le code fait foi sur ce que fait le firmware. La Table A fait foi sur la norme.**
> Les documents de ce dossier ne font foi sur rien.

> 🔴 **Nuance ajoutée le 22/10/2026, et elle est capitale.** « La Table A fait foi »
> vaut pour ses colonnes `VAR`, `PROP` et `INSTANCE`. **Pas pour sa colonne `VPIV_EX`** :
> 150 de ses 251 exemples étaient écrits à l'ancien ordre, parce que cette colonne n'a
> jamais été migrée quand la convention a changé. C'est d'elle que tout le monde a
> recopié — les commentaires du firmware, les documents archivés, les légendes, et
> jusqu'au code de réception de la Mega. **Elle est la cause racine de tout.**
> Toute l'histoire est dans `docs/convention_VPIV_ordre.md`. La colonne est corrigée
> depuis le 22/10 ; une copie de la Table A antérieure à cette date reste fausse à 61 %.

Ils gardent une valeur historique — les raisonnements de conception, les justifications de choix —
mais **aucune de leurs valeurs techniques ne doit être reprise sans vérification dans le code**.

## Ce que contient ce dossier

| Document | Statut |
|---|---|
| `reference_modules_arduino_2026-02.odt` *(dans `A - Architecture_Tables_Schemas`)* | ✅ **Vérifié bon** — 12 constantes matérielles exactes, et c'est lui qui documentait `Odom.coeff` que la Table A oubliait |
| `Synthèse-Arduino-GenereParChatGPT.odt` | ⚠️ Sections 1 à 5 utilisables, **annexe B à jeter** |
| `old\IMP encodage et code arduino.odt` | 🔴 Archivé — 58 trames sur 88 à l'ordre inverse |
| Les autres | Non vérifiés. À confronter au code avant tout usage |

## Où aller selon la question

| Vous cherchez | Allez voir |
|---|---|
| Comment le code est organisé, que fait la boucle, comment une trame est traitée | `docs/reference_firmware_arduino.md` |
| Le nom exact d'une variable ou d'une propriété | La Table A — `A - …\Tables\TableA_updatedV3_corrige.ods` |
| Les règles du protocole VPIV | `docs/tables_RZ_descripteur.md` §5 |
| L'état d'avancement, les pièges connus | `docs/synthese_etat.md` |
| Ce qui attend le robot monté | `docs/travaux_robot_assemble.md` |
| Un module en particulier, en détail | `reference_modules_arduino_2026-02.odt` |

---

*V1 — 22/10/2026 — Vincent Philippe / Claude*
