# Audit de Node-RED — ordre VPIV et copie de la Table A

> **Rapport d'audit. Rien n'a été modifié dans `node-red/`.**
> SP est le chantier en cours : ce document prépare le travail, il ne le fait pas.
>
> *V1 — 22/10/2026 — Vincent Philippe / Claude*

---

## 1. Ce qu'il faut retenir

| Constat | Gravité |
|---|---|
| Le parseur JS vivant découpe la trame **dans le bon ordre** | ✅ rien à faire |
| Flux **RPi** (celui en service) : **zéro** trame inversée | ✅ rien à faire |
| Flux **Xiaomi** : le joystick construit une commande moteur inversée | 🔴 **le secours ne pilote plus depuis la correction du 22/10** |
| `config/varBrute.csv` : copie de Table A périmée, 151 exemples sur 245 inversés | ⚠️ n'est chargée par rien, mais induit en erreur |

---

## 2. 🔴 Le joystick du Xiaomi ne pilote plus — et c'est la correction qui l'a cassé

### Le fait

`flows-XIAOMI-2026-10-18.json`, nœud **`→ v,omega`**, type `function`, ligne 9 :

```js
msg.payload = `$V:Mtr:*:cmd:${v},${omega},${A}#`;
```

L'instance `*` est en position 3, la propriété `cmd` en position 4. **Ancien ordre.**

### Pourquoi cela marchait avant, et plus maintenant

Le parseur du firmware est positionnel, et il l'a toujours été :

```c
// communication.cpp
if (!nextToken(raw, out.prop, sizeof(out.prop), pos))   // ligne 69 -> position 3
if (!nextToken(raw, out.inst, sizeof(out.inst), pos))   // ligne 73 -> position 4
```

Ce qui a changé le 22/10, c'est l'appel qui suit :

```c
// AVANT (défectueux)      dispatchVPIV(module, msg.inst, msg.prop, value)
// APRÈS (corrigé)         dispatchVPIV(module, msg.prop, msg.inst, value)
//
// signature : dispatchVPIV(var, prop, inst, value)
```

Déroulons les deux cas, avant la correction :

| Trame reçue | `msg.prop` (pos 3) | Ce que le dispatcheur recevait comme `prop` | Résultat |
|---|---|---|---|
| `$V:Mtr:cmd:*:80,0,2#` *(correcte)* | `cmd` | `*` ← c'était `msg.inst` | 🔴 rejetée |
| `$V:Mtr:*:cmd:80,0,2#` *(inversée)* | `*` | `cmd` ← c'était `msg.inst` | ✅ **acceptée** |

> **Deux erreurs se compensaient.** Le joystick du Xiaomi a été écrit pour coller au
> firmware tel qu'il se comportait, c'est-à-dire fautif. Il fonctionnait *grâce* au défaut.
>
> La correction du 22/10 remet le firmware dans la norme — et fait tomber le seul
> consommateur qui s'était aligné sur l'erreur.

### La correction à faire, côté Xiaomi

```js
msg.payload = `$V:Mtr:cmd:*:${v},${omega},${A}#`;
```

Un seul caractère déplacé. **Non vérifiée sur matériel** : le robot n'est pas monté.
À inscrire dans la campagne « robot assemblé ».

### Note de méthode

Je n'ai **pas** observé ce comportement : le robot n'est pas assemblé, et la liaison
Xiaomi n'a pas été essayée. C'est une déduction à partir du code des deux côtés. Elle est
solide — les quatre lignes ci-dessus sont lisibles dans les sources — mais c'est une
déduction, pas un essai.

---

## 3. Le reste des flux est sain

### 3.1 Flux RPi — rien à signaler

`flows-RPI-2026-10-18.json`, 298 nœuds, balayés champ par champ : **zéro trame inversée**,
ni dans du comportement, ni dans de la documentation. Aucun nœud n'y construit de commande
`Mtr` — seulement un commentaire dans `fn_init_traitements`.

### 3.2 Le parseur JS est correct

`functions/fn_stt_intent_router.js`, ligne 111 :

```js
const [iCat, iVar, iProp, iInst, iVal] = parts;
```

Propriété avant instance. Conforme.

### 3.3 Un faux positif qu'il faut nommer

Mon premier balayage a signalé **deux** trames inversées dans du comportement sur le
Xiaomi. Il n'y en a qu'une.

La seconde, dans `RZ_Parser_Trieur_V2`, est à la ligne 23 :

```js
// Format attendu : $I:STT:*:intent:{VAR:INST:PROP:VAL}#     <- commentaire
if (vpiv.var === "STT" && vpiv.prop === "intent") {          <- la vraie logique
```

C'est un **commentaire à l'intérieur** du champ `func` d'un nœud. Mon classeur voyait
« champ `func` donc comportement » sans descendre dans la syntaxe JS. La logique réelle,
ligne 24, teste par **nom de champ** (`vpiv.prop`) et non par position : elle est
insensible à l'ordre. Sans effet.

> Le commentaire mérite quand même d'être corrigé, pour la même raison que les 89 du
> firmware : c'est de lui qu'on recopie.

---

## 4. ⚠️ `config/varBrute.csv` — une troisième copie de la Table A, périmée

Même structure que la Table A (15 colonnes, même en-tête), 411 lignes.

### Elle n'est chargée par rien

Aucune référence à `varBrute` dans les `.js`, les `.json` de flux ou les `README`. C'est
une copie de travail, vraisemblablement importée à la main dans l'onglet `ui_VarG`.
**Sa péremption n'a donc pas d'effet à l'exécution** — elle induit en erreur à la lecture.

### Elle est antérieure à plusieurs renommages

| `varBrute.csv` dit | La Table A et le code disent |
|---|---|
| `System.boot` | `CfgS.boot` — le firmware émet `sendInfo("CfgS","boot","*","1")` |
| `Lring.int` | `Lring.lumin` — `dispatch_Lring.cpp:135` émet `lumin` |

Et il lui manque onze entrées ajoutées depuis : `Appli.IA_Conv`, `LockSE.camera`,
`LockSE.microphone`, `STT.state`, `Voice.output`, `Voice.ttsRate`, entre autres.

> **C'est elle qui explique un mystère de la journée.** Les commentaires de `main.cpp`
> annonçaient `$I:System:boot:*:OK#` — un nom de variable qui n'existe nulle part dans le
> code. Il ne venait pas de nulle part : **il venait de `varBrute.csv`.**

### Sa colonne `VPIV_EX` est malade de la même maladie

```
94 exemples au bon ordre · 151 INVERSÉS · 0 indécidable
```

Soit 62 %, exactement la proportion de la Table A avant correction. Les deux copies
descendent du même ancêtre non migré.

### Ce qu'il faudrait en faire

La remplacer par la Table A corrigée, en vérifiant d'abord ce que l'onglet `ui_VarG`
attend comme colonnes. **Non fait ici**, volontairement : toucher à `node-red/` pendant
le chantier SP exposerait à un conflit avec le travail en cours sur le RPi.

---

## 5. Récapitulatif des gestes à faire, quand le chantier SP s'ouvrira

| # | Geste | Où | Vérifiable sans le robot ? |
|---|---|---|---|
| 1 | `$V:Mtr:*:cmd:…` → `$V:Mtr:cmd:*:…` | Xiaomi, nœud `→ v,omega` | non — demande le robot monté |
| 2 | Corriger le commentaire ligne 23 | Xiaomi, `RZ_Parser_Trieur_V2` | oui |
| 3 | Remplacer `varBrute.csv` par la Table A corrigée | `node-red/config/` | oui, après contrôle des colonnes de `ui_VarG` |
| 4 | Vérifier que `ui_VarG` existe bien côté RPi | RPi | oui |

Le point 4 est un reste de la séance du 20/10 : le flux `ui_VarG` est présent sur le
Xiaomi et introuvable sur le RPi, alors que c'est le RPi qui porte SP.

---

## 6. Comment cet audit a été fait

Script `outils/vpiv/audit_flows.py`. Il charge le JSON de flux, descend dans **tous** les
champs de **tous** les nœuds, et pour chaque trame trouvée compare la position de la
propriété au référentiel tiré de la Table A corrigée. Il classe ensuite selon le type de
nœud et le nom du champ : `comment`, `group`, `tab`, ou les champs `info` / `name` sont
de la documentation ; le reste est traité comme du comportement.

**Sa limite est celle décrite au §3.3** : il ne sait pas qu'un `//` à l'intérieur d'un
champ `func` ouvre un commentaire JavaScript. Il surestime donc le comportement — ce qui
est le bon sens de l'erreur pour un audit, mais oblige à relire chaque signalement.

Il ne modifie rien.

---

*Fin du rapport.*
