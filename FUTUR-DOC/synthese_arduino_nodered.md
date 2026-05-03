# Synthese Arduino — Projet RZ
**Version 1.0 — Mars 2026 — Vincent Philippe**

Document de reference Arduino a utiliser pour demarrer le developpement Node-RED (SP).
Source normative : Table_A_Arduino v3.1

---
## 1. Architecture et Protocole VPIV

### Format VPIV sur le fil
```
$CAT:VAR:INST:PROP:VALUE#
```

| Champ | Valeurs | Description |
|---|---|---|
| CAT | V / I / F / E / A | V=Consigne SP>A | I=Info/ACK A>SP | F=Flux continu | E=Evenement critique | A=Application |
| VAR | CfgS, Mtr, Odom... | Module Arduino cible (PascalCase) |
| INST | * / TGD / 2... | Instance ou * global. **INST avant PROP sur le fil** (convention projet) |
| PROP | act, cmd, freq... | Propriete ciblee (camelCase) |
| VALUE | 1, 50,0,2... | Valeur. **JAMAIS de float** : entiers x1000 pour les decimaux |

Exemples :
```
$V:Mtr:*:cmd:50,0,2#          <- SP envoie commande moteur
$I:CfgS:*:modeRZ:1#           <- Arduino confirme modeRZ
$E:Urg:A:source:loop_slow#    <- Arduino signale urgence
$F:Odom:pos:*:1234,567,1570#  <- Arduino publie position
```

### Chemin MQTT Arduino <-> SP

| Sens | Topic MQTT | Description |
|---|---|---|
| SP -> Arduino | `SE/arduino/commande` | SP publie trame VPIV brute. mqtt_bridge.py la lit et l'ecrit sur le port Serie. |
| Arduino -> SP | `SE/arduino/data` | Arduino ecrit sur Serie. mqtt_bridge.py lit et publie sur MQTT. |
| SP -> SE -> Arduino | `SE/commande` puis relai | Pour donnees SE relayees vers Arduino (gyro, compass). |

> **Note bridge** : mqtt_bridge.py v1.0 est un prototype. Traiter la valeur recue sur SE/arduino/data comme une string VPIV brute a parser cote Node-RED.

---
## 2. Regles de Codage des Valeurs

**Regle absolue : pas de float sur le fil VPIV.**

| Propriete | Encodage | Exemple envoye | Valeur reelle |
|---|---|---|---|
| Mtr:scale | entier x1000 | 750 | 0.750 = 75% vitesse max |
| Mtr:kturn | entier x1000 | 800 | 0.800 (confort) |
| SecMvt:scale | entier x1000 | 500 | 0.500 = 50% en mode soft |
| Odom:pos x,y | mm entier | 1234 | 1234 mm = 1.234 m |
| Odom:pos theta | mrad entier | 1570 | 1570 mrad = pi/2 rad = 90 deg |
| Odom:vel v | mm/s entier | 250 | 250 mm/s = 0.250 m/s |
| Odom:vel omega | mrad/s entier | 12 | 12 mrad/s |
| Odom:paraOdom gains | x1000 | 1000 | 1.000 = gain neutre |
| FS:cal | entier x1000 | 950 | 0.950 facteur calibration |
| Gyro (Odom:gyro) | deg/s direct | 12 | 12 deg/s |

---
## 3. Sequence d'Initialisation Arduino (SF_Init_4)

Attendre `$I:System:*:boot:OK#` AVANT tout envoi. Respecter l'ordre :

```
0.  <- $I:System:*:boot:OK#              // Attendre ce message d'abord
1.  $V:CfgS:*:modeRZ:0#                 // ARRET pendant init (securite)
2.  $V:CfgS:*:typePtge:0#               // Pilotage Ecran (defaut)
3.  $V:SecMvt:*:thd:40,20#              // Seuils warn=40cm, danger=20cm
4.  $V:SecMvt:*:mode:soft#              // Ralentissement avant arret
5.  $V:SecMvt:*:scale:500#              // 50% vitesse max mode soft
6.  $V:SecMvt:*:act:1#                  // Activer SecMvt AVANT Mtr
7.  $V:US:*:thd:20#                     // Seuil US 20cm
8.  $V:US:*:freq:100#                   // 100ms entre mesures
9.  $V:US:*:delta:5#                    // Publier si variation > 5cm
10. $V:US:*:act:1#                      // Activer US
11. $V:Mtr:*:scale:1000#                // Vitesse nominale (100%)
12. $V:Mtr:*:kturn:800#                 // Coefficient rotation confort
13. $V:Mtr:*:act:1#                     // Activer moteurs
14. $V:Odom:*:act:0#                    // Pause avant paraOdom
15. $V:Odom:*:paraOdom:65,200,256,1000,1000,50,200,40#  // Geometrie + timers
16. $V:Odom:*:reset:1#                  // Pose a zero (x=0, y=0, theta=0)
17. $V:Odom:*:act:1#                    // Activer odometrie
18. $V:Lring:*:act:1#                   // Activer anneau LED
19. $V:Lring:*:expr:neutral#            // Expression neutre
20. $V:Srv:*:act:1#                     // Activer servos
21. $V:CfgS:*:modeRZ:1#                // -> VEILLE (fin init)
```

> **Valeur paraOdom** : `wheel_mm, track_mm, cpr, gainG_x1000, gainD_x1000, fCalc_ms, fPos_ms, fVel_ms`
> Exemple : `65,200,256,1000,1000,50,200,40` = roue 65mm, voie 200mm, CPR 256, gains neutres, calcul 50ms (20Hz), pos 200ms (5Hz), vel 40ms (25Hz)

---
## 4. Reference Complete des Modules

Legende SOURCE : **P**=SP envoie | **A**=Arduino envoie | **S**=SE fournit (relaye par SP)

### System
*Un seul message. A attendre AVANT tout envoi.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| A | **boot** | I | A→SP | OK |  | `$I:System:*:boot:OK#` |
| | | | | | | *Publié par RZ_initAll() à la fin de l'initialisation Arduino. Indique à SP que l* |

### CfgS
*Configuration globale partagee SE+Arduino. modeRZ et typePtge initialises au boot.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **modeRZ** | V | SP→A | 0=ARRET\|1=VEILLE\|2=FIXE\|3=DEPLACEMENT |  | `$V:CfgS:*:modeRZ:2#` |
| | | | | | | *Mode global du robot. Côté Arduino : number interne mais les valeurs sont les mê* |
| A | **modeRZ** | I | A→SP | 0=ARRET\|1=VEILLE\|2=FIXE\|3=DEPLACEMENT |  | `$I:CfgS:*:modeRZ:2#` |
| | | | | | | *Confirmation du mode appliqué. Même enum que SE pour cohérence.* |
| P | **typePtge** | V | SP→A | 0=ECRAN\|1=VOCALE\|2=JOYSTICK\|3=LAISSE\ |  | `$V:CfgS:*:typePtge:3#` |
| | | | | | | *Mode de pilotage. Active ctrl_L automatiquement si 3 ou 4.* |
| A | **typePtge** | I | A→SP | 0=ECRAN\|1=VOCALE\|2=JOYSTICK\|3=LAISSE\ |  | `$I:CfgS:*:typePtge:3#` |
| | | | | | | *Confirmation type de pilotage appliqué.* |
| P | **emg** | V | SP→A | clear |  | `$V:CfgS:*:emg:clear#` |
| | | | | | | *Efface l'urgence active. Préférer Urg:clear.* |
| A | **emg** | I | A→SP | 0 |  | `$I:CfgS:*:emg:0#` |
| P | **reset** | V | SP→A | true\|1 |  | `$V:CfgS:*:reset:true#` |
| | | | | | | *Réinitialise Arduino via RZ_initAll(). Bloqué si urgence active.* |
| A | **reset** | I | A→SP | true |  | `$I:CfgS:*:reset:true#` |

### Urg
*Urgences critiques. CAT=E = reaction automatique SP obligatoire. Instance toujours 'A'.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **clear** | V | SP→A | 1 |  | `$V:Urg:*:clear:1#` |
| | | | | | | *Efface l'urgence active Arduino.* |
| A | **source** | E | A→SP | loop_slow\|us_danger\|mvt_danger\|... |  | `$E:Urg:*:source:A:loop_slow#` |
| | | | | | | *Cause de l'urgence détectée par Arduino. Instance=A. SP doit réagir automatiquem* |
| A | **etat** | E | A→SP | active |  | `$E:Urg:*:etat:A:active#` |
| | | | | | | *Urgence active côté Arduino. Instance=A. Publié simultanément avec source.* |
| A | **etat** | I | A→SP | cleared |  | `$I:Urg:*:etat:A:cleared#` |
| | | | | | | *Urgence effacée côté Arduino.* |

### Mtr
*Moteurs differentiels via UNO esclave (Serial3). scale et kturn en entiers x1000.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **cmd** | V | SP→A | "v,omega,a" |  | `$V:Mtr:*:cmd:80,0,2#` |
| | | | | | | *v∈[-100..100], omega∈[-100..100], a∈[0..4]. Bloqué si urgence active.* |
| A | **targets** | I | A→SP | "L,R,A" |  | `$I:Mtr:*:targets:80,-80,2#` |
| | | | | | | *Consignes différentielles calculées envoyées à l'UNO.* |
| P | **stop** | V | SP→A | 1 |  | `$V:Mtr:*:stop:1#` |
| | | | | | | *Arrêt propre : v=0, omega=0.* |
| A | **stop** | I | A→SP | OK |  | `$I:Mtr:*:stop:OK#` |
| P | **override** | V | SP→A | stop\|clear |  | `$V:Mtr:*:override:stop#` |
| | | | | | | *Arrêt forcé (stop) ou lever override (clear).* |
| A | **override** | I | A→SP | stop\|cleared |  | `$I:Mtr:*:override:stop#` |
| P | **scale** | V | SP→A | 0–1000 | ×1000 (0=arrêt 1000=normal) | `$V:Mtr:*:scale:750#` |
| | | | | | | *Facteur réduction vitesses encodé ×1000. 750=0.750, 1000=pleine vitesse. Convert* |
| A | **scale** | I | A→SP | 0–1000 | ×1000 | `$I:Mtr:*:scale:750#` |
| | | | | | | *Confirmation facteur scale appliqué, en entier ×1000.* |
| P | **kturn** | V | SP→A | 0–2000 | ×1000 (1000=standard) | `$V:Mtr:*:kturn:800#` |
| | | | | | | *Coefficient rotation encodé ×1000. 500=doux, 800=confort, 1000=standard, 1500=fr* |
| A | **kturn** | I | A→SP | 0–2000 | ×1000 | `$I:Mtr:*:kturn:800#` |
| | | | | | | *Confirmation kturn appliqué, en entier ×1000.* |
| P | **act** | V | SP→A | 0\|1 |  | `$V:Mtr:*:act:1#` |
| | | | | | | *Active (1) ou désactive (0) l'envoi vers l'UNO.* |
| A | **act** | I | A→SP | 0\|1 |  | `$I:Mtr:*:act:1#` |
| P | **read** | V | SP→A | 1 |  | `$V:Mtr:*:read:1#` |
| | | | | | | *Demande l'état courant L,R,A.* |
| A | **state** | I | A→SP | "L,R,A" |  | `$I:Mtr:*:state:80,-80,2#` |
| | | | | | | *Consignes actuelles en réponse à read.* |

### Odom
*Odometrie encodeurs. Toutes valeurs en entiers x1000 (mm, mrad, mm/s, mrad/s). Init obligatoire : act:0 > paraOdom > reset > act:1.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **read** | V | SP→A | pos\|vel\|all |  | `$V:Odom:*:read:pos#` |
| | | | | | | *Lecture immédiate. pos=position, vel=vitesse, all=les deux. Réponse : $I:Odom:po* |
| A | **pos** | I | A→SP | x_mm,y_mm,theta_mrad | mm;mm;mrad | `$I:Odom:pos:*:1234,567,1570#` |
| | | | | | | *Position du robot. Entiers ×1000. x_mm=x×1000(mm), y_mm=y×1000(mm), theta_mrad=t* |
| A | **vel** | I | A→SP | v_mm_s,omega_mrad_s | mm/s;mrad/s | `$I:Odom:vel:*:250,12#` |
| | | | | | | *Vitesse robot. Entiers ×1000. v_mm_s=vitesse_linéaire×1000(mm/s), omega_mrad_s=v* |
| A | **report** | I | A→SP | pos:x,y,t;vel:v,w | mm;mm;mrad;mm/s;mrad/s | `$I:Odom:report:*:pos:1234,567,1570;vel:250,12#` |
| | | | | | | *Rapport combiné one-shot (sur demande). Toutes valeurs en entiers ×1000. Déclenc* |
| P | **reset** | V | SP→A | 1\|true |  | `$V:Odom:*:reset:1#` |
| | | | | | | *Remet pose à zéro (x=0,y=0,theta=0). Déclenche un report one-shot.* |
| A | **reset** | I | A→SP | OK |  | `$I:Odom:reset:*:OK#` |
| | | | | | | *Confirmation reset pose.* |
| P | **act** | V | SP→A | 0\|1 |  | `$V:Odom:*:act:1#` |
| | | | | | | *Active (1) ou met en pause (0) le calcul odométrique.* |
| A | **act** | I | A→SP | 0\|1 |  | `$I:Odom:act:*:1#` |
| | | | | | | *Confirmation état activation.* |
| P | **paraOdom** | V | SP→A | wheel_mm,track_mm,cpr[,gainG,gainD,fCalc | mm;mm;ticks;×1000;×1000;ms;ms;ms | `$V:Odom:*:paraOdom:65,200,256,1000,1000,50,200,40#` |
| | | | | | | *Configuration physique complète. Les 3 premiers sont obligatoires. gainG/gainD×1* |
| A | **paraOdom** | I | A→SP | wheel_mm,track_mm,cpr,gainG,gainD,fCalc, | mm;mm;ticks;×1000;×1000;ms;ms;ms | `$I:Odom:paraOdom:*:65,200,256,1000,1000,50,200,40#` |
| | | | | | | *Echo de confirmation avec TOUTES les valeurs effectives après application.* |
| P | **freq** | V | SP→A | ≥ 1 | ms | `$V:Odom:*:freq:50#` |
| | | | | | | *Période de calcul odométrique (ms). 50=20Hz recommandé. Aussi configurable via p* |
| A | **freq** | I | A→SP | ≥ 1 | ms | `$I:Odom:freq:*:50#` |
| | | | | | | *Confirmation période calcul.* |
| P | **freqPos** | V | SP→A | ≥ 0 | ms | `$V:Odom:*:freqPos:200#` |
| | | | | | | *Période d'envoi automatique de la position (ms). 0=désactivé. 200ms=5Hz recomman* |
| A | **freqPos** | I | A→SP | ≥ 0 | ms | `$I:Odom:freqPos:*:200#` |
| | | | | | | *Confirmation période envoi position.* |
| P | **freqVel** | V | SP→A | ≥ 0 | ms | `$V:Odom:*:freqVel:40#` |
| | | | | | | *Période d'envoi automatique de la vitesse (ms). 0=désactivé. 40ms=25Hz recommand* |
| A | **freqVel** | I | A→SP | ≥ 0 | ms | `$I:Odom:freqVel:*:40#` |
| | | | | | | *Confirmation période envoi vitesse.* |
| S | **gyro** | V | SP→A | deg_s,ts_ms | deg/s;ms | `$V:Odom:*:gyro:12,170932#` |
| | | | | | | *Données gyroscope SE relayées par SP vers Arduino. deg_s=vitesse angulaire axe Z* |
| A | **gyro** | I | A→SP | OK |  | `$I:Odom:gyro:*:OK#` |
| | | | | | | *Confirmation réception données gyro.* |
| S | **compass** | V | SP→A | deg,quality | deg;0-100 | `$V:Odom:*:compass:125,85#` |
| | | | | | | *Cap boussole SE relayé par SP vers Arduino. deg=cap(0-360°,0=Nord), quality=fiab* |
| A | **compass** | I | A→SP | OK |  | `$I:Odom:compass:*:OK#` |
| | | | | | | *Confirmation réception données boussole.* |
| P | **mode** | V | SP→A | idle\|normal\|surv |  | `$V:Odom:*:mode:normal#` |
| | | | | | | *Preset. idle=arrêt calcul+report lent(1s). normal=reprend avec fréquences par dé* |
| A | **mode** | I | A→SP | idle\|normal\|surv |  | `$I:Odom:mode:*:normal#` |
| | | | | | | *Confirmation mode appliqué.* |

### US
*9 capteurs ultrason. Instance = nom capteur ou index. Valeurs en cm.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **read** | V | SP→A | 1 |  | `$V:US:AvC:read:1#` |
| A | **one** | I | A→SP | 0–600 | cm | `$I:US:AvC:one:45#` |
| | | | | | | *Valeur d'un capteur US spécifique en cm.* |
| A | **val** | I | A→SP | CSV 9 valeurs | cm | `$I:US:*:val:120,45,80,200,35,90,150,60,110#` |
| | | | | | | *Toutes les valeurs US.* |
| A | **alert** | I | A→SP | 0–600 | cm | `$I:US:2:alert:12#` |
| | | | | | | *Capteur sous seuil danger. Instance=index numérique.* |
| P | **thd** | V | SP→A | ≥ 0 | cm | `$V:US:*:thd:20#` |
| A | **thd** | I | A→SP | OK |  | `$I:US:*:thd:OK#` |
| P | **freq** | V | SP→A | ≥ 5 | ms | `$V:US:*:freq:100#` |
| A | **freq** | I | A→SP | OK |  | `$I:US:*:freq:OK#` |
| P | **act** | V | SP→A | 0\|1 |  | `$V:US:*:act:1#` |
| A | **act** | I | A→SP | OK |  | `$I:US:*:act:OK#` |
| P | **sendall** | V | SP→A | 0\|1 |  | `$V:US:*:sendall:0#` |
| | | | | | | *1=publie toutes valeurs, 0=seulement si variation > delta.* |
| A | **sendall** | I | A→SP | OK |  | `$I:US:*:sendall:OK#` |
| P | **delta** | V | SP→A | ≥ 0 | cm | `$V:US:*:delta:5#` |
| A | **delta** | I | A→SP | OK |  | `$I:US:*:delta:OK#` |
| P | **suspect_ms** | V | SP→A | ≥ 0 | ms | `$V:US:*:suspect_ms:500#` |
| | | | | | | *Délai sans réponse pour marquer capteur suspect.* |
| A | **suspect_ms** | I | A→SP | OK |  | `$I:US:*:suspect_ms:OK#` |
| P | **mode** | V | SP→A | idle\|normal\|surv |  | `$V:US:*:mode:normal#` |
| A | **mode** | I | A→SP | OK |  | `$I:US:*:mode:OK#` |

### SecMvt
*Securite anti-collision. Activer AVANT Mtr. Surveille US, reduit/arrete moteurs.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **act** | V | SP→A | 0\|1 |  | `$V:SecMvt:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:SecMvt:*:act:1#` |
| P | **mode** | V | SP→A | soft\|hard\|idle |  | `$V:SecMvt:*:mode:soft#` |
| | | | | | | *soft=réduit vitesse, hard=arrêt complet.* |
| A | **mode** | I | A→SP | soft\|hard\|idle |  | `$I:SecMvt:*:mode:soft#` |
| P | **thd** | V | SP→A | "warn,danger" | cm | `$V:SecMvt:*:thd:40,20#` |
| | | | | | | *warn=ralentissement, danger=arrêt.* |
| A | **thd** | I | A→SP | "warn,danger" | cm | `$I:SecMvt:*:thd:40,20#` |
| P | **scale** | V | SP→A | 0–1000 | ×1000 | `$V:SecMvt:*:scale:500#` |
| | | | | | | *Facteur vitesse max mode soft ×1000. Même convention que Mtr:scale.* |
| A | **scale** | I | A→SP | 0–1000 | ×1000 | `$I:SecMvt:*:scale:500#` |
| P | **status** | V | SP→A | 1 |  | `$V:SecMvt:*:status:1#` |
| | | | | | | *Demande état courant.* |
| A | **status** | I | A→SP | 0–3 |  | `$I:SecMvt:*:status:1#` |
| | | | | | | *0=idle, 1=OK, 2=warn, 3=danger.* |
| A | **alert** | I | A→SP | danger_soft |  | `$I:SecMvt:*:alert:danger_soft#` |
| | | | | | | *Alerte seuil danger en mode soft.* |

### Lring
*Anneau NeoPixel expressions faciales. Mode 'expr'=firmware, 'extern'=controle SP direct.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **act** | V | SP→A | 0\|1 |  | `$V:Lring:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:Lring:*:act:1#` |
| P | **expr** | V | SP→A | smile\|angry\|neutral\|... |  | `$V:Lring:*:expr:smile#` |
| | | | | | | *Expression prédéfinie.* |
| A | **expr** | I | A→SP | nom expression |  | `$I:Lring:*:expr:smile#` |
| P | **rgb** | V | SP→A | "R,G,B" | ADC 0–255 | `$V:Lring:3:rgb:255,0,0#` |
| A | **rgb** | I | A→SP | "R,G,B" | ADC 0–255 | `$I:Lring:3:rgb:255,0,0#` |
| P | **clr** | V | SP→A | 1 |  | `$V:Lring:*:clr:1#` |
| | | | | | | *Éteint l'anneau.* |
| A | **clr** | I | A→SP | OK |  | `$I:Lring:*:clr:OK#` |
| P | **int** | V | SP→A | 0–255 | ADC | `$V:Lring:*:int:128#` |
| | | | | | | *Intensité globale.* |
| A | **int** | I | A→SP | 0–255 | ADC | `$I:Lring:*:int:128#` |
| P | **mode** | V | SP→A | expr\|extern |  | `$V:Lring:*:mode:extern#` |
| | | | | | | *'expr'=interne firmware, 'extern'=contrôle SP direct.* |
| A | **mode** | I | A→SP | expr\|extern |  | `$I:Lring:*:mode:extern#` |
| P | **init** | V | SP→A | 1 |  | `$V:Lring:*:init:1#` |
| | | | | | | *Réinitialise l'anneau LED.* |
| A | **init** | I | A→SP | OK |  | `$I:Lring:*:init:OK#` |
| A | **urg** | I | A→SP | état urgence |  | `$I:Lring:*:urg:active_loop_slow#` |
| | | | | | | *Notification visuelle urgence via lring_emergencyTrigger().* |

### Lrub
*Ruban NeoPixel. Instance *=tous, N=pixel, [a,b]=liste. Timeout extinction programmable.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **col** | V | SP→A | "R,G,B" | ADC 0–255 | `$V:Lrub:*:col:255,128,0#` |
| A | **col** | I | A→SP | "R,G,B" | ADC 0–255 | `$I:Lrub:*:col:255,128,0#` |
| P | **lumin** | V | SP→A | 0–255 | ADC | `$V:Lrub:*:lumin:128#` |
| | | | | | | *Intensité ruban.* |
| A | **lumin** | I | A→SP | 0–255 | ADC | `$I:Lrub:*:lumin:128#` |
| P | **act** | V | SP→A | 0\|1 |  | `$V:Lrub:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:Lrub:*:act:1#` |
| P | **timeout** | V | SP→A | ≥ 0 | ms | `$V:Lrub:*:timeout:5000#` |
| | | | | | | *Extinction auto. 0=annule timeout sans éteindre.* |
| A | **timeout** | I | A→SP | durée ms ou annulé | ms | `$I:Lrub:*:timeout:5000#` |
| P | **effetFuite** | V | SP→A | 0\|1 |  | `$V:Lrub:*:effetFuite:1#` |
| | | | | | | *Effet visuel fuite alternance pairs/impairs.* |
| A | **effetFuite** | I | A→SP | 0\|1 |  | `$I:Lrub:*:effetFuite:1#` |
| P | **init** | V | SP→A | 1 |  | `$V:Lrub:*:init:1#` |
| A | **init** | I | A→SP | OK |  | `$I:Lrub:*:init:OK#` |

### Srv
*3 servos : TGD (tete gauche-droite), THB (tete haut-bas), BASE. Angle 0-180 deg.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **angle** | V | SP→A | 0–180 | deg | `$V:Srv:TGD:angle:90#` |
| A | **angle** | I | A→SP | 0–180 | deg | `$I:Srv:TGD:angle:90#` |
| P | **speed** | V | SP→A | ≥ 0 |  | `$V:Srv:TGD:speed:10#` |
| A | **speed** | I | A→SP | ≥ 0 |  | `$I:Srv:TGD:speed:10#` |
| P | **act** | V | SP→A | 0\|1 |  | `$V:Srv:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:Srv:TGD:act:1#` |
| P | **read** | V | SP→A | 1 |  | `$V:Srv:*:read:1#` |
| | | | | | | *Demande lecture angle courant par instance.* |

### Ctrl_L
*Pilotage laisse / follow. Active par CfgS:typePtge=3 ou 4. Robot suit a distance cible.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **act** | V | SP→A | 0\|1 |  | `$V:Ctrl_L:*:act:1#` |
| | | | | | | *Active/désactive contrôle laisse. Aussi activé par CfgS:typePtge=3/4.* |
| A | **act** | I | A→SP | OK\|OFF |  | `$I:Ctrl_L:*:act:OK#` |
| P | **dist** | V | SP→A | ≥ 0 | mm | `$V:Ctrl_L:*:dist:500#` |
| | | | | | | *Distance cible mm.* |
| A | **dist** | I | A→SP | OK |  | `$I:Ctrl_L:*:dist:OK#` |
| P | **vmax** | V | SP→A | ≥ 0 |  | `$V:Ctrl_L:*:vmax:50#` |
| | | | | | | *Vitesse longitudinale max correction laisse.* |
| A | **vmax** | I | A→SP | OK |  | `$I:Ctrl_L:*:vmax:OK#` |
| P | **dead** | V | SP→A | ≥ 0 | mm | `$V:Ctrl_L:*:dead:100#` |
| | | | | | | *Zone morte mm. Pas de correction si delta < dead.* |
| A | **dead** | I | A→SP | OK |  | `$I:Ctrl_L:*:dead:OK#` |
| P | **status** | V | SP→A | 1 |  | `$V:Ctrl_L:*:status:1#` |
| | | | | | | *Demande état courant.* |
| A | **status** | I | A→SP | OK\|OFF\|FceKO\|VtKO |  | `$I:Ctrl_L:*:status:OK#` |
| | | | | | | *État laisse. Anti-spam : publié seulement si changement.* |

### Mic
*3 micros Arduino. modeMicsF=flux sonore, modeMicsI=detection evenements. Params CSV.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **modeMicsF** | V | SP→A | OFF\|MOY\|ANGLE\|PIC |  | `$V:Mic:*:modeMicsF:MOY#` |
| | | | | | | *Mode flux microphone Arduino.* |
| A | **modeMicsF** | I | A→SP | OFF\|MOY\|ANGLE\|PIC |  | `$I:Mic:*:modeMicsF:MOY#` |
| P | **modeMicsI** | V | SP→A | OFF\|InfoNuisce\|DetectPresence |  | `$V:Mic:*:modeMicsI:InfoNuisce#` |
| | | | | | | *Mode détection événements sonores Arduino.* |
| A | **modeMicsI** | I | A→SP | OFF\|InfoNuisce\|DetectPresence |  | `$I:Mic:*:modeMicsI:InfoNuisce#` |
| P | **paraMicsF** | V | SP→A | "fMoy,fPic,wMoy,wPic,dSeuil,dAngle" | ms/ms/ms/ms/ADC/deg | `$V:Mic:*:paraMicsF:200,100,200,100,5,10#` |
| | | | | | | *6 entiers CSV paramètres flux.* |
| A | **paraMicsF** | I | A→SP | OK |  | `$I:Mic:*:paraMicsF:OK#` |
| P | **paraMicsI** | V | SP→A | "s0,s1,s2,sP,dNuisce,dPresence,cooldown" | ADC×4/ms×3 | `$V:Mic:*:paraMicsI:30,300,600,100,500,300,2000#` |
| | | | | | | *7 entiers CSV paramètres détection.* |
| A | **paraMicsI** | I | A→SP | OK |  | `$I:Mic:*:paraMicsI:OK#` |
| A | **micMoy** | F | A→SP | ≥ 0 | ADC | `$F:Mic:*:micMoy:512#` |
| | | | | | | *Niveau sonore moyen. Mode MOY requis.* |
| A | **micAngle** | F | A→SP | −180–180 | deg | `$F:Mic:*:micAngle:45#` |
| | | | | | | *Angle d'arrivée du son. Mode ANGLE requis.* |
| A | **micPIC** | F | A→SP | ≥ 0 | ADC | `$F:Mic:*:micPIC:820#` |
| | | | | | | *Niveau sonore pic. Mode PIC requis.* |
| A | **bruitNiv1** | I | A→SP | ≥ 0 | ADC | `$I:Mic:*:bruitNiv1:310#` |
| | | | | | | *Bruit soutenu > seuil1. Mode InfoNuisce requis.* |
| A | **bruitNiv2** | I | A→SP | ≥ 0 | ADC | `$I:Mic:*:bruitNiv2:650#` |
| | | | | | | *Bruit soutenu > seuil2.* |
| A | **detectPresenceSon** | I | A→SP | ≥ 0 | ADC | `$I:Mic:*:detectPresenceSon:120#` |
| | | | | | | *Présence sonore détectée. Mode DetectPresence requis.* |

### FS
*Capteur force HX711. Force calibree = (raw-offset)/cal. cal encode x1000. Alerte aux transitions seuil.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **read** | V | SP→A | force\|raw |  | `$V:FS:*:read:force#` |
| | | | | | | *Lecture immédiate force ou raw.* |
| A | **force** | F | A→SP | plage calibration |  | `$F:FS:*:force:320#` |
| | | | | | | *Force normalisée. Périodique et réponse à read.* |
| A | **raw** | F | A→SP | plage HX711 |  | `$F:FS:*:raw:1245600#` |
| | | | | | | *Valeur brute HX711.* |
| P | **tare** | V | SP→A | 1 |  | `$V:FS:*:tare:1#` |
| | | | | | | *Remise à zéro capteur.* |
| A | **tare** | I | A→SP | OK |  | `$I:FS:*:tare:OK#` |
| P | **cal** | V | SP→A | ≠ 0 | ×1000 | `$V:FS:*:cal:950#` |
| | | | | | | *Facteur calibration ×1000 (950=0.950). 0 interdit.* |
| A | **cal** | I | A→SP | OK |  | `$I:FS:*:cal:OK#` |
| P | **act** | V | SP→A | 0\|1 |  | `$V:FS:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:FS:*:act:1#` |
| P | **freq** | V | SP→A | ≥ 0 | ms | `$V:FS:*:freq:200#` |
| | | | | | | *Fréquence publication ms. 0=désactive.* |
| A | **freq** | I | A→SP | OK |  | `$I:FS:*:freq:OK#` |
| P | **thd** | V | SP→A | ≥ 0 | même unité force | `$V:FS:*:thd:80#` |
| | | | | | | *Seuil déclenchement alerte.* |
| A | **thd** | I | A→SP | OK |  | `$I:FS:*:thd:OK#` |
| A | **status** | I | A→SP | alerte\|actif |  | `$I:FS:*:status:alerte#` |
| | | | | | | *État du module FS.* |
| A | **alerte** | I | A→SP | ≥ 0 | même unité force | `$I:FS:*:alerte:95#` |
| | | | | | | *Valeur force au dépassement seuil.* |

### IRmvt
*Detecteur IR presence/mouvement. Valeur brute + alerte si depassement seuil.*

| SRC | PROP | CAT | DIR | FORMAT | CODAGE | VPIV exemple |
|---|---|---|---|---|---|---|
| P | **read** | V | SP→A | 1 |  | `$V:IRmvt:*:read:1#` |
| A | **read** | I | A→SP | OK |  | `$I:IRmvt:*:read:OK#` |
| P | **act** | V | SP→A | 0\|1 |  | `$V:IRmvt:*:act:1#` |
| A | **act** | I | A→SP | 0\|1 |  | `$I:IRmvt:*:act:1#` |
| P | **freq** | V | SP→A | ≥ 0 | ms | `$V:IRmvt:*:freq:500#` |
| A | **freq** | I | A→SP | ≥ 0 | ms | `$I:IRmvt:*:freq:500#` |
| P | **thd** | V | SP→A | ≥ 0 |  | `$V:IRmvt:*:thd:10#` |
| A | **thd** | I | A→SP | ≥ 0 |  | `$I:IRmvt:*:thd:10#` |
| P | **mode** | V | SP→A | idle\|monitor\|surveillance |  | `$V:IRmvt:*:mode:monitor#` |
| A | **mode** | I | A→SP | idle\|monitor\|surveillance |  | `$I:IRmvt:*:mode:monitor#` |
| A | **value** | I | A→SP | ≥ 0 |  | `$I:IRmvt:*:value:23#` |
| | | | | | | *Valeur capteur IR. Publié à cadence freq.* |
| A | **alert** | I | A→SP | 0\|1 |  | `$I:IRmvt:*:alert:1#` |
| | | | | | | *Alerte mouvement détecté (dépassement seuil).* |

---
## 5. Points d'Attention pour Node-RED

**Urgences CAT=E — reaction automatique obligatoire**
> Reception de `$E:Urg:A:source:...#` ou `$E:Urg:A:etat:active#` -> arreter immediatement les moteurs (`$V:Mtr:*:stop:1#`), bloquer toute consigne, alerter l'UI. Aucun VPIV de commande ne passe pendant une urgence active (Arduino refuse). Deblocage : `$V:Urg:*:clear:1#`.

**Inversion INST/PROP sur le fil**
> Le format reel sur le fil est `$CAT:VAR:INST:PROP:VALUE#` (INST avant PROP). Cote Node-RED, parser normalement : token 3 = INST, token 4 = PROP.

**Pas de float dans VALUE**
> Toute valeur decimale est encodee en entier x1000. Lors de l'envoi depuis Node-RED, toujours arrondir et envoyer un entier. Ex: facteur 0.8 -> envoyer 800.

**Attendre le boot avant d'envoyer**
> Ne jamais envoyer de VPIV avant d'avoir recu `$I:System:*:boot:OK#`. L'Arduino n'initialise ses modules qu'a la fin de RZ_initAll(). Subscriber au topic SE/arduino/data en premier.

**Ordre init SecMvt avant Mtr**
> SecMvt:act:1 DOIT preceder Mtr:act:1. La securite anti-collision doit etre operationnelle avant le premier mouvement moteur.

**Sequence Odom obligatoire**
> L'ordre act:0 -> paraOdom -> reset -> act:1 est imperatif. Un paraOdom envoye alors que act=1 peut corrompre les coefficients pendant un calcul en cours.

**Gyro/Compass : relai SP obligatoire**
> SE publie `$F:Gyro:dataContGyro:*:...:gz:...#` -> SP extrait gz (diviser par 100 pour deg/s) et publie `$V:Odom:*:gyro:<gz_dps>,<ts_ms>#` vers Arduino. Ce relai est de la responsabilite de Node-RED.

**enums modeRZ et typePtge**
> modeRZ : 0=ARRET | 1=VEILLE | 2=FIXE | 3=DEPLACEMENT | 4=AUTONOME | 5=ERREUR. typePtge : 0=ECRAN | 1=VOCALE | 2=JOYSTICK | 3=LAISSE | 4=LAISSE+VOCALE. Memes valeurs SE et Arduino.

**Keepalive UNO moteurs (interne Arduino)**
> L'UNO a un timeout safety 1000ms. Si l'Arduino Mega ne lui envoie pas de keepalive toutes les 100ms, l'UNO freine vers 0. Ce mecanisme est interne a l'Arduino (mtr_processPeriodic dans loop). SP n'a rien a faire.

**mqtt_bridge.py v1.0 limitation**
> Le bridge actuel est un prototype qui encapsule en JSON generique. Adapter le parsing Node-RED pour extraire la trame VPIV brute. Une refonte du bridge sera necessaire en production.

---
## 6. Aide-memoire Valeurs d'Init

Valeurs de reference a envoyer depuis SF_Init_4 :

| Module:Prop | Valeur init | Commentaire |
|---|---|---|
| `CfgS:modeRZ` | `0` | ARRET pendant init |
| `CfgS:typePtge` | `0` | Pilotage Ecran |
| `SecMvt:thd` | `40,20` | warn=40cm danger=20cm |
| `SecMvt:mode` | `soft` | Ralentissement puis arret |
| `SecMvt:scale` | `500` | 50% vitesse max mode soft |
| `SecMvt:act` | `1` | Avant Mtr |
| `US:thd` | `20` | Seuil alerte 20cm |
| `US:freq` | `100` | 100ms mesures |
| `US:delta` | `5` | Publication si delta > 5cm |
| `US:act` | `1` |  |
| `Mtr:scale` | `1000` | 100% vitesse nominale |
| `Mtr:kturn` | `800` | Coefficient rotation confort |
| `Mtr:act` | `1` |  |
| `Odom:act (pause)` | `0` | Avant paraOdom |
| `Odom:paraOdom` | `65,200,256,1000,1000,50,200,40` | roue=65mm voie=200mm CPR=256 |
| `Odom:reset` | `1` | Pose a zero |
| `Odom:act` | `1` |  |
| `Lring:act` | `1` |  |
| `Lring:expr` | `neutral` | Expression neutre |
| `Srv:act` | `1` | Tous les servos |
| `FS:freq` | `200` | Publication force 200ms |
| `FS:thd` | `80` | Seuil alerte force |
| `CfgS:modeRZ (fin)` | `1` | VEILLE - fin init |

---
## 7. Codes d'Urgence Arduino

Recus via `$E:Urg:A:source:<code>#` :

| Code | Cause |
|---|---|
| `URG_LOOP_TOO_SLOW` | Boucle loop() > 80ms — surcharge CPU Arduino |
| `URG_BUFFER_OVERFLOW` | Buffer serie overflow — trames trop longues ou trop rapides |
| `URG_PARSING_ERROR` | Trame VPIV invalide recue |
| `URG_MOTOR_STALL` | Moteur bloque (courant trop eleve) |
| `URG_SENSOR_FAIL` | Capteur defaillant |
| `URG_LOW_BAT` | Batterie faible (si moniteur batterie connecte) |

---
*Fichier de reference — source : Table_A_Arduino v3.1 — Projet RZ — Mars 2026*