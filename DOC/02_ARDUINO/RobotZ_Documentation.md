
# 📖 Documentation RobotZ – Arduino Mega (NewRZ4)

## 1. Présentation du projet
Le projet **RobotZ (NewRZ4)** repose sur une Arduino Mega 2560.  
Il intègre plusieurs capteurs (ultrasoniques, force HX711, microphones, IR, odométrie) et actionneurs (moteurs, servos, LEDs NeoPixel ring et ruban).  
Les échanges de données se font avec **Node-RED** via un protocole série textuel optimisé : **VPIV** (Variable-Propriété-Instance-Valeur).  

L’objectif est d’obtenir une architecture factorisée, modulaire et facilement extensible. Chaque fonctionnalité (capteur/actionneur) est isolée dans un couple `.h/.cpp` et reliée par une librairie personnalisée `RZlibrariesPerso`.

---

## 2. Tableau des pins (config.h)

| Pin Arduino | Module / Capteur | Fichier associé | Fonctions principales |
|-------------|------------------|-----------------|-----------------------|
| 28–43 | 9 capteurs ultrasoniques (HC-SR04 via NewPing) | `ultrasonic.cpp/.h` | `initUltrasonic()`, `fSonarSe()`, `processUltrasonicMessage()` |
| 11, 12 | HX711 (capteur de force) | `force_sensor.cpp/.h` | `initForceSensor()`, `checkForceSensor()` |
| [défini dans config.h] | LEDs Ring NeoPixel | `leds.cpp/.h` | `initLeds()`, `processLedMessage()` |
| [défini dans config.h] | Ruban LED NeoPixel | `leds.cpp/.h` | idem |
| [Servo pins dans config.h] | Servos (tête GD, tête HB, base) | `servos.cpp/.h` | `processServoMessage()` |
| [pins moteurs dans config.h] | Moteurs gauche/droite | `moteurs.cpp/.h` | `processMotorMessage()`, `envoiUno()` |
| [pins microphone array] | Microphones directionnels | `microphone_array.cpp` | `processMicMessage()`, calcul intensité |
| [pins IR] | Détecteur de mouvement IR | `ir_motion.cpp` | `checkIrMotion()` |
| Encodeurs (digital) | Odométrie | `odometry.cpp` | `updateOdometrie()`, `sendOdometryData()` |

---

## 3. Protocole VPIV (Variable-Propriété-Instance-Valeur)

### Format général
```
$<Variable>:<Instances>:<Propriétés>:<Valeurs>#
```

- **Variable** : identifie le module (ex: `B`=Ruban LED, `R`=Ring, `T`=Servo, `M`=Moteurs, `U`=Ultrason, `F`=Force, etc.)
- **Instances** : `*` (toutes), `[n1,n2,...]` (liste), `n` (une seule).
- **Propriétés** : identifie l’action (ex: `C`=couleur, `A`=activation, `V`=vitesse, `D`=distance).
- **Valeurs** : données simples ou tableau.

### Exemple
- `$B:5:C:255,0,0#` → LED du ruban n°5 en rouge.  
- `$T:t:A:45#` → Servo tête gauche/droite à 45°.  
- `$U:[0,1,2]:D:[10,15,20]#` → Seuils de danger pour sonars 0,1,2.  

---

## 4. Messages Arduino ↔ Node-RED

### Arduino → Node-RED
Format :  
```
$<Type>:<Contenu>:<Variable>,<Instances>,<Propriétés>,<Valeurs>#
```

- **Type** :  
  - `I` : Information (`$I:Mouvement détecté:IR,0,,#`)  
  - `F` : Fonction (`$F:led_triste:,,,1#`)  
  - `V` : Valeur capteur (`$V:US:U,0,D,25#`)  
  - `E` : Erreur (`$E:Capteur 0 HS:US,0,,#`)  

### Node-RED → Arduino (exemples)
- Moteurs : `$M:*:[V,W]:[[8],[0]]#` → vitesse=8, direction=0.  
- LEDs ring : `$R:*:C:255,0,0#` → allumer en rouge.  
- Sonar seuil danger : `$U:*:D:10#` → seuil global 10 cm.  
- Servo tête : `$T:t:A:45#` → angle 45°.  

---

## 5. Modules et fichiers

| Fichier | Rôle |
|---------|------|
| `communication.cpp/.h` | Parsing & envoi des messages VPIV |
| `ultrasonic.cpp/.h` | Gestion des capteurs ultrasoniques |
| `force_sensor.cpp/.h` | Gestion du capteur HX711 |
| `leds.cpp/.h` | Contrôle NeoPixel ring et ruban |
| `servos.cpp/.h` | Gestion des servos |
| `moteurs.cpp/.h` | Contrôle moteurs DC |
| `odometry.cpp/.h` | Calculs odométrie avec encodeurs |
| `microphone_array.cpp` | Analyse intensité sonore |
| `sound_tracker.cpp` | Détermination direction source sonore |
| `ir_motion.cpp` | Détection mouvement IR |
| `emergency.cpp/.h` | Gestion arrêts d’urgence |
| `config.h/.cpp` | Centralisation des pins, paramètres initiaux |

---

## 6. Variables et constantes globales

| Nom | Type | Fichier | Usage |
|-----|------|---------|-------|
| `ModeRzB` | `enum` | config.h | Modes de fonctionnement |
| `modeRzB` | `ModeRzB` | config.cpp | Mode courant (init=ARRET) |
| `SONAR_PINS[9][2]` | `uint8_t` | config.h | Tableau TRIG/ECHO sonars |
| `nomSonar[9]` | `const char*` | config.cpp | Noms symboliques sonars |
| `sonarDanger[9]` | `unsigned int` | config.cpp | Seuils danger sonar |
| `transfertSonarCode[]` | `unsigned int` | ultrasonic.cpp | Dernières mesures |
| `transfertSonarCodeOld[]` | `unsigned int` | ultrasonic.cpp | Anciennes mesures |
| `pingTimer[]` | `unsigned long` | ultrasonic.cpp | Timers ping sonar |
| `HX711` | objet | force_sensor.cpp | Lecture force |
| `stripRing`, `stripBand` | objets NeoPixel | leds.cpp | LEDs Ring & Bande |
| `servoTeteGD`, etc. | Servo | servos.cpp | Contrôle servos |
| `encG`, `encD` | Encoder | odometry.cpp | Odométrie moteurs |
| `vitesseMoteurs[]` | int | moteurs.cpp | Consigne moteurs |
| `messageVPIV` | struct | communication.h | Stockage message VPIV |

---

## 7. Tableau Node-RED → Arduino (commandes)

| Message VPIV | Variable | Instances | Propriétés | Traitement |
|--------------|----------|------------|-------------|-------------|
| `$M:*:[V,W]:[[8],[0]]#` | `M` | `*` | `V`=vitesse, `W`=sens | `processMotorMessage()` |
| `$R:*:C:255,0,0#` | `R` | `*` | `C`=couleur | `processLedMessage()` |
| `$B:5:C:0,255,0#` | `B` | `5` | `C`=couleur | idem LED Bande |
| `$T:t:A:45#` | `T` | `t` | `A`=angle | `processServoMessage()` |
| `$U:*:D:15#` | `U` | `*` | `D`=danger | `processUltrasonicMessage()` |
| `$U:[0,2]:D:[20,25]#` | `U` | `0,2` | `D` | seuils différenciés |
| `$F:led_triste:,,,1#` | `F` | - | - | fonction prédéfinie |
| `$C:*:M:1#` | `C` | `*` | `M`=mode | change modeRzB |

---

## 8. Tableau Capteur/Actionneur → Arduino → Node-RED

| Source | Variable | Message envoyé | Exemple | Node-RED usage |
|--------|----------|----------------|---------|----------------|
| Sonar i | `U` | `$V:US:U,<i>,D,<dist>#` | `$V:US:U,3,D,28#` | Détection obstacle |
| HX711 | `F` | `$V:FS:F,0,V,<val>#` | `$V:FS:F,0,V,120#` | Force/poids |
| IR | `I` | `$I:Mouvement:IR,0,,#` | idem | Détection mouvement |
| Encodeurs | `O` | `$V:OD:O,[G,D],V,[120,118]#` | idem | Odométrie |
| Microphones | `S` | `$V:SND:S,0,A,75#` | idem | Intensité sonore |
| Mode robot | `C` | `$I:Mode:C,,M,<mode>#` | idem | Log changement |
| Erreur | `E` | `$E:Capteur HS:U,0,D,0#` | idem | Alerte |

---
