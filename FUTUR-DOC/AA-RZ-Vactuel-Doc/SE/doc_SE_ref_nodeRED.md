Projet RZ

**SE — Description des modules et VPIV**

Référence pour le développement Node-RED (SP)

Version 1.0  —  2026-03-14  —  Vincent Philippe

Table A SE v3 — scripts corrigés v2.1



# **1. Rôle de SE dans l'architecture**


**SE est le smartphone embarqué sur la plateforme mobile. C'est le cerveau local du robot. Il n'a pas de carte SIM et se connecte au réseau via WiFi (box ou partage de connexion).**


**SE joue trois rôles simultanés :**

- Capteurs — collecte et publie les données de ses capteurs intégrés (gyroscope, magnétomètre, micro, caméra, métriques système)

- Applications — pilote les applications Android à la demande de SP (Zoom, NavGPS, Baby, Tasker, expressions)

- Pont — relaie les VPIV entre SP et l'Arduino (via le bridge MQTT + USB-C)


ℹ  Node-RED (SP) est le maître absolu. SE exécute les ordres de SP, collecte et lui remonte les données, et signale les urgences. SE ne prend aucune décision autonome de haut niveau.


| **Sens** | **Contenu** |
| - | - |
| **SP → SE** | **Consignes de mode ($V:), commandes application ($A:), demandes de config ($V:...paraMicro…)** |
| **SE → SP** | **ACK de chaque consigne ($I:), flux de données capteurs ($I:), événements et urgences ($E:), trames d'init au boot ($F:)** |
| **SE → Arduino** | **Relais des $V: Arduino via MQTT bridge + USB-C** |
| **Arduino → SP** | **SE relaie les $I: Arduino vers SP** |



# **2. Protocole VPIV — rappel format**


$CAT:VAR:PROP:INSTANCE:VALUE\#


| **Champ** | **Description** |
| - | - |
| **CAT** | **V = Consigne  |  I = Info / ACK  |  E = Événement  |  A = Application  |  F = Config-init (boot)** |
| **VAR** | **Module concerné  (ex : CamSE, Gyro, Sys, Appli…)** |
| **PROP** | **Propriété dans le module  (ex : modeCam, dataContGyro, alerte…)** |
| **INSTANCE** | **Sous-élément  (\* = tous, rear/front pour cam, TGD/THB pour servo…)** |
| **VALUE** | **Valeur. Objet JSON entre \{\}. String avec guillemets \\"…\\" dans les événements.** |
| **$ et \#** | **Délimiteurs obligatoires début et fin** |


~~**⚠  Les trames $F: (CAT=F) sont émises par SE au démarrage avant que MQTT soit connecté. SP peut les recevoir en retard ou hors ordre. Elles servent à synchroniser l'état initial.~~**

CAT F = flux


# **3. CfgS — Configuration système globale**


**CfgS est le module de configuration qui conditionne le comportement de tous les autres modules SE. SP doit l'initialiser en premier après le démarrage.**


**modeRZ — mode global du robot :**

| **Valeur** | **Comportement SE** |
| - | - |
| **0 = ARRET** | **Tout arrêté. STT inactif. Aucune commande moteur acceptée.** |
| **1 = VEILLE** | **Robot immobile. STT actif en mode Conv (Gemini prioritaire).** |
| **2 = FIXE** | **Robot en position fixe. STT actif mode Mixte.** |
| **3 = DEPLACEMENT** | **Robot en mouvement. STT mode Cmde (PocketSphinx uniquement). Laisse active si typePtge=3 ou 4.** |
| **4 = AUTONOME** | **Mode autonome. Même restrictions que DEPLACEMENT.** |
| **5 = ERREUR** | **Erreur système. STT inactif. Attente intervention SP.** |


**typePtge — type de pilotage :**

| **Valeur** | **Comportement SE** |
| - | - |
| **0 = Ecran** | **Pas de TTS. Si cam perdue → URG\_SENSOR\_FAIL après urgDelay.** |
| **1 = Vocal** | **TTS actif. STT actif.** |
| **2 = Manette** | **Pas de TTS. Si cam perdue → URG\_SENSOR\_FAIL après urgDelay.** |
| **3 = Laisse** | **Commandes motrices STT bloquées. TTS actif.** |
| **4 = Laisse+Vocal** | **Commandes motrices STT bloquées + TTS actif.** |


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE+A** | **modeRZ** | **\*** | **0|1|2|3|4|5** | **Config** | **$V:CfgS:modeRZ:\*:3\#** | **Mis à jour dans global.json .CfgS.modeRZ. Déclenche recalcul contexte STT.** |
| **SP→SE+A** | **typePtge** | **\*** | **0|1|2|3|4** | **Config** | **$V:CfgS:typePtge:\*:1\#** | **Conditionne TTS, laisse, URG cam.** |
| **SP→SE** | **typeReseau** | **\*** | **WifiSPSsI|WifiInternetBox** | **Config** | **$V:CfgS:typeReseau:\*:WifiInternetBox\#** | **WifiSPSsI = SP partage connexion sans Internet.** |
| **SP→SE** | **reset** | **\*** | **1** | **Event** | **$V:CfgS:reset:\*:1\#** | **Relance tous les managers SE.** |
| **SE→SP** | **modeRZ** | **SE** | **0|1|2|3|4|5** | **ACK** | **$I:CfgS:modeRZ:SE:3\#** | **ACK mode appliqué. ** |
| **SE→SP** | **typePtge** | **SE** | **0|1|2|3|4** | **ACK** | **$I:CfgS:typePtge:SE:1\#** | **ACK typePtge appliqué.** |
| **SE→SP** | **typeReseau** | **SE** | **WifiSPSsI|WifiInternetBox** | **ACK** | **$I:CfgS:typeReseau:SE:WifiInternetBox\#** | **ACK type réseau.** |



# **4. Sys — Métriques système du SE**


**SE surveille en continu ses ressources matérielles et les publie selon le mode demandé. Les alertes sont déclenchées après 10 secondes de dépassement continu.**


**Modes disponibles :**

- OFF — aucune surveillance

- FLOW — publication de chaque métrique à chaque freqCont secondes

- MOY — publication de la moyenne sur nbCycles à chaque freqMoy secondes

- ALERTE — surveillance seuils uniquement, aucun flux de données


**Structure paraSys (envoyée en VALUE JSON) :**

\{"freqCont":10, "freqMoy":60, "nbCycles":3,

 "thresholds": \{

   "cpu"     : \{"warn":80, "urg":90\},          (%)

   "thermal" : \{"warn":70, "urg":85\},          (°C)

   "storage" : \{"warn":85, "urg":95\},          (%)

   "mem"     : \{"warn":80, "urg":90\},          (%)

   "batt"    : \{"warn":30, "urg":15\}           (%) ← SENS INVERSÉ

 \}

\}


**⚠  freqCont et freqMoy sont en SECONDES (pas en Hz). Batterie SENS INVERSÉ : alerte si valeur \< seuil. Contrainte : urg(15) \< warn(30).**

ℹ  Alertes déclenchées après 10s de dépassement continu. warn → $I:COM:warn. urg → $E:Urg:source:SE:\<code\>\#.


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeSys** | **\*** | **OFF|FLOW|MOY|ALERTE** | **Config** | **$V:Sys:modeSys:\*:FLOW\#** | **Aussi émis en $F: au boot.** |
| **SP→SE** | **paraSys** | **\*** | **object JSON** | **Config** | **$V:Sys:paraSys:\*:\{...\}\#** | **Structure complète ci-dessus.** |
| **SE→SP** | **modeSys** | **SE** | **OFF|FLOW|MOY|ALERTE** | **ACK** | **$I:Sys:modeSys:SE:FLOW\#** | **ACK mode appliqué.** |
| **SE→SP** | **dataContCpu** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataContCpu:SE:72\#** | **Mode FLOW, chaque freqCont s.** |
| **SE→SP** | **dataContThermal** | **SE** | **number °C** | **Etat** | **$I:Sys:dataContThermal:SE:42\#** | **Mode FLOW.** |
| **SE→SP** | **dataContStorage** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataContStorage:SE:55\#** | **Mode FLOW.** |
| **SE→SP** | **dataContMem** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataContMem:SE:61\#** | **Mode FLOW.** |
| **SE→SP** | **dataContBatt** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataContBatt:SE:45\#** | **Mode FLOW. Sens inversé pour les seuils.** |
| **SE→SP** | **dataMoyCpu** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataMoyCpu:SE:68\#** | **Mode MOY, chaque freqMoy s.** |
| **SE→SP** | **dataMoyBatt** | **SE** | **number 0–100 (%)** | **Etat** | **$I:Sys:dataMoyBatt:SE:43\#** | **Mode MOY.** |
| **SE→SP** | **uptime** | **SE** | **number (secondes)** | **Etat** | **$I:Sys:uptime:SE:3600\#** | **Ponctuel sur demande. Lu /proc/uptime.** |



# **5. Gyro — Gyroscope du SE**


**Capteur gyroscope intégré au smartphone. Utilisé pour détecter l'inclinaison et le risque de chute de la plateforme.**


**Modes :**

- OFF — capteur inactif

- DATACont — flux brut continu à freqCont Hz

- DATAMoy — flux moyenné à freqMoy Hz

- ALERTE — surveillance seuilAlerte uniquement, statusGyro si changement


ℹ  angleVSEBase est l'angle de montage physique du smartphone sur la plateforme (deg×10). Calculé à la calibration, ne change pas au runtime. 150 = 15.0°.

**⚠  Unités : vitesses angulaires en rad/s × 100. Seuils seuilMesure et seuilAlerte aussi en rad/s × 100.**


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeGyro** | **\*** | **OFF|DATACont|DATAMoy|ALERTE** | **Config** | **$V:Gyro:modeGyro:\*:DATACont\#** | **Aussi émis en $F: au boot.** |
| **SP→SE** | **paraGyro** | **\*** | **object JSON** | **Config** | **$V:Gyro:paraGyro:\*:\{...\}\#** | **freqCont(Hz) freqMoy(Hz) nbValPourMoy seuilMesure\{x,y,z\} seuilAlerte\{x,y\}** |
| **SP→SE** | **angleVSEBase** | **\*** | **number (deg×10)** | **Ini** | **$V:Gyro:angleVSEBase:\*:150\#** | **150 = 15.0°. Correction tangage. Aussi émis en $F: au boot.** |
| **SE→SP** | **modeGyro** | **SE** | **OFF|DATACont|DATAMoy|ALERTE** | **ACK** | **$I:Gyro:modeGyro:SE:DATACont\#** | **ACK mode.** |
| **SE→SP** | **dataContGyro** | **\*** | **object \{x,y,z\}** | **Etat** | **$I:Gyro:dataContGyro:\*:\{x:12,y:3,z:0\}\#** | **rad/s×100 filtré par seuilMesure. Mode DATACont.** |
| **SE→SP** | **dataMoyGyro** | **\*** | **object \{x,y,z\}** | **Etat** | **$I:Gyro:dataMoyGyro:\*:\{x:11,y:2,z:0\}\#** | **Moyenne nbValPourMoy mesures. Mode DATAMoy.** |
| **SE→SP** | **statusGyro** | **\*** | **string** | **Etat** | **$I:Gyro:statusGyro:\*:CHUTE\_DETECTEE\#** | **Émis si seuilAlerte franchi (mode ALERTE).** |



# **6. Mag — Magnétomètre du SE**


**Capteur magnétomètre intégré. Calcule le cap magnétique et géographique du robot.**


**Modes :**

- OFF — inactif

- DATABrutCont — champ magnétique brut \[x,y,z\] en µT×100

- DATAMgCont / DATAMgMoy — cap magnétique continu / moyenné

- DATAGeoCont / DATAGeoMoy — cap géographique continu / moyenné


**⚠  Correction de déclinaison magnétique NON implémentée en v1. Le cap géographique est une approximation.**

ℹ  Cap calculé via atan2(y,x) sur les axes magnétomètre. Unités : champ brut en µT×100, caps en deg×10.


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeMg** | **\*** | **OFF|DATABrutCont|DATAGeoCont|DATAGeoMoy|DATAMgCont|DATAMgMoy** | **Config** | **$V:Mag:modeMg:\*:DATAMgCont\#** | **Aussi émis en $F: au boot.** |
| **SP→SE** | **paraMg** | **\*** | **object JSON** | **Config** | **$V:Mag:paraMg:\*:\{...\}\#** | **freqCont(Hz) freqMoy(Hz) nbValPourMoy seuilMesure\{x,y,z\}** |
| **SE→SP** | **modeMg** | **SE** | **(voir SP→SE)** | **ACK** | **$I:Mag:modeMg:SE:DATAMgCont\#** | **ACK mode.** |
| **SE→SP** | **dataContBrut** | **\*** | **object \{x,y,z\}** | **Etat** | **$I:Mag:dataContBrut:\*:\{x:100,y:210,z:-30\}\#** | **Champ brut µT×100. Mode DATABrutCont.** |
| **SE→SP** | **DataContMg** | **\*** | **number (deg×10)** | **Etat** | **$I:Mag:DataContMg:\*:1250\#** | **Cap magnétique continu. Mode DATAMgCont.** |
| **SE→SP** | **DataMoyMg** | **\*** | **number (deg×10)** | **Etat** | **$I:Mag:DataMoyMg:\*:1248\#** | **Cap magnétique moyenné. Mode DATAMgMoy.** |
| **SE→SP** | **DataContGeo** | **\*** | **number (deg×10)** | **Etat** | **$I:Mag:DataContGeo:\*:1180\#** | **Cap géographique continu. Mode DATAGeoCont.** |
| **SE→SP** | **DataMoyGeo** | **\*** | **number (deg×10)** | **Etat** | **$I:Mag:DataMoyGeo:\*:1179\#** | **Cap géographique moyenné. Mode DATAGeoMoy.** |



# **7. CamSE — Caméra du SE (IP Webcam)**


**La caméra utilise l'application IP Webcam d'Android. SE démarre/arrête IP Webcam selon les ordres de SP et fournit l'URL du flux ou de la photo.**


**Modes :**

- off — am force-stop IP Webcam

- stream — démarrage flux vidéo continu (port 8080/video)

- snapshot — démarrage IP Webcam en mode photo (shot.jpg)

- error — positionné automatiquement sur erreur non critique


**Instances : rear (caméra arrière) | front (caméra avant)**


ℹ  Le profil CPU s'adapte automatiquement : si CPU \> 70%, SE passe en profil 'optimized' (résolution réduite, fps limité). SP n'a pas à gérer ce mécanisme.

**⚠  Si typePtge=0 ou 2 et erreur caméra : SE déclenche URG\_SENSOR\_FAIL après urgDelay secondes.**


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeCam** | **rear|front** | **off|stream|snapshot** | **Config** | **$V:CamSE:modeCam:rear:stream\#** | **Commande principale.** |
| **SP→SE** | **paraCam** | **rear|front** | **object JSON** | **Config** | **$V:CamSE:paraCam:rear:\{"res":"720p","fps":30,"quality":80,"urgDelay":5\}\#** | **res:low|medium|high|720p|1080p. urgDelay en secondes.** |
| **SP→SE** | **snap** | **rear|front** | **1** | **Event** | **$V:CamSE:snap:rear:1\#** | **Capture snapshot immédiate. URL retournée via StreamURL.** |
| **SE→SP** | **modeCam** | **rear|front** | **off|stream|snapshot|error** | **ACK** | **$I:CamSE:modeCam:rear:stream\#** | **ACK mode appliqué.** |
| **SE→SP** | **paraCam** | **rear|front** | **object JSON** | **ACK** | **$I:CamSE:paraCam:rear:\{...\}\#** | **ACK paramètres appliqués.** |
| **SE→SP** | **StreamURL** | **rear|front** | **string URL** | **Etat** | **$I:CamSE:StreamURL:rear:http://192.168.1.10:8080/video\#** | **URL flux ou photo snapshot.** |
| **SE→SP** | **error** | **rear|front** | **CAM\_START\_FAIL|CAM\_IP\_FAIL|CAM\_CONFIG\_FAIL** | **Event** | **$E:CamSE:error:rear:CAM\_START\_FAIL\#** | **Événement erreur.** |


**Codes erreur :**

- CAM\_START\_FAIL — IP Webcam ne répond pas sur le port 8080 après démarrage

- CAM\_IP\_FAIL — adresse IP wlan0 non lisible

- CAM\_CONFIG\_FAIL — cam\_config.json absent ou illisible



# **8. TorchSE — Torche du SE**


**Commande la LED torche du smartphone. SE choisit automatiquement la méthode selon l'état de la caméra.**

- Caméra active → commande via l'API HTTP de IP Webcam (http://IP:8080/enabletorch)

- Caméra inactive → commande via termux-torch (API Android native)


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **active** | **\*** | **On|Off** | **Etat** | **$V:TorchSE:active:\*:On\#** | **Allume ou éteint la torche.** |
| **SE→SP** | **active** | **SE** | **On|Off** | **ACK** | **$I:TorchSE:active:SE:On\#** | **ACK état appliqué.** |



# **9. MicroSE — Microphone du SE**


**Le microphone du smartphone mesure le niveau sonore ambiant et peut localiser la direction du son par balayage servo.**


**Modes :**

- off — inactif

- normal — dataContRMS publié à chaque mesure (freqCont ms)

- intensite — dataContRMS si variation \> seuilDelta + alerte si seuil maintenu holdTime ms

- orientation — balayage servo TGD pour trouver l'angle de la source sonore maximale

- surveillance — intensite + balayage automatique si RMS \> seuilMoyen


**Structure paraMicro :**

\{"freqCont":300,         (ms — période de mesure)

 "winSize":300,           (ms — fenêtre glissante, \>= freqCont)

 "seuilDelta":30,         (RMS×10 — variation min pour envoi dataContRMS)

 "holdTime":500,          (ms — durée maintien seuil avant alerte)

 "seuilFaible":100,       (RMS×10)

 "seuilMoyen":400,        (RMS×10 — seuilFaible \< seuilMoyen \< seuilFort)

 "seuilFort":700,         (RMS×10)

 "pasBalayage":10,        (degrés/étape pour orientation)

 "timeoutOrientation":15\} (secondes — durée max balayage)


**⚠  freqCont et winSize sont en MILLISECONDES. winSize doit être \>= freqCont. Contrainte seuils : seuilFaible \< seuilMoyen \< seuilFort obligatoire.**


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeMicro** | **\*** | **off|normal|intensite|orientation|surveillance** | **Config** | **$V:MicroSE:modeMicro:\*:intensite\#** | **Aussi émis en $F: au boot.** |
| **SP→SE** | **paraMicro** | **\*** | **object JSON** | **Config** | **$V:MicroSE:paraMicro:\*:\{...\}\#** | **Structure complète ci-dessus.** |
| **SE→SP** | **modeMicro** | **SE** | **(voir SP→SE)** | **ACK** | **$I:MicroSE:modeMicro:SE:intensite\#** | **ACK mode appliqué.** |
| **SE→SP** | **dataContRMS** | **\*** | **number (RMS×10)** | **Etat** | **$I:MicroSE:dataContRMS:\*:342\#** | **Publié si |mesure - dernière| \> seuilDelta.** |
| **SE→SP** | **alerte** | **\*** | **faible|moyen|fort|silence** | **Event** | **$E:MicroSE:alerte:\*:fort\#** | **Seuil maintenu \>= holdTime ms.** |
| **SE→SP** | **direction** | **\*** | **number (degrés)** | **Etat** | **$I:MicroSE:direction:\*:45\#** | **Angle source sonore. Mode orientation/surveillance.** |



# **10. STT — Reconnaissance vocale**


**SE intègre un système de reconnaissance vocale PocketSphinx (hors ligne) avec repli sur Gemini (en ligne). Les commandes reconnues sont transmises à SP sous forme d'intent VPIV.**


**Modes :**

- KWS — Keyword Spotting actif (mot de réveil : 'rz' par défaut)

- OFF — STT désactivé


**Contextes STT (calculés automatiquement par SE selon modeRZ) :**

| **Contexte** | **Comportement** |
| - | - |
| **Cmde** | **speed ≠ 0 (robot en mouvement) → PocketSphinx uniquement, pas de Gemini** |
| **Conv** | **modeRZ=1 VEILLE → Gemini prioritaire** |
| **Mixte** | **modeRZ=2/3/4 à l'arrêt → PocketSphinx d'abord, puis Gemini si non reconnu (exit 1)** |


**⚠  SP reçoit l'intent via $A:STT:intent:... et doit décider de l'exécuter ou le refuser. C'est SP qui valide.**

ℹ  STT est inactif si modeRZ=0 ou 5, ou si typePtge ≠ 1 et ≠ 4.


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **modeSTT** | **\*** | **KWS|OFF** | **Config** | **$V:STT:modeSTT:\*:KWS\#** | **Aussi émis en $F: au boot.** |
| **SP→SE** | **paraSTT** | **\*** | **object JSON** | **Config** | **$V:STT:paraSTT:\*:\{...\}\#** | **\{threshold, keyphrase, lang, lib\_path\}. Threshold ex: 1e-20.** |
| **SE→SP** | **modeSTT** | **SE** | **KWS|OFF** | **ACK** | **$I:STT:modeSTT:SE:KWS\#** | **ACK mode.** |
| **SE→SP** | **context** | **SE** | **Cmde|Conv|Mixte** | **Etat** | **$I:STT:context:SE:Mixte\#** | **Contexte actuel. Recalculé à chaque changement modeRZ.** |
| **SE→SP** | **intent** | **SE** | **object JSON** | **Intent** | **$A:STT:intent:SE:\{VAR,PROP,INST,VAL\}\#** | **Commande vocale reconnue. SP doit l'exécuter ou la refuser.** |



# **11. Appli — Applications Android sur SE**


**SE pilote des applications Android à la demande de SP. Chaque application est démarrée/arrêtée via Tasker ou directement (am start).**


**⚠  Tasker doit être actif sur SE. Si Tasker est arrêté, les commandes vers Baby, zoom, BabyMonitor, NavGPS et ExprTasker échouent.**

ℹ  L'état de chaque appli est mis à jour dans appli\_config.json (champs state et last\_change).


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **Baby** | **\*** | **On|Off** | **Etat** | **$A:Appli:Baby:\*:On\#** | **Surveillance bébé. Tâche Tasker RZ\_Baby.** |
| **SP→SE** | **BabyMonitor** | **\*** | **On|Off** | **Etat** | **$A:Appli:BabyMonitor:\*:On\#** | **Tâche Tasker RZ\_BabyMonitor.** |
| **SP→SE** | **zoom** | **\*** | **On|Off** | **Etat** | **$A:Appli:zoom:\*:On\#** | **Tâche Tasker RZ\_Zoom. Package: us.zoom.videomeetings.** |
| **SP→SE** | **NavGPS** | **\*** | **On|Off** | **Etat** | **$A:Appli:NavGPS:\*:On\#** | **Tâche Tasker RZ\_NavGPS. am start direct (pas de dépendance Tasker). Package: com.google.android.apps.maps.** |
| **SP→SE** | **tasker** | **\*** | **On|Off** | **Etat** | **$A:Appli:tasker:\*:On\#** | **Démarre/ferme Tasker lui-même. Package: net.dinglisch.android.taskerm.** |
| **SP→SE** | **ExprTasker** | **Expression** | **string** | **Etat** | **$A:Appli:ExprTasker:Expression:sourire\#** | **Expressions faciales via Tasker RZ\_Expression. L'INSTANCE porte le type d'action.** |
| **SP→SE** | **ExprTasker** | **Off** | **\*** | **Etat** | **$A:Appli:ExprTasker:Off:\*\#** | **Arrêt ExprTasker. Off est dans l'INSTANCE, pas la VALUE.** |
| **SE→SP** | **(toutes)** | **\*** | **On|Off** | **ACK** | **$I:Appli:zoom:\*:On\#** | **ACK état appliqué.** |


ℹ  ATTENTION ExprTasker : l'INSTANCE porte le type d'action (Expression, Off). C'est différent des autres Appli où INSTANCE=\*.



# **12. Voice — Synthèse vocale TTS**


**SE peut parler via son moteur TTS Android. Géré directement par rz\_vpiv\_parser.sh — pas d'ACK retour.**


ℹ  TTS actif uniquement si typePtge ≠ 0. Les guillemets sont retirés avant envoi au moteur TTS.


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SP→SE** | **tts** | **\*** | **string (texte à dire)** | **\_** | **$A:Voice:tts:\*:"Démarrage terminé"\#** | **Pas d'ACK. Ignoré si typePtge=0.** |



# **13. Urg — Urgences**


**SE surveille son état et déclenche des urgences vers SP. SP décide de la réponse et doit ensuite remettre SE en route.**


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SE→SP** | **source** | **SE** | **URG\_\<CODE\>** | **Event** | **$E:Urg:source:SE:URG\_CPU\_CHARGE\#** | **Urgence déclenchée par SE. SP décide du niveau et de la réponse.** |
| **SE→SP** | **statut** | **SE** | **cleared|active** | **ACK** | **$I:Urg:statut:SE:cleared\#** | **Confirmé après clear.** |
| **SE→SP** | **niveau** | **SE** | **warn|danger** | **Etat** | **$I:Urg:niveau:SE:danger\#** | **Niveau urgence courante.** |
| **SP→SE** | **clear** | **\*** | **1** | **Event** | **$V:Urg:clear:\*:1\#** | **SP demande effacement urgence.** |


**Codes urgence émis par SE :**

| **Code** | **Déclencheur** |
| - | - |
| **URG\_CPU\_CHARGE** | **CPU \> seuil urg depuis 10s** |
| **URG\_OVERHEAT** | **Température \> seuil urg depuis 10s** |
| **URG\_SENSOR\_FAIL** | **Storage ou Mem \> seuil urg, ou caméra perdue (typePtge 0 ou 2)** |
| **URG\_LOW\_BAT** | **Batterie \< seuil urg (sens inversé — 15%)** |
| **URG\_PARSING\_ERROR** | **Trame VPIV reçue illisible** |
| **URG\_LOOP\_TOO\_SLOW** | **Boucle principale SE trop lente** |
| **URG\_APPLI\_FAIL** | **Application Android critique crashée** |
| **URG\_BUFFER\_OVERFLOW** | **Buffer VPIV dépassé** |
| **URG\_UNKNOWN** | **Code urgence non reconnu (fallback)** |


**⚠  Pendant une urgence active : SE arrête ses émissions et n'accepte que $V:Urg:clear. Tout autre $V: reçoit $I:COM:error:SE:"urgence active, clear first"\#.**

ℹ  Après clear : SP DOIT réémettre modeSys et modeRZ pour relancer les managers SE arrêtés.



# **14. COM — Messages de diagnostic**


**Canal de messages texte de SE vers SP. Tous les managers SE utilisent COM:error pour les erreurs récupérables.**


| **Direction** | **PROP** | **Instance** | **Valeurs légales** | **Nature** | **Exemple VPIV** | **Notes** |
| - | - | - | - | - | - | - |
| **SE→SP** | **debug** | **SE** | **string** | **\_** | **$I:COM:debug:SE:"gyro init OK"\#** | **Réservé développement. Ne pas activer en production.** |
| **SE→SP** | **info** | **SE** | **string** | **Etat** | **$I:COM:info:SE:"CAMERA\_START\_PRIORITY"\#** | **Information non critique.** |
| **SE→SP** | **warn** | **SE** | **string** | **Etat** | **$I:COM:warn:SE:"CPU 85% depuis 10s"\#** | **Seuil warn franchi. Affichage SP uniquement, pas d'arrêt.** |
| **SE→SP** | **error** | **SE** | **string** | **Etat** | **$I:COM:error:SE:"CamSE rear:CAM\_START\_FAIL"\#** | **Erreur récupérable. Émis par tous les managers.** |



# **15. Récapitulatif — ce que SP doit faire au démarrage**


**Séquence recommandée côté Node-RED pour initialiser SE :**


| **\#** | **Action Node-RED** |
| - | - |
| **1** | **Attendre les trames $F: de SE (init boot) — synchroniser l'état interne de Node-RED** |
| **2** | **Envoyer $V:CfgS:typeReseau:\*:\<type\>\# — SP informe SE du type de réseau** |
| **3** | **Envoyer $V:CfgS:typePtge:\*:\<type\>\# — choisir le mode de pilotage** |
| **4** | **Envoyer $V:CfgS:modeRZ:\*:1\# (VEILLE) — mettre SE en état connu avant tout** |
| **5** | **Envoyer $V:Sys:modeSys:\*:FLOW\# + $V:Sys:paraSys:\*:\{...\}\# — démarrer la surveillance** |
| **6** | **Envoyer $V:Gyro:modeGyro:\*:... et/ou $V:Mag:modeMg:\*:... selon besoin** |
| **7** | **Envoyer $V:MicroSE:modeMicro:\*:... et/ou $V:CamSE:modeCam:rear:... selon besoin** |
| **8** | **Envoyer $V:STT:modeSTT:\*:KWS\# si pilotage vocal souhaité** |
| **9** | **Envoyer $V:CfgS:modeRZ:\*:\<mode\>\# — passer au mode opérationnel souhaité** |


**⚠  Paradoxe d'amorçage : SE peut ne pas encore écouter MQTT quand SP envoie les premières trames. Prévoir un délai (ex: 2s) ou une mécanique de retry avec vérification des ACK $I:.**



# **16. Récapitulatif des unités**


| **Variable / module** | **Unité** |
| - | - |
| **Gyro / Mag  freqCont, freqMoy** | **Hz — plus grand = plus fréquent** |
| **Sys  freqCont, freqMoy** | **Secondes — plus petit = plus fréquent** |
| **MicroSE  freqCont, winSize, holdTime** | **Millisecondes — plus petit = plus fréquent** |
| **Gyro vitesses angulaires** | **rad/s × 100  (ex: 300 = 3 rad/s)** |
| **Gyro / Mag  seuilMesure, seuilAlerte** | **rad/s × 100 ou µT × 100** |
| **Mag champ brut** | **µT × 100** |
| **Gyro / Mag  caps et angles** | **deg × 10  (ex: 1250 = 125.0°)** |
| **Sys cpu, mem, storage, batt** | **% (0–100)  — batt SENS INVERSÉ : alerte si \< seuil** |
| **Sys thermal** | **°C** |
| **Sys uptime** | **Secondes depuis le boot** |
| **Mic RMS, seuils** | **RMS × 10  (0–1000)** |
| **Cam urgDelay** | **Secondes avant URG si perte cam** |
| **angleVSEBase** | **deg × 10  (angle de montage smartphone)** |

