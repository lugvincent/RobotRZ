# Référence du firmware Arduino — RobotRZ

> **Fichier** : `docs/reference_firmware_arduino.md`
> **Objet** : permettre de relire et de retoucher le code Arduino sans avoir à
> le redécouvrir. Où est quoi, qui appelle qui, que se passe-t-il quand une
> trame arrive, et quels sont les pièges du code tel qu'il est.
> **Comment il a été écrit** : **depuis les sources du dépôt**, pas depuis la
> documentation existante. Le 22/10/2026, la confrontation au code a montré
> que les documents Arduino antérieurs comportaient des erreurs de fond — une
> annexe fausse sur cinq modules, un « mode legacy » qui n'existe pas, et
> deux tiers des trames citées dans l'ordre inverse. Le code ne ment pas sur
> ce qu'il fait ; ses commentaires, eux, le peuvent, et le §11 les recense.
> **Articulation** : `docs/tables_RZ_descripteur.md` pour la norme des
> variables et du protocole · `docs/synthese_etat.md` §9.5 pour le défaut de
> réception corrigé · `docs/travaux_robot_assemble.md` pour ce qui attend le
> robot monté.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe / Claude
> **État du code décrit** : commit `bf0e422` et suivants.

---

## 1. Les deux cartes

| | Arduino **Mega 2560** | Arduino **UNO** |
|---|---|---|
| Rôle | Capteurs, actionneurs, sécurité, protocole | Pilotage des moteurs, exclusivement |
| Code | `src/mega/main.cpp` + toute la librairie | `src/uno/main.cpp`, autonome |
| Parle à | SE par USB (`Serial`), UNO par `Serial3` | La Mega seulement |
| Connaît VPIV | Oui | **Non** |

L'UNO est **invisible depuis SE et SP**. Il ne reçoit jamais de VPIV : la Mega
lui envoie un format court qui lui est propre (§7).

**Mémoire utilisée** (compilation du 22/10/2026) : RAM 2 825 octets sur 8 192
(34,5 %), Flash 45 674 sur 253 952 (18 %). Il reste de la marge, surtout en
Flash ; la RAM est le poste à surveiller.

---

## 2. L'arborescence, et la règle qui l'explique

```
lib/RZlibrairiesPersoNew/src/
 ├── config/          config.h, config.cpp   — constantes et variables globales
 ├── communication/   VPIV : parser, routeur, 14 dispatchers
 ├── system/          urg — urgences
 ├── safety/          mvt_safe — sécurité anti-collision
 ├── control/         ctrl_L — pilotage par laisse
 ├── sensors/         us, fs, mic, mvt_ir
 ├── actuators/       mtr, srv, lring, lrub
 ├── navigation/      odom — odométrie
 └── hardware/        accès bas niveau, un fichier par module
```

### La règle des trois couches

Elle structure tout le code, et c'est elle qu'il faut avoir en tête pour s'y
retrouver. Chaque module fonctionnel existe en trois fichiers :

| Couche | Dossier | Fait | Ne fait jamais |
|---|---|---|---|
| **Hardware** | `hardware/` | Broches, timers, périphériques | Aucune logique, aucune trame |
| **Métier** | `sensors/`, `actuators/`, … | Calculs, états internes, décisions | Accès direct aux broches |
| **Communication** | `communication/dispatch_*.cpp` | Décoder, valider, router | **Aucun calcul, aucun accès matériel** |

> 🛑 Un dispatcher ne calcule rien, ne touche jamais au matériel, n'invente
> aucun état. S'il le fait, le découpage est rompu.

Exemple complet, les servomoteurs : `hardware/srv_hardware.cpp` pilote le PWM,
`actuators/srv.cpp` borne les angles entre 0 et 180 et mémorise les consignes,
`communication/dispatch_Srv.cpp` décode `$V:Srv:angle:TGD:90#` et appelle
`srv_setAngle()`.

**Le point d'entrée de la librairie** est `RZlibrairiesPersoNew.h`. Il n'expose
que ce dont `main.cpp` a besoin, et déclare `RZ_initAll()`.

---

## 3. Le démarrage — `RZ_initAll()`

`setup()` ne contient qu'un appel. Tout est dans `RZlibrairiesPersoNew.cpp` :

```
initConfig()          ouvre Serial et MOTEURS_SERIAL, pose les valeurs par défaut
cfg_modeRz = 0        ARRÊT — sécurité au boot
communication_init()  buffers VPIV
urg_init()            sécurité
ctrl_L_init()
  ── actionneurs ──   lring_init, lrub_init, srv_init, mtr_init
  ── capteurs ──      mic_init, us_init, fs_init, mvt_ir_init
  ── navigation ──    odom_init
  ── sécurité ──      mvtsafe_init
sendInfo("CfgS", "boot", "*", "1")
```

Trois choses à retenir.

**`initConfig()` ouvre les ports série.** Ne jamais appeler `Serial.begin()`
dans `setup()` : on obtiendrait une double initialisation.

**Le robot démarre en mode ARRÊT**, pas en veille. C'est délibéré.

**Le message de fin est `$I:CfgS:boot:*:1#`.** L'en-tête de `main.cpp` annonce
`$I:System:boot:*:OK#` — c'est faux, voir §11.

---

## 4. La boucle — huit étapes, aucune bloquante

```c
void loop()
{
    loopStartTime = millis();

    communication_processInput();   // 1. réception et parsing VPIV

    lring_processPeriodic();        // 2. tâches périodiques
    mic_processPeriodic();
    us_processPeriodic();
    fs_processPeriodic();
    mvt_ir_processPeriodic();
    odom_processPeriodic();

    mvtsafe_process();              // 3. sécurité anti-collision
    ctrl_L_update();                // 4. pilotage par laisse
    lrub_processTimeout();          // 5. extinction automatique du ruban
    srv_process();                  // 6. PWM servos, 50 Hz
    mtr_processPeriodic();          // 7. signal de vie vers l'UNO

    if (millis() - loopStartTime > LOOP_MAX_MS)   // 8. chien de garde
        urg_handle(URG_LOOP_TOO_SLOW_A);
}
```

> ⚠ **Jamais de `delay()` dans cette boucle.** Toutes les tâches périodiques
> sont écrites en non bloquant, et un seul appel bloquant ferait sonner le
> chien de garde — qui verrouille le robot (§6).

> ⚠ **L'étape 7 est obligatoire.** L'UNO a un délai de sécurité de 1 000 ms :
> sans ce signal toutes les 100 ms, il freine progressivement jusqu'à l'arrêt.

---

## 5. Le protocole en réception — du caractère au module

C'est le chemin le plus important du firmware, et celui qui portait le défaut
corrigé le 22/10/2026.

### 5.1 Lecture caractère par caractère

`communication_processInput()`, dans `communication.cpp`, vide tout ce que le
port série a reçu. Il ignore CR, LF et les caractères de contrôle. `$` ouvre une
trame, `#` la ferme. Un dépassement du tampon déclenche `URG_BUFFER_OVERFLOW`.

### 5.2 Découpage

`parseVPIV()` découpe sur les deux-points et remplit `MessageVPIV` :

```
$V:Srv:angle:TGD:90#
   │   │     │    └── value — le reste brut, peut contenir ':' ',' '[' ']'
   │   │     └─────── inst  (token 4)
   │   └───────────── prop  (token 3)
   └───────────────── module (token 2)      type = 'V' (token 1)
```

**PROP en position 3, INST en position 4.** Sans aucune permutation.

### 5.3 Filtre d'urgence

Si une urgence est active, seules deux trames passent :

```
$V:Urg:clear:*:1#          effacement direct
$V:CfgS:emg:*:clear#       effacement par CfgS
```

Tout le reste reçoit `$E:Urg:lock:*:STOP actif#`.

### 5.4 Routage

`dispatchVPIV(var, prop, inst, value)` cherche le module dans `ACTION_TABLE`,
une table de quatorze entrées dans `vpiv_dispatch.cpp`. **Un module inconnu
déclenche `URG_PARSING_ERROR`** — donc une urgence, donc un verrouillage. Une
faute de frappe dans un nom de variable immobilise le robot.

### 5.5 Le dispatcher du module

Chaque dispatcher a la même signature et compare `prop` à ses propriétés
connues. Une propriété non reconnue fait retourner `false` — **sans message
d'erreur**. La commande disparaît en silence.

---

## 6. L'urgence

### Ce qui la déclenche

| Code | Cause | Où |
|---:|---|---|
| 0 | `URG_NONE` | — |
| 1 | `URG_LOW_BAT` | non implémenté à ce jour |
| 2 | `URG_MOTOR_STALL` | `mvt_safe.cpp` |
| 3 | `URG_SENSOR_FAIL` | — |
| 4 | `URG_BUFFER_OVERFLOW` | `communication.cpp` |
| 5 | `URG_PARSING_ERROR` | `vpiv_dispatch.cpp`, module inconnu |
| 6 | `URG_LOOP_TOO_SLOW_A` | `main.cpp`, boucle > 80 ms |
| 7 | `URG_US_DANGER` | — |
| 8 | `URG_MVT_DANGER` | — |
| 9 | `URG_UNKNOWN` | repli |

### Ce qu'elle fait, dans l'ordre

1. **Arrêt immédiat des moteurs**, sans attendre SP — `urg_stopAllMotors()`
2. **Signalisation sur l'anneau de LED** — `lring_emergencyTrigger(code)`
3. **Deux trames** : `$E:Urg:source:*:<cause>#` puis `$E:Urg:etat:*:active#`

Une urgence déjà active n'est pas réarmée : `urg_handle()` sort immédiatement.
Pas d'escalade, pas de second message.

### L'effacement

`urg_clear()` remet l'anneau au neutre et publie `$I:Urg:etat:*:cleared#`.

> ⚠ **Rien d'autre ne l'efface.** Aucun délai, aucune condition de retour à la
> normale. Tant que SP n'envoie pas le `clear`, le robot reste bloqué.

> 🔴 **Sur établi, sans capteurs, l'urgence revient à la fin du tour de boucle
> même où on l'efface.** NewPing attend l'expiration pour chacun des neuf
> ultrasons et le HX711 attend une donnée qui ne vient pas : les 80 ms sont
> dépassées presque à chaque itération. Aucune commande n'est testable dans
> ces conditions. Voir `docs/travaux_robot_assemble.md` §1.1.

---

## 7. La liaison Mega → UNO

Format propre à cette liaison, **ce n'est pas du VPIV** :

```
<L,R,A>        L, R  vitesses signées, bornées à ±400
               A     intensité d'accélération, 0 à 4
```

`<120,-80,2>` fait avancer en virant. `A=0` applique le changement
immédiatement ; `A=1` à `4` lissent de plus en plus — pas maximum par cycle de
`{400, 40, 24, 12, 6}`, valeurs calibrées pour un robot d'environ 4 kg à roues
de 10 cm.

Un format ancien `<acc,spd,dir>` en valeurs 0 à 9 reste accepté par l'UNO.

**Câblage** : `Serial3` de la Mega vers RX pin 0 et TX pin 1 de l'UNO, masses
reliées, 115 200 bauds des deux côtés.

> ⚠ **Débrancher la Mega avant de téléverser sur l'UNO** : les deux se
> disputent le port série de l'UNO.

---

## 8. La configuration — `config.h`

Un fichier unique pour toutes les constantes et toutes les variables globales
réglables. Toute variable qui s'y trouve **peut être écrasée à l'exécution** par
VPIV : c'est la contrepartie Arduino de la Table C.

Les repères principaux :

```c
#define VPIV_MAX_MSG_LEN 128       // longueur maximale d'une trame
#define MAIN_SERIAL      Serial    // vers SE, 115200
#define MOTEURS_SERIAL   Serial3   // vers UNO, 115200

#define FS_DOUT_PIN  A4            // capteur de force HX711
#define FS_SCK_PIN   A5
#define MVT_IR_PIN   8
#define LRING_PIN    6             // 12 pixels
#define LRUB_PIN     10            // 15 pixels

#define SERVO_TGD_PIN 4            // tête gauche-droite
#define SERVO_THB_PIN 5            // tête haut-bas
#define SERVO_BASE_PIN 9           // rotation du tronc
#define SERVO_TGD_ZERO 30          // positions neutres — à revérifier
#define SERVO_THB_ZERO 70          //   après remontage
#define SERVO_BASE_ZERO 0

#define ODO_ENCODER_GA_PIN 2       // encodeurs, broches à interruption
#define ODO_ENCODER_GB_PIN 19
#define ODO_ENCODER_DA_PIN 18
#define ODO_ENCODER_DB_PIN 3
#define ODO_WHEEL_DIAMETER_MM_DEFAULT 100   // ⚠ valeurs indicatives,
#define ODO_TRACK_MM_DEFAULT 300            //   à mesurer sur le robot réel
#define ODO_ENCODER_CPR_DEFAULT 256
```

---

## 9. Les modules et leurs propriétés

**Ce tableau est extrait des `strcmp()` des dispatchers** : ce sont les
propriétés que le firmware accepte réellement en entrée, ni plus ni moins.

| Module | Propriétés acceptées |
|---|---|
| `CfgS` | `modeRZ` `typePtge` `emg` `reset` |
| `Urg` | `clear` |
| `Mtr` | `act` `cmd` `kturn` `override` `read` `scale` `stop` |
| `Srv` | `act` `angle` `read` `speed` |
| `US` | `act` `delta` `freq` `mode` `read` `sendall` `suspect_ms` `thd` |
| `FS` | `act` `cal` `freq` `read` `tare` `thd` |
| `Mic` | `modeMicsF` `modeMicsI` `paraMicsF` `paraMicsI` |
| `IRmvt` | `act` `freq` `mode` `read` `thd` |
| `Odom` | `act` `coeff` `compass` `freq` `freqPos` `freqVel` `gyro` `mode` `paraOdom` `read` `report` `reset` |
| `SecMvt` | `act` `mode` `scale` `status` `thd` |
| `Ctrl_L` | `act` `dead` `dist` `status` `vmax` |
| `Lring` | `act` `clr` `expr` `init` `lumin` `mode` `rgb` |
| `Lrub` | `act` `col` `effetFuite` `init` `lumin` `timeout` |
| `COM` | `debug` `error` `info` `warn` |

> **La Table A en liste davantage.** C'est normal : elle inclut aussi les
> propriétés que le firmware **émet** sans les recevoir — `US.val`,
> `FS.force`, `Odom.pos`, `Urg.etat`… Les deux listes ne sont pas
> contradictoires, elles ne couvrent pas la même chose.
>
> 🔴 **Un écart réel subsiste** : `Odom.coeff` est accepté par le code et
> **absent de la Table A**. Ligne à ajouter.

---

## 10. L'émission

`sendVPIV(type, module, prop, inst, value)` construit la trame et l'écrit sur
`MAIN_SERIAL`, suivie d'un saut de ligne pour faciliter la lecture côté
Node-RED. Trois raccourcis l'utilisent :

| Fonction | CAT | Usage |
|---|---|---|
| `sendInfo()` | `I` | État, ACK, confirmation |
| `sendError()` | `E` | Erreur, et urgence |
| — | `F` | Flux continu, émis par les modules capteurs |

**L'ordre d'émission est correct** : `$CAT:MODULE:PROP:INST:VALUE#`. C'est la
réception qui était fautive, jamais l'émission.

---

## 11. 🔴 Les pièges du code tel qu'il est

Relevés en lisant les sources. Aucun n'est corrigé à ce jour, sauf mention.

**1. ✅ CORRIGÉ le 22/10/2026 — les exemples en commentaire employaient l'ordre ancien.**

État au moment de la rédaction de ce document : 85 exemples à l'ancien ordre dans
15 fichiers, contre 45 au bon ordre. Un commentaire comme `// $V:Odom:*:read:pos#`
plaçait l'instance en position 3.

**Corrigé depuis**, commit `9787ef8` : 89 exemples remis dans l'ordre normatif sur
17 fichiers, dont 81 permutations `INST`↔`PROP`. Aucune trame inversée ne se trouvait
hors commentaire — le code vivant était déjà sain, et la preuve en est faite dans le
message de commit.

**La cause racine a été trouvée le même jour, et ce n'était pas une négligence de
rédaction** : la colonne `VPIV_EX` de la Table A n'avait jamais été migrée lors du
changement de convention. 150 de ses 251 exemples portaient l'ancien ordre, et c'est
d'elle que tout le monde recopiait — y compris le code de réception de la Mega.
Lire `docs/convention_VPIV_ordre.md`.

⚠️ Quatre trames à l'ancien ordre subsistent **volontairement** dans `docs/` (une ici
au §9, trois dans `synthese_etat.md`) : ce sont les pièces à conviction du défaut.
Ne pas les corriger.

**2. Deux constantes pour le chien de garde, dont une morte.** `config.h`
ligne 19 définit `LOOP_MAX_TIME_MS 100` — qui n'est utilisé nulle part.
`main.cpp` ligne 72 définit son propre `LOOP_MAX_MS = 80`, qui est le seul
effectif. Modifier celui de `config.h` ne produit aucun effet.

**3. `cfg_urg_messages[]` est désynchronisé de l'énumération.** `config.cpp`
ligne 191 déclare huit libellés avec un `"OVERHEAT"` surnuméraire en
position 1 ; `urg.h` compte dix valeurs et n'a pas d'`URG_OVERHEAT`. Indexer
l'un par l'autre donnerait des libellés décalés d'un cran et deux lectures hors
limites. **Ce n'est pas un bug aujourd'hui** — le tableau n'est lu nulle part,
la fonction utilisée est `urg_reason_str()` dans `urg.cpp`, qui est correcte.
Mais les deux symboles sont déclarés à trois lignes l'un de l'autre dans
`config.h`. À supprimer ou à aligner.

**4. Code mort dans le chien de garde.** `main.cpp` lignes 140-144 construit un
`char buf[16]` par `snprintf` qui n'est jamais utilisé.

**5. L'en-tête de `main.cpp` annonce un message de démarrage inexact** :
`$I:System:boot:*:OK#`, là où le code émet `$I:CfgS:boot:*:1#`.

**6. Une erreur de nom de module verrouille le robot.** `dispatchVPIV()`
déclenche `URG_PARSING_ERROR` sur un module inconnu — donc une urgence, donc un
arrêt complet. Une faute de frappe dans un flux Node-RED suffit.

**7. Une propriété inconnue, en revanche, disparaît en silence.** Le dispatcher
retourne `false` et personne n'émet d'erreur. Deux comportements opposés pour
deux fautes voisines.

**8. `SecMvt` et `mvtSafe` sont le même module.** `SecMvt` est le nom VPIV,
`mvtSafe` l'implémentation. Ne pas chercher deux modules.

---

## 12. Pour retoucher un module — la marche à suivre

1. **Lire la Table A** pour la propriété concernée : nom exact, CAT, instances,
   format de valeur. Elle fait foi, pas les commentaires du code.
2. **Identifier les trois fichiers** du module : `hardware/`, le dossier métier,
   `communication/dispatch_*.cpp`.
3. **Respecter le découpage.** Si la modification demande un calcul dans le
   dispatcher, c'est qu'elle est au mauvais endroit.
4. **Vérifier le temps d'exécution.** Tout ajout dans une fonction
   `*_processPeriodic()` entre dans le budget des 80 ms.
5. **Déclarer la propriété dans le dispatcher**, sans quoi elle sera ignorée
   silencieusement.
6. **Mettre la Table A à jour** si une propriété est ajoutée.
7. **Compiler, téléverser, vérifier l'ACK** avant de brancher quoi que ce soit.

```powershell
pio run -d "<chemin du projet>" -e megaatmega2560 -t upload
```

---

*V1 — 22/10/2026. Écrit depuis les sources du dépôt après que la lecture du
code a révélé un défaut bloquant de réception, invisible dans trois jours
d'examen de la documentation. Les documents Arduino antérieurs sont dans
`AA-Doc-horsGit\D - Arduino\` et son sous-dossier `old\` ; ils gardent une
valeur historique mais ne font plus référence.*
