# Les ultrasons : pourquoi la détection de panne ne fonctionnait pas

> **Registre de décision.** Ce document garde la trace d'un raisonnement, pas seulement de
> son résultat. Il dit ce qui a été écarté et pourquoi — c'est ce qui manque le plus,
> six mois plus tard, quand on se demande pourquoi le code est fait ainsi.
>
> *V1 — 24/10/2026 — Vincent Philippe / Claude*
>
> Point de retour : `git reset --hard origin/sauvegarde/avant-correction-US-2026-10-24`

---

## 1. Comment la question est arrivée

Le banc de test, à son deuxième passage, a montré ceci :

```
Levee de l'urgence demandee : envoi de $V:Urg:clear:*:1#
   reponse : $I:Urg:etat:*:cleared#
   ✅ urgence levee
  1/60 ✅ CfgS.emg      -> $I:CfgS:emg:*:0#
  2/60 🔒 CfgS.modeRZ   -> $E:Urg:lock:*:STOP actif#
```

**L'urgence est levée, et elle revient au cas suivant.** Personne ne l'avait vu avant,
parce que personne n'avait essayé de lever l'urgence puis d'envoyer une commande dans la
seconde.

---

## 2. Le cycle qui se referme sur lui-même

```
urgence active  →  passe US sautée (us.cpp:80)  →  boucle rapide
      ↑                                                  ↓
chien de garde  ←  270 ms  ←  passe US complète  ←  urgence levée
```

Trois nombres l'expliquent entièrement :

| | |
|---|---|
| `pulseIn(usEchoPins[i], HIGH, 30000UL)` — `us_hardware.cpp:33` | **30 ms** quand l'écho ne vient jamais |
| les 9 capteurs dans la même passe — `us.cpp:88` | **×9** |
| `LOOP_MAX_MS = 80UL` — `main.cpp:72` | seuil du chien de garde |

**9 × 30 = 270 ms contre 80 ms.** Le chien de garde n'est pas trop sévère : il signale
correctement une boucle trop lente.

---

## 3. 🔴 Le défaut de fond : la détection de panne n'a jamais tourné

Vincent se souvenait d'un mécanisme distinguant « hors de portée » de « capteur mort ».
**Il existe, il est bien conçu, et il n'a jamais été exécuté une seule fois.**

### La convention, documentée et cohérente dans trois fichiers

```c
// us.h
#define US_UNREPORTED -999  // jamais publié
#define US_DISABLED     -1  // capteur en échec
#define US_SILENCE       0  // timeout pulseIn()

// us_hardware.h:8
//  - us_measureOnce(idx) -> retourne distance en cm (0 si timeout)
unsigned int us_measureOnce(uint8_t idx);
```

### Et la condition qui la contredit

```c
// us.cpp:99
// ----- gestion du mode suspect -----
if (d < 0)                        // ← jamais vrai
{
    ...
    currValue[i] = US_SILENCE;    // ← le corps traite pourtant le cas 0
```

Le type de retour est **`unsigned int`** : la condition est morte **par le type**, pas
seulement par la valeur. Le commentaire, la constante assignée et l'en-tête disent tous
l'intention juste. **Une seule ligne contredit trois fichiers de documentation cohérente.**

### Ce que cela entraînait

| | |
|---|---|
| Le mécanisme de suspicion | jamais exécuté |
| `US_DISABLED` | inatteignable — le **blanc** de l'écran Node-RED n'a jamais pu s'afficher |
| Un timeout | enregistré comme `currValue[i] = 0`, donc **0 cm** |
| `us.cpp:127` — `if (d_u <= dangerThreshold)` | `0 <= 15` est **vrai** → alerte de danger |

> **Les neuf capteurs débranchés émettaient une alerte de danger en continu.** C'est ce
> qu'avait montré le premier passage du banc — `$I:Us:alert:0:20#`, `$I:Us:alert:1:15#`… —
> que j'avais pris pour du bruit de télémétrie.

### Un point rassurant

`mvt_safe.cpp:95` se protège : `if (v <= 0) continue;`. Le module de sécurité du mouvement
**ignore correctement** les zéros. Aucune fausse urgence de proximité n'a eu lieu.

Un fichier sur deux fait la vérification : la signature d'une intention juste, appliquée à
un endroit et oubliée à l'autre.

---

## 4. Ce qui a été écarté, et pourquoi

C'est la partie que ce document existe pour garder.

### ❌ Faire rendre `-1` à `us_measureOnce` sur timeout

**Première idée, fausse.** Elle aurait écrasé `US_DISABLED` (-1) avec `US_SILENCE` (0) :
exactement la distinction qu'on cherche à rétablir. Et le type de retour est
`unsigned int`, donc cela demandait de changer la signature.

> C'est l'audit des commentaires, demandé par Vincent **avant** toute modification, qui a
> arrêté cette erreur. Sans lui, la correction aurait détruit le mécanisme en croyant le
> réparer.

### ❌ Suspendre la détection de panne à l'arrêt

Idée séduisante : un capteur qui ne voit rien pendant que le robot bouge est plus suspect
qu'un capteur immobile face à un mur.

**Écartée par les modes.** `modeRZ` vaut `0=ARRET 1=VEILLE 2=FIXE 3=DEPLACEMENT
4=AUTONOME 5=ERREUR`. Le mode **FIXE** est le mode surveillance : le robot est immobile
**et les ultrasons y sont essentiels**. Suspendre la détection à l'arrêt la désactiverait
là où elle sert le plus.

*Vérifié au passage : le module US ignore totalement `modeRZ`. Il se comporte de la même
façon dans les six modes.*

### ❌ Accepter le délai d'une seconde tel quel

En mode FIXE, un capteur peut légitimement faire face à une pièce vide pendant des heures.
Il serait déclaré en panne au bout d'une seconde, et l'écran virerait au blanc en
permanence.

---

## 5. Ce qui a été retenu

### La latence n'est pas le problème, à cette vitesse

```
5 000 m / 3 600 s = 1,39 m/s

en  30 ms (un capteur)      →   4,2 cm
en 270 ms (passe complète)  →  37,5 cm
```

37 cm en aveugle contre des seuils à 10-20 cm : inquiétant en apparence. **Ça ne l'est
pas**, parce qu'une passe lente signifie que rien n'est à moins de 4 m — soit 2,9 s de
marge. Quand un obstacle approche, la passe redevient rapide : un obstacle à 20 cm renvoie
son écho en **1,2 ms**.

> **Le cas lent est exactement le cas sans danger.** C'est la propriété qui rend tout le
> reste acceptable.

### Les trois correctifs

| # | Correctif | Ce qu'il règle |
|---|---|---|
| 1 | `us.cpp:99` — `if (d < 0)` → `if (d == US_SILENCE)` | ressuscite la détection de panne, tarit les fausses alertes, rend le blanc atteignable |
| 2 | `US_SUSPECT_TIMEOUT_MS_DEFAULT` — 1 000 → 30 000 ms | plus de faux « en panne » en mode FIXE |
| 3 | Échelonnement de la passe, priorité inversée en marche arrière | règle le chien de garde |

Le correctif 1 suffirait à régler le chien de garde **si** le délai restait court : un
capteur désactivé n'est plus interrogé (`us.cpp:90`), donc la passe deviendrait gratuite.
Mais avec 30 s, elle subsiste. **Les correctifs 2 et 3 ne se remplacent pas.**

### Pourquoi l'échelonnement ne dégrade pas la sécurité

Deux objections avaient été soulevées, les deux levées par le code :

**« Les composants en aval attendent les neuf valeurs. »** Ils lisent `currValue[]`
(`us.cpp:13`), un tableau **statique et persistant**. `send_one` (l.45) et l'envoi groupé
(l.148) y puisent tous les deux. Échelonner change la fraîcheur, jamais la présence.

**« Les US sont un élément de sécurité primordial. »** Oui — et le dispositif actuel est
le moins sûr des deux : pendant les 270 ms d'une passe, la Mega ne peut ni recevoir un
ordre d'arrêt, ni exécuter `mvt_safe`. Échelonner **restaure** sa réactivité.

### Aucun nouveau paramètre

`US.suspect_ms`, `US.thd`, `US.freq`, `US.delta`, `US.mode` sont **déjà** dans la Table A,
et `dispatch_Us.cpp` les écrit toutes. Le réglage dynamique existe : seule la valeur par
défaut change. **Pas de cascade de modifications** — la crainte exprimée ne se réalise pas.

---

## 6. 🔬 Une piste à éprouver quand les capteurs seront branchés

Beaucoup de modules HC-SR04 maintiennent la ligne d'écho **haute pendant ~38 ms** quand ils
ne détectent rien, au lieu de rester muets. Si les vôtres font cela :

| Délai de `pulseIn` | Capteur vivant face au vide | Capteur mort |
|---|---|---|
| 30 ms (actuel) | 0 — indiscernable | 0 |
| **au-delà de 40 ms** | ~655 cm, écrêté à 400 | **0** |

Ce serait **la vraie discrimination** cherchée : « ne voit rien » distinct de « mort »,
sans aucun délai de suspicion.

**Non vérifié** — le comportement dépend du module. Mais c'est mesurable en une minute dès
qu'un capteur est branché, et cela rendrait le correctif 2 inutile. À inscrire dans la
campagne du robot monté.

---

## 7. Ce qui reste ouvert

| Question | État |
|---|---|
| Le délai de 30 s est-il le bon ? | arbitraire assumé — à réviser après l'essai des 38 ms |
| Les capteurs arrière doivent-ils être prioritaires en marche arrière ? | ✅ retenu, implémenté au correctif 3 |
| Faut-il une portée réglable par VPIV (`US.portee`) ? | différé — `suspect_ms` suffit pour l'instant |
| `LOOP_MAX_TIME_MS` (100, dans `config.h`) n'est jamais utilisé | à unifier avec `LOOP_MAX_MS` (80, dans `main.cpp`) |
| ⚠️ `$V:US:read:*:1#` mesure les 9 capteurs à portée complète — 270 ms, soit trois fois le seuil du chien de garde | **non corrigé** : `read` est une demande explicite de mesures fraîches, pas une veille. Soit on accepte le coût, soit on refuse `*` et on impose une liste |
| Le cycle `cfg_us_ping_cycle_ms` reste à 200 ms | la passe ne coûte plus que 48 ms : il pourrait descendre à 100 ms et doubler la réactivité. Réglable par `$V:US:freq:*:<ms>#` sans recompiler |

---

*Ce document se lit avec `docs/synthese_etat.md` §9.10 et `docs/travaux_robot_assemble.md` §1.1.*
