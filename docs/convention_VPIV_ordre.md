# La convention d'ordre VPIV — cause racine, campagne de correction, règles

> **Document de référence.** Il explique pourquoi l'ordre des champs VPIV a été faux
> pendant des mois dans la moitié de la documentation du projet, ce qui a été corrigé
> le 22/10/2026, et quelle règle appliquer désormais.
>
> *V1 — 22/10/2026 — Vincent Philippe / Claude*

---

## 1. La règle, en une ligne

```
$CAT:VAR:PROP:INST:VAL#
  │   │    │    │    │
  │   │    │    │    └── valeur — jamais un flottant sur le fil (×10, ×100, ×1000)
  │   │    │    └─────── INSTANCE  — position 4
  │   │    └──────────── PROPRIÉTÉ — position 3
  │   └───────────────── VARIABLE
  └───────────────────── CATÉGORIE : V consigne · I info/ACK · E urgent · F flux · A app Android
```

**La propriété précède l'instance.** Exemple : `$V:Srv:angle:TGD:90#` — on règle *l'angle*
(propriété) du servo *TGD* (instance).

---

## 2. La cause racine, trouvée le 22/10/2026

La Table A comporte quinze colonnes. Trois nous intéressent ici :

| Colonne | Contenu | A-t-elle été migrée ? |
|---|---|---|
| `PROP` | le nom de la propriété, valeur unique | ✅ oui |
| `INSTANCE` | les instances autorisées, **énumération** (`rear\|front`, `TGD\|THB\|BASE`) | ✅ oui |
| `VPIV_EX` | un exemple de trame complète | 🔴 **non** |

Quand la convention est passée de `$CAT:VAR:INST:PROP:VAL#` à `$CAT:VAR:PROP:INST:VAL#`,
les colonnes `PROP` et `INSTANCE` ont suivi. **La colonne `VPIV_EX` est restée en l'état.**

Or `VPIV_EX` est la colonne que tout le monde lit, parce que c'est la seule qui montre
une trame entière. D'où la propagation :

```
        Table A, colonne VPIV_EX  (150 exemples sur 251 à l'ancien ordre)
                      │
        ┌─────────────┼─────────────┬──────────────────┐
        ▼             ▼             ▼                  ▼
  commentaires   documents     deux fichiers    code de RÉCEPTION
  du firmware    archivés      de légende       de la Mega
     (81)        (58 sur 88)                    (corrigé le 22/10,
                                                 commit bf0e422)
```

Le dernier maillon est celui qui faisait mal : `communication.cpp` appelait
`dispatchVPIV(module, inst, prop, value)` au lieu de `(module, prop, inst, value)`.
**La carte ne pouvait recevoir aucune commande.** Et la trame d'erreur qu'elle renvoyait,
`$E:Urg:*:clear:urgence active#`, portait la permutation dans sa propre sortie.

> Ce n'est donc pas une suite de fautes de frappe indépendantes. **C'est une seule
> colonne non migrée, recopiée partout.**

---

## 3. Ce qui a été corrigé le 22/10/2026

### 3.1 Le firmware — commit `bf0e422`

Deux lignes dans `communication.cpp` (193 et 212). Vérifié sur matériel. Le filtre
d'urgence a été élargi pour accepter `$V:CfgS:emg:*:clear#`, que le dispatcheur
acceptait déjà en aval.

### 3.2 Les commentaires du firmware — commit `9787ef8`

**89 exemples** dans 17 fichiers.

| Correction | Nombre |
|---|---|
| Permutation `INST`↔`PROP` | 81 |
| `$I:System:boot:*:OK#` → `$I:CfgS:boot:*:1#` | 2 |
| `$I:ctrl_L:…` → `$I:Ctrl_L:…` | 4 |
| Gabarit `$V:Ctrl_L:*:...:...#` → `$V:Ctrl_L:<prop>:*:<valeur>#` | 1 |
| `$I:Mic:rms:0:512#` → `$I:Mic:micMoy:*:512#` (`rms` n'existe nulle part) | 1 |

**Aucune trame inversée ne se trouvait hors commentaire.** Le code vivant était déjà
sain — c'est le point qui autorisait la correction en masse. Preuve : en retirant tous
les commentaires des 17 fichiers avant et après, le code nu est identique octet pour
octet, et le diff est équilibré (89 ajouts / 89 suppressions, donc que des remplacements).

### 3.3 La Table A, colonne `VPIV_EX`

**150 cellules sur 251 non vides.** Fichier produit : `TableA_VPIV_EX_corrige.csv`,
accompagné de `audit_vpiv_ex.csv` qui donne, ligne à ligne, l'avant, l'après et le verdict.

| Verdict | Nombre |
|---|---|
| Déjà correct, laissé intact | 101 |
| Permuté | 147 |
| Étoile parasite (voir §3.4) | 3 |
| Vide | 2 |
| **Ambigu, non corrigé** | **0** |

### 3.4 Les trois trames `Urg` doublement cassées

```
$E:Urg:*:etat:A:active#      →   $E:Urg:etat:A:active#
$I:Urg:*:etat:A:cleared#     →   $I:Urg:etat:A:cleared#
$E:Urg:*:source:A:loop_slow# →   $E:Urg:source:A:loop_slow#
```

Six champs au lieu de cinq : ordre inversé **plus** un `*` surnuméraire. Une simple
permutation aurait donné `$E:Urg:etat:*:A:active#`, toujours faux. Traitées à part.

---

## 4. Comment la détection a été faite — et pourquoi elle est sûre

Le réflexe naturel est de comparer le 3ᵉ champ à la colonne `PROP` **et** le 4ᵉ à la
colonne `INSTANCE`. **C'est une erreur**, et elle m'a d'abord produit 48 faux « ambigus » :

> `INSTANCE` contient des **énumérations** (`rear|front`, `TGD|THB|BASE`, `*|0–11`) et
> parfois de simples descripteurs (`inst`, `idx`). La comparer à un champ de trame ne
> veut rien dire.

La colonne `PROP`, elle, a été vérifiée **mono-valuée** : aucune occurrence de `|`,
aucune cellule vide, sur les 253 lignes. C'est donc le seul juge fiable :

```
   champ3 == PROP                      →  CORRECT     (on ne touche pas)
   champ4 == PROP  et champ3 != PROP   →  INVERSÉ     (on permute)
   ni l'un ni l'autre                  →  AMBIGU      (relecture humaine obligatoire)
```

Avec cette règle : **zéro cas ambigu** sur la Table A. La correction est intégralement
déterministe et rejouable.

---

## 5. Les pièges qui subsistent — à connaître avant de lire quoi que ce soit

### 5.1 🔴 Ne jamais recopier un exemple sans vérifier sa source

| Source | Confiance |
|---|---|
| `TableA…corrige.csv` colonnes `VAR` / `PROP` / `INSTANCE` | ✅ fait foi sur la **norme** |
| `TableA…corrige.csv` colonne `VPIV_EX`, **après** la correction du 22/10 | ✅ fiable |
| La même colonne dans une copie **antérieure** au 22/10 | 🔴 **61 % de faux** |
| Le code du firmware (`sendInfo`, `strcmp`) | ✅ fait foi sur le **comportement réel** |
| Les commentaires du firmware, après `9787ef8` | ✅ fiables |
| `docs/` du dépôt | ✅ fiables |
| `AA-Doc-horsGit/` et les ODT archivés | 🔴 **non migrés, jamais** |

### 5.2 Les quatre citations volontairement fausses

Quatre trames à l'ancien ordre subsistent **exprès** dans `docs/` : ce sont les
*pièces à conviction* du défaut.

| Fichier | Ligne | Pourquoi |
|---|---|---|
| `reference_firmware_arduino.md` | ~353 | illustre le piège |
| `synthese_etat.md` | ~438, ~444, ~485 | sortie réelle de la carte avant correction |

Ne pas les « corriger » : elles perdraient leur sens.

### 5.3 Divergences Table A ↔ code — tranchées le 22/10 au soir

Trois d'entre elles ont été arbitrées **par la cohérence**, pas au jugé.

#### `CfgS.boot` : la valeur est `1`, pas `OK`

La réponse était dans la ligne elle-même :

```
CfgS.boot   TYPE = boolean    FORMAT_VALUE = 1|true    NATURE = Etat
            VPIV_EX = $I:CfgS:boot:*:OK#      ← contredit sa propre ligne
```

Et le firmware émet bien `sendInfo("CfgS","boot","*","1")`. **C'est l'exemple qui mentait**,
troisième victime de la colonne non migrée. Corrigé en `$I:CfgS:boot:*:1#`.

La convention du projet se lit en creux : `TYPE=enum` avec `OK` dans `FORMAT_VALUE` →
accusé de réception ; `TYPE=boolean` → état `1`/`0`.

#### `act` : `1`/`0` partout, 9 contre 2

| | `1`/`0` | `OK` |
|---|---|---|
| Table A, les 20 lignes `act` | **20** | 0 |
| Firmware, les 11 dispatcheurs | **9** | 2 — `Ctrl_L`, `US` |

Les deux retardataires ont été alignés sur le motif majoritaire, qui renvoie **l'état
obtenu** (`on ? "1" : "0"`) et non un « OK » qui n'apprend rien à SP :

```c
sendInfo("Ctrl_L", "act", "*",  on ? "1" : "0");            // dispatch_Ctrl_L.cpp
sendInfo("US",     "act", inst, cfg_us_active ? "1" : "0"); // dispatch_Us.cpp
```

Les exemples `$I:Ctrl_L:act:*:OK#` et `$I:US:act:*:OK#` de la Table A ont suivi.

#### 🔴 `Us` au lieu de `US` — un vrai défaut, pas une erreur de documentation

Les **onze** émissions du module ultrason écrivaient la variable en minuscule :

```
RÉCEPTION   vpiv_dispatch.cpp:36   {"US", US::dispatch}   accepte "US"    ✅
ÉMISSION    11 sites               sendInfo("Us", …)      SP n'y voit rien ❌
Table A     US          Node-RED   US          personne n'attend "Us"
```

**La Mega obéissait aux commandes ultrason, mais ses accusés et ses mesures étaient jetés
en silence par SP** — un capteur qui a l'air mort alors qu'il fonctionne. Corrigé sur les
onze sites. ⚠️ **Non vérifié sur matériel** : le robot n'est pas monté.

#### Restent ouvertes

| Point | État |
|---|---|
| `Odom.coeff` présente dans le code, absente de la Table A | ⏳ à ajouter |
| `Mic.rms`, `System`, `ctrl_L` minuscule | ✅ exemples corrigés — n'existaient nulle part |

### 5.5 Node-RED — audité, non modifié

Rapport complet : **`docs/audit_nodered_ordre_VPIV.md`**. L'essentiel :

- le parseur JS vivant découpe **dans le bon ordre** ;
- flux **RPi** (en service) : **zéro** trame inversée ;
- 🔴 flux **Xiaomi** : le joystick construit `$V:Mtr:*:cmd:…`, à l'ancien ordre. Il
  fonctionnait **grâce** au défaut du firmware — deux erreurs se compensaient. **La
  correction du 22/10 le casse** ;
- `node-red/config/varBrute.csv` est une **troisième copie de la Table A**, antérieure à
  plusieurs renommages (`System.boot`, `Lring.int`) et inversée à 151/245. Elle n'est
  chargée par rien, mais **c'est d'elle que venait le `$I:System:boot:*:OK#` des
  commentaires de `main.cpp`**.

### 5.4 Erreurs de la synthèse générale SE (02/26) — document externe

Relevées en vérifiant ses affirmations contre le code. **Le document ne doit pas être
suivi sur ces points :**

| Le document dit | Réalité |
|---|---|
| **§8.1 : `1 = JOYSTICK`, `2 = VOCALE`** | 🔴 **`config.cpp` l.22 et Table A : `0=ÉCRAN 1=VOCALE 2=JOYSTICK 3=LAISSE 4=LAISSE+VOCALE`** |
| `$I:Urg:raison:SE:URG_CPU_CHARGE#` | `$E:Urg:source:SE:…` — catégorie *et* propriété fausses |
| `$A:Expr:modeComVcal:*:1#` | `Expr` n'est pas une des 24 variables de la Table A |
| `$I:EchVoc:*:<0\|1>#` | `EchVoc` n'existe pas, et la trame n'a que 4 champs |
| variable `Com`, propriété `erreur` | `COM`, propriété `error` |
| `URG_LOW_BAT_SE`, `URG_MQTT_LOST` | absents du code |
| §10.7 : `rz_parse_vpiv_fifo.sh`, `rz_tasker_listener.sh`, `rz_termux_listener.sh`, `rz_encode_vpiv.sh` | **aucun de ces quatre scripts n'existe** |
| §9.1 : modèle de catalogue `id/synonyms/category/context/priority/scope/vpiv` | remplacé par la Table D réelle, 16 colonnes |

> **§8.1 est le plus dangereux** : suivre le document reviendrait à demander le pilotage
> vocal en envoyant `2`, et à obtenir la manette. Sur une variable de sécurité.

---

## 6. Rejouer la correction

Les deux scripts sont déterministes et ne modifient jamais leur source.

```bash
# 1) La colonne VPIV_EX de la Table A
python3 corriger_vpiv_ex.py  TableA_source.csv  TableA_corrige.csv  audit.csv

# 2) Les commentaires du firmware — simulation d'abord, TOUJOURS
python3 corriger_commentaires_vpiv.py  TableA_corrige.csv  <racine_depot>
python3 corriger_commentaires_vpiv.py  TableA_corrige.csv  <racine_depot>  --appliquer
```

Le second refuse de toucher une trame hors commentaire : il la signale et passe.
Il distingue commentaire et code par un automate à états (code / `//` / `/* */` /
`"…"` / `'…'`), et non par une expression régulière — une regex se fait piéger par
le `//` d'une URL dans une chaîne.

---

## 7. Point de retour

```bash
git reset --hard origin/sauvegarde/avant-correction-VPIV-2026-10-22
```

Branche distante poussée sur GitHub avant toute modification de cette campagne.

> Une étiquette annotée `avant-correction-VPIV_EX-2026-10-22` avait été posée d'abord,
> mais le jeton d'accès refuse l'écriture sur `refs/tags` (HTTP 403) alors qu'il accepte
> les branches. Le point de retour est donc une **branche**, et il est bien sur GitHub.

---

## 8. Ce qui reste à faire

| Tâche | Où | Demande le robot monté ? |
|---|---|---|
| 🔴 Corriger le joystick Xiaomi : `$V:Mtr:*:cmd:…` → `$V:Mtr:cmd:*:…` | `flows-XIAOMI`, nœud `→ v,omega` | **oui**, pour l'essai |
| 🔴 Vérifier les 11 émissions `US` et les 2 `act` sur matériel | Mega | **oui** |
| Réinjecter `VPIV_EX` corrigée dans le `.ods` de travail | `A - Architecture_Tables_Schemas\Tables\` | non |
| Remplacer `varBrute.csv` par la Table A corrigée | `node-red/config/` | non |
| Ajouter `Odom.coeff` à la Table A | Table A | non |
| Écrire `installation_SE_termux.md` — **rien n'existe** | `docs/` | non |
| Corriger §8.1 de la synthèse SE, ou la marquer périmée | document externe | non |
| Verser `TableC.json`, `doc_A1`, `doc_A2` au dépôt | dépôt | non |
| `reference_SP_nodered.md` | différé — chantier en cours | non |

✅ Tranchées le 22/10 : `CfgS.boot` → `1` ; `Ctrl_L.act` et `US.act` → `1`/`0` ;
`Us` → `US` ; les 14 fichiers vides supprimés.

---

*Fin du document.*
