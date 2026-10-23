# Préparation du chantier SP — Node-RED

> **Ce document n'est pas une référence du code SP.** Celle-là, `reference_SP_nodered.md`,
> reste volontairement différée : documenter un chantier en cours revient à écrire sur du
> sable. Ce document-ci sert à **ouvrir** le chantier : ce qui est connu, ce qui est cassé,
> ce qu'il faut trancher, et dans quel ordre.
>
> *V1 — 23/10/2026 — Vincent Philippe / Claude*

---

## 1. L'état des lieux en un tableau

| | RPi 3 — en service | Xiaomi — secours |
|---|---|---|
| Nœuds | 298 | 455 |
| Onglets | 7 | 16 |
| Trames VPIV inversées | **0** | 1 en code vivant, 2 en commentaire |
| `ui_VarG` | ✅ **présent** | ✅ présent |

**Le Xiaomi est l'instance de développement, le RPi l'instance propre.** Le Xiaomi porte
des onglets de travail — `travail_joystickAccelerationEncodageVPIV`,
`travail_Calibrage Odometrie` — et des doublons de versions : `ui_Accueil_v3` et
`ui_Accueil_V2`, `RZ_GestionFlux_V1` et `V2`, `oldvxui_Admin`.

### Les sept onglets du RPi

| Onglet | Rôle apparent |
|---|---|
| `Gtion_Init` | initialisation |
| `ui_Accueil_v4` | tableau de bord |
| `Gtion_EtatGlobal_V2` | l'état global — le cœur décisionnel |
| `RZ_GestionFlux_V3` | la gestion des flux |
| `ui_VarG` | l'édition de la Table C |
| `Diag_SE_Statut` | diagnostic du SE |
| `B2_Variables_Complexes` | les variables à structure |

> **Correction d'une conclusion du 20/10** : la séance avait conclu que `ui_VarG` était
> absent du RPi et présent seulement sur le Xiaomi. **C'est faux** — il est sur les deux.
> Le flux existe bien là où il doit être.

---

## 2. Ce qui est déjà vérifié, et qu'il ne faut pas refaire

Ces points ont été établis les 22 et 23/10, code en main. Rapport complet dans
`docs/audit_nodered_ordre_VPIV.md`.

| Vérifié | Résultat |
|---|---|
| Le parseur JS découpe-t-il la trame dans le bon ordre ? | ✅ oui — `fn_stt_intent_router.js` l.111, `[iCat, iVar, iProp, iInst, iVal]` |
| Les flux du RPi portent-ils des trames à l'ancien ordre ? | ✅ aucune, sur 298 nœuds balayés champ par champ |
| Le broker est-il ouvert ? | ✅ non — `allow_anonymous false`, et les scripts SE passent `-u` / `-P` depuis `keys.json` |
| Des identifiants traînent-ils dans le dépôt ? | ✅ aucun — ni jeton, ni clé, ni mot de passe |

---

## 3. 🔴 Les quatre points à traiter, dans cet ordre

### 3.1 La redirection de la box — **en premier**

Node-RED est joignable depuis internet **sans authentification**, constaté le 23/10 depuis
le Xiaomi hors du domicile, et ouvert depuis plusieurs mois. L'éditeur permet de déployer
du JavaScript quelconque : c'est une exécution de code sur le RPi, dans le réseau
domestique.

Aggravé par le dépôt **public**, où le nom DuckDNS figure dans douze fichiers.

**Décision prise** : traité en premier à l'ouverture du chantier, parce que couper la
redirection tout de suite supprimerait l'accès distant dont Vincent a besoin.

Les gestes, dans l'ordre :

1. `node-red admin hash-pw` pour produire une empreinte de mot de passe
2. la poser dans `adminAuth` de `settings.js` — protège l'éditeur
3. `httpNodeAuth` pour les pages `ui_` — protège le tableau de bord
4. basculer l'accès distant sur un **tunnel** (WireGuard ou SSH) plutôt qu'un port ouvert
5. seulement alors, retirer la redirection

Et la question connexe, à trancher en même temps : la branche publique **`master`** porte
encore l'adresse IPv4 publique de 2025, `SSH (local) Id : u0_a234` et le port `8022`.
Voir `BRANCHES.md`.

> **Piste signalée par Vincent** : une version antérieure savait faire l'authentification,
> et il subsiste peut-être sur le Node-RED du Xiaomi des flux désactivés liés à la
> sécurité, mal configurés à l'époque. À chercher avant de repartir de zéro.

### 3.2 Le joystick du Xiaomi ne pilote plus

`flows-XIAOMI-2026-10-18.json`, nœud `→ v,omega`, ligne 9 :

```js
msg.payload = `$V:Mtr:*:cmd:${v},${omega},${A}#`;   // ancien ordre
```

Il fonctionnait **grâce** au défaut du firmware : avant le 22/10, seules les trames
inversées étaient acceptées. La correction remet la Mega dans la norme et fait tomber ce
consommateur. Un caractère à déplacer :

```js
msg.payload = `$V:Mtr:cmd:*:${v},${omega},${A}#`;
```

⚠️ Non vérifiable sans le robot monté. Inscrit en §1.9 de `travaux_robot_assemble.md`.

### 3.3 `varBrute.csv` est une troisième copie périmée de la Table A

`node-red/config/varBrute.csv`, 411 lignes, même en-tête que la Table A.

| Elle dit | La réalité |
|---|---|
| `System.boot` | `CfgS.boot` |
| `Lring.int` | `Lring.lumin` |
| — | il lui manque 11 entrées ajoutées depuis |
| 151 exemples sur 245 à l'ancien ordre | corrigés dans la Table A depuis le 22/10 |

**Elle n'est chargée par rien** — aucune référence dans les `.js`, les flux ou les
`README`. C'est une copie de travail, probablement importée à la main dans `ui_VarG`.
Sa péremption n'a donc pas d'effet à l'exécution, mais elle induit en erreur à la lecture :
**c'est d'elle que venait le `$I:System:boot:*:OK#` des commentaires de `main.cpp`**.

À remplacer par `outils/vpiv/TableA_VPIV_EX_corrige.csv`, après avoir vérifié quelles
colonnes `ui_VarG` attend.

### 3.4 Le commentaire du décapsulateur STT

`RZ_Parser_Trieur_V2`, ligne 23 : `// Format attendu : $I:STT:*:intent:{…}#` — ancien
ordre. **Sans effet** : la logique de la ligne 24 teste par nom de champ. Mais c'est de
ces commentaires qu'on recopie.

---

## 4. Les questions auxquelles le chantier devra répondre

Elles ne sont pas tranchées, et ce document ne prétend pas y répondre.

| Question | Pourquoi elle se pose |
|---|---|
| Que faire des **16 onglets du Xiaomi** ? | Doublons de versions (`ui_Accueil_v3` / `V2`, `GestionFlux_V1` / `V2`), onglets de travail, `oldvxui_Admin`. Le RPi n'en a que 7 |
| Le Xiaomi doit-il rester un **clone** ou devenir un **secours figé** ? | Aujourd'hui il diverge : il a plus d'onglets, et c'est lui qui porte le joystick |
| Où vit la **Table B** ? | La règle du 18/10 dit : générée sur SP depuis la Table C. Le flux responsable n'a pas été identifié |
| `ui_Traitement` est-il **documenté** ? | C'est lui qui résout `TRADUCTION`, `CALCUL` et `MACRO` venus du SE — mécanisme central du vocal |
| Le **tableau de bord** est-il à jour ? | `ui_Accueil_v4` côté RPi, `v3` et `V2` côté Xiaomi |

---

## 5. Ce que le chantier peut s'appuyer sur, dès maintenant

| Ressource | Ce qu'elle apporte |
|---|---|
| `docs/synthese_generale_RobotRZ.md` | L'entrée unique — 12 chapitres, protocole, tables, urgences, règles d'écriture |
| `docs/convention_VPIV_ordre.md` | **À lire avant de recopier le moindre exemple de trame** |
| `docs/audit_nodered_ordre_VPIV.md` | L'audit complet des flux, et sa méthode |
| `docs/schemas_SE.md` | Les quatre schémas, dont le routage vocal qui traverse SP |
| `outils/vpiv/audit_ordre_global.py` | Rejouable à tout moment : il balaye tout le dépôt et sort en code 1 s'il trouve une faute |
| `outils/vpiv/TableA_VPIV_EX_corrige.csv` | La Table A corrigée, prête à remplacer `varBrute.csv` |

---

## 6. L'ordre proposé

1. **Sécurité** — §3.1, parce que le reste peut attendre et pas ça
2. **`varBrute.csv`** — §3.3, rapide, et cela tarit une source d'erreur
3. **Le commentaire STT** — §3.4, une ligne
4. **Trancher le sort du Xiaomi** — §4, parce que cela conditionne le reste
5. **Le joystick** — §3.2, mais il faudra le robot monté pour l'essayer
6. **Documenter**, alors seulement — `reference_SP_nodered.md`, quand le chantier se stabilise

---

*Ce document sera périmé par son propre succès. Quand le chantier sera clos, c'est
`reference_SP_nodered.md` qui prendra sa place.*
