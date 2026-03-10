#!/bin/bash
# =============================================================================
# SCRIPT  : courant_init.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/config/courant_init.sh
#
# OBJECTIF
# --------
# Simule en développement l'envoi de configuration par SP.
# Génère courant_init.json à partir des variables définies ci-dessous.
# En production, ce fichier JSON est reçu directement depuis SP via MQTT.
#
# UTILISATION
# -----------
# Ce fichier est LA SEULE SOURCE à modifier manuellement en développement.
# Après modification, enchaîner :
#   1. bash courant_init.sh    → génère courant_init.json
#   2. bash check_config.sh    → valide et génère les *_runtime.json / *_config.json
#   3. bash rz_sys_monitor.sh  (ou autre manager pour test)
#
# MODULES COUVERTS
# ----------------
#   [x] Gyro  : modeGyro, angleVSEBase, paraGyro
#   [x] Mag   : modeMg, paraMg
#   [x] Sys   : modeSys, paraSys (5 métriques dont batt sens inversé)
#   [x] Cam   : instances front/rear, profils CPU normal/optimized
#   [x] Mic   : modeMicro, paraMicro (seuils, balayage orientation)
#   [x] Appli : états initiaux, tâches Tasker, packages Android
#   [x] STT   : modeSTT, keyphrase, threshold, lib_path
#   [x] Mtr   : speed_cruise, scales, kturn  (lu directement par rz_stt_handler.sh)
#
# ARTICULATION
# ------------
#   courant_init.sh → courant_init.json → check_config.sh → *_runtime.json
#                                                          → *_config.json
#
# NOTE BLOC MTR
# -------------
# Le bloc Mtr est inclus dans courant_init.json comme référence des valeurs
# par défaut. Il N'est PAS distribué en fichier local séparé par check_config.sh.
# rz_stt_handler.sh le lit directement depuis courant_init.json pour construire
# les trames VPIV dynamiques (commandes vocales de type PLAN).
#
# NOTE BLOCS LIB_POCKETSPHINX
# ----------------------------
# model-fr/ et lexique_referent.dict sont des fichiers LOURDS (50-200 Mo),
# exclus du dépôt Git. À installer manuellement une seule fois sur le smartphone.
# fr.dict est léger et regénérable via rz_build_dict.py.
# original_init.sh vérifie leur présence sans jamais les écraser.
#
# NOTE UNITÉS FRÉQUENCES
# -----------------------
# Gyro/Mag : freqCont/freqMoy en Hz       (plus grand = plus fréquent)
# Sys      : freqCont/freqMoy en secondes (plus petit  = plus fréquent)
# Mic      : freqCont/winSize  en ms      (plus petit  = plus fréquent)
#
# NOTE SEUILS BATTERIE (sens inversé)
# ------------------------------------
# Contrairement aux autres métriques (cpu, thermal, storage, mem) où
# l'alerte se déclenche si la valeur est TROP HAUTE (> seuil) :
#   batt : alerte si valeur TROP BASSE (< seuil)
#   Règle : sys_urg_batt < sys_warn_batt  (ex: urg=15% < warn=30%)
#
# DÉPENDANCES
# -----------
#   Aucune (génération texte pur)
#
# AUTEUR  : Vincent Philippe
# VERSION : 5.1  (ajout blocs STT et Mtr)
# DATE    : 2026-03-06
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
fi

OUTPUT_JSON="$BASE_DIR/config/courant_init.json"

# =============================================================================
# SECTION GYRO — Propriétés SP → SE  (Table A : module Gyro)
# =============================================================================

# --- modeGyro ---
# OFF      : capteur inactif, aucun flux
# DATACont : flux brut continu à freqCont Hz
# DATAMoy  : flux moyenné à freqMoy Hz
# ALERTE   : surveillance seuils sans flux
modeGyro="DATACont"

# --- angleVSEBase (deg×10) ---
# Angle combiné BASE+THB pour correction tangage (mode ALERTE)
# Exemple : 150 = 15.0° | 0 = machine parfaitement verticale
angleVSEBase=150

freqCont_gyro=20        # Hz — flux continu (> 0)
freqMoy_gyro=2          # Hz — flux moyenné (> 0)
nbValPourMoy_gyro=5     # mesures avant calcul moyenne (> 0)

# seuilMesure {x,y,z} (rad/s×100, >= 0) — filtrage bruit
seuilMesureX_gyro=10
seuilMesureY_gyro=10
seuilMesureZ_gyro=5

# seuilAlerte {x,y} (rad/s×100, > 0) — seuils alerte chute
seuilAlerteX_gyro=300
seuilAlerteY_gyro=300

# =============================================================================
# SECTION MAG — Propriétés SP → SE  (Table A : module Mag)
# =============================================================================

# --- modeMg ---
# OFF         : capteur inactif
# DATABrutCont: flux brut [x,y,z] µT×100 continu
# DATAGeoCont : cap géographique continu (degrés×10)
# DATAGeoMoy  : cap géographique moyenné
# DATAMgCont  : cap magnétique continu (degrés×10)
# DATAMgMoy   : cap magnétique moyenné
modeMg="DATAMgCont"

freqCont_mag=5          # Hz — flux continu (> 0)
freqMoy_mag=1           # Hz — flux moyenné (> 0)
nbValPourMoy_mag=5      # mesures pour moyenne du cap (> 0)

# seuilMesure {x,y,z} (µT×100, >= 0) — filtrage bruit
seuilMesureX_mag=50
seuilMesureY_mag=50
seuilMesureZ_mag=50

# =============================================================================
# SECTION SYS — Propriétés SP → SE  (Table A : module Sys)
# =============================================================================

# --- modeSys ---
# OFF    : aucune surveillance, script terminé
# FLOW   : publication continue dataCont* à freqCont secondes
# MOY    : publication dataMoy* (moyenne nbCycles) à freqMoy secondes
# ALERTE : aucun flux, surveillance seuils uniquement
modeSys="FLOW"

# ⚠️ SECONDES (contrairement à Gyro/Mag en Hz) : plus petit = plus fréquent
sys_freqCont=10         # secondes — période flux continu (> 0)
sys_freqMoy=60          # secondes — période flux moyenné (>= freqCont)
sys_nbCycles=3          # mesures accumulées avant publication moyenne (> 0)

# Seuils SENS HAUT : alerte si valeur > seuil — contrainte : urg > warn
# warn → $I:COM:warn   | urg → $E:Urg:source:SE:URG_...#
sys_warn_cpu=80    ;  sys_urg_cpu=90      # CPU (%) — URG_CPU_CHARGE
sys_warn_thermal=70;  sys_urg_thermal=85  # Température (°C) — URG_OVERHEAT
sys_warn_storage=85;  sys_urg_storage=95  # Stockage SDCard (%) — URG_SENSOR_FAIL
sys_warn_mem=80    ;  sys_urg_mem=90      # Mémoire RAM (%) — URG_SENSOR_FAIL

# Seuils SENS BAS : alerte si valeur < seuil — ⚠️ contrainte : urg < warn
sys_warn_batt=30   ;  sys_urg_batt=15     # Batterie SE (%) — URG_LOW_BAT

# =============================================================================
# SECTION CAM — Propriétés SP → SE  (Table A : module CamSE)
# =============================================================================
# res     : résolution (low|medium|high|720p|1080p)
# fps     : fréquence images/s (> 0)
# quality : compression JPEG 0-100
# urgDelay: délai (s) avant URG si perte cam avec typePtge 0 ou 2

# Instance REAR (principale)
cam_rear_res="1080p"  ;  cam_rear_fps=30  ;  cam_rear_quality=80  ;  cam_rear_urgDelay=5

# Instance FRONT
cam_front_res="720p"  ;  cam_front_fps=15  ;  cam_front_quality=80  ;  cam_front_urgDelay=5

# Profil CPU normal (charge <= 70%)
cam_profile_normal_res="720p"  ;  cam_profile_normal_fps=30  ;  cam_profile_normal_quality=80

# Profil CPU optimized (charge > 70%) — doit être plus lent que normal
cam_profile_optimized_res="low"  ;  cam_profile_optimized_fps=10  ;  cam_profile_optimized_quality=50

# =============================================================================
# SECTION MIC — Propriétés SP → SE  (Table A : module MicroSE)
# =============================================================================
# MODES modeMicro :
#   off         : micro coupé, aucun flux
#   normal      : dataContRMS à chaque mesure (sans filtre)
#   intensite   : dataContRMS si delta > seuilDelta + alerte si seuil franchi holdTime ms
#   orientation : balayage servo TGD → direction (angle max RMS)
#   surveillance: intensite + balayage auto si RMS > seuilMoyen
#
# Fenêtre glissante : NB_FENETRE = winSize / freqCont (min 2 → winSize >= freqCont)
# Durée balayage estimée : (180/pasBalayage) × (200ms stabilisation + winSize)

mic_mode="off"              # off|normal|intensite|orientation|surveillance

mic_freqCont=300            # ms — période mesure (> 0)
mic_winSize=300             # ms — fenêtre glissante (>= freqCont)
mic_seuilDelta=30           # RMS×10 — variation min pour envoi dataContRMS
mic_holdTime=500            # ms — durée maintien seuil avant alerte

# Seuils intensité (RMS×10, 0-1000) — contrainte : faible < moyen < fort
mic_seuilFaible=100
mic_seuilMoyen=400
mic_seuilFort=700

mic_pasBalayage=10          # degrés par étape (balayage orientation)
mic_timeoutOrientation=15   # secondes — durée max balayage (sécurité)

# =============================================================================
# SECTION APPLI — Propriétés SP → SE  (Table A : module Appli, CAT=A)
# =============================================================================
# Chaque app a un état initial Off (sécurité démarrage) et un nom de tâche
# Tasker. Tasker doit être lancé avant Baby, BabyMonitor, NavGPS.

# États initiaux (On|Off) — recommandé : toujours Off au démarrage
appli_Baby_state="Off"  ;  appli_tasker_state="Off"  ;  appli_zoom_state="Off"
appli_BabyMonitor_state="Off"  ;  appli_NavGPS_state="Off"

# Noms des tâches Tasker (doivent correspondre exactement dans Tasker)
appli_Baby_task="RZ_Baby"
appli_zoom_task="RZ_Zoom"
appli_BabyMonitor_task="RZ_BabyMonitor"
appli_NavGPS_task="RZ_NavGPS"

# Packages Android (pour fallback am start si Tasker bridge échoue)
appli_tasker_pkg="net.dinglisch.android.taskerm"
appli_zoom_pkg="us.zoom.videomeetings"
appli_NavGPS_pkg="com.google.android.apps.maps"

appli_bridge_timeout=5      # secondes — timeout bridge Tasker

# ExprTasker : tâches Tasker pour expressions faciales et messages info
appli_expr_task="RZ_Expression"     # Tâche pour expressions (sourire, triste...)
appli_info_task="RZ_Info"           # Tâche pour affichage message info

# =============================================================================
# SECTION STT — Propriétés SP → SE  (Table A : module STT)
# =============================================================================
# Paramètres de reconnaissance vocale PocketSphinx.
# PocketSphinx écoute en continu et ne réagit qu'au wake word (keyphrase).
#
# modeSTT   : KWS = Keyword Spotting (écoute wake word) | OFF = inactif
#
# threshold : seuil de sensibilité détection wake word (notation scientifique).
#   1e-20 = 0.00000000000000000001
#   Plus petite → plus exigeant (moins de faux déclenchements)
#   Valeur recommandée PocketSphinx : 1e-20
#
# keyphrase : mot de réveil prononcé pour activer RZ.
#   Doit être présent dans fr.dict (dictionnaire phonétique local).
#   Dans ce projet : "rz"
#
# lang      : langue de reconnaissance. Uniquement "fr" dans ce projet.
#
# lib_path  : chemin RELATIF au dossier du script STT vers les modèles.
#   Structure attendue dans lib_pocketsphinx/ :
#     model-fr/             ← modèle acoustique français (~50-200 Mo, hors Git)
#     lexique_referent.dict ← dictionnaire source français (~milliers de mots, hors Git)
#     fr.dict               ← dictionnaire réduit catalogue STT (regénérable, dans Git)
#   ⚠️ model-fr/ et lexique_referent.dict : installer manuellement une seule fois.
#      original_init.sh vérifie leur présence sans jamais les écraser.

stt_modeSTT="KWS"
stt_threshold="1e-20"
stt_keyphrase="rz"
stt_lang="fr"
stt_lib_path="lib_pocketsphinx"

# =============================================================================
# SECTION MTR — Propriétés de référence moteur  (Table A : module Mtr)
# =============================================================================
# Paramètres de référence du module moteur Arduino.
# Stockés dans courant_init.json comme source de vérité (valeurs par défaut SP).
#
# ⚠️ CE BLOC N'EST PAS distribué en fichier local séparé par check_config.sh.
#    rz_stt_handler.sh lit directement courant_init.json pour construire les
#    trames VPIV des commandes vocales dynamiques (type PLAN).
#    Cohérence : PLAN utilise les valeurs de CONFIGURATION (stables),
#    pas les valeurs runtime (vitesse instantanée inconnue de SE).
#
# speed_cruise : vitesse "normale" de déplacement (-100..100, unité Mtr).
#   Utilisée par les commandes PLAN type "angle+speed" :
#   ex "avance de 2 mètres" → trame Mtr:cmd = speed_cruise, omega, accel
#
# inputScale  : échelle des commandes en entrée
# outputScale : échelle de sortie vers le hardware moteur
#   Conversion : consigne_hw = commande × outputScale / inputScale
#
# kturn : facteur de rotation, stocké ×10 pour éviter les float en Bash.
#   Côté Arduino : kturn_reel = kturn / 10  (ex: 8 → 0.8)
#   Formule : L = (v - omega×0.8)  |  R = (v + omega×0.8)
#
# active : état moteur au démarrage. Toujours false (sécurité).

mtr_speed_cruise=30
mtr_inputScale=100
mtr_outputScale=400
mtr_kturn=8             # ×10 → 0.8 côté Arduino
mtr_active=false

# =============================================================================
# GÉNÉRATION DE courant_init.json
# =============================================================================

echo "Génération de $OUTPUT_JSON ..."
mkdir -p "$(dirname "$OUTPUT_JSON")"

cat > "$OUTPUT_JSON" <<EOF
{
  "gyro": {
    "modeGyro": "$modeGyro",
    "angleVSEBase": $angleVSEBase,
    "paraGyro": {
      "freqCont": $freqCont_gyro,
      "freqMoy": $freqMoy_gyro,
      "nbValPourMoy": $nbValPourMoy_gyro,
      "seuilMesure": {
        "x": $seuilMesureX_gyro,
        "y": $seuilMesureY_gyro,
        "z": $seuilMesureZ_gyro
      },
      "seuilAlerte": {
        "x": $seuilAlerteX_gyro,
        "y": $seuilAlerteY_gyro
      }
    }
  },
  "mag": {
    "modeMg": "$modeMg",
    "paraMg": {
      "freqCont": $freqCont_mag,
      "freqMoy": $freqMoy_mag,
      "nbValPourMoy": $nbValPourMoy_mag,
      "seuilMesure": {
        "x": $seuilMesureX_mag,
        "y": $seuilMesureY_mag,
        "z": $seuilMesureZ_mag
      }
    }
  },
  "sys": {
    "modeSys": "$modeSys",
    "paraSys": {
      "freqCont": $sys_freqCont,
      "freqMoy":  $sys_freqMoy,
      "nbCycles": $sys_nbCycles,
      "thresholds": {
        "cpu":     { "warn": $sys_warn_cpu,     "urg": $sys_urg_cpu     },
        "thermal": { "warn": $sys_warn_thermal, "urg": $sys_urg_thermal },
        "storage": { "warn": $sys_warn_storage, "urg": $sys_urg_storage },
        "mem":     { "warn": $sys_warn_mem,     "urg": $sys_urg_mem     },
        "batt":    { "warn": $sys_warn_batt,    "urg": $sys_urg_batt    }
      }
    }
  },
  "cam": {
    "front": {
      "mode": "off",
      "res": "$cam_front_res",
      "fps": $cam_front_fps,
      "quality": $cam_front_quality,
      "urgDelay": $cam_front_urgDelay,
      "streamURL": ""
    },
    "rear": {
      "mode": "off",
      "res": "$cam_rear_res",
      "fps": $cam_rear_fps,
      "quality": $cam_rear_quality,
      "urgDelay": $cam_rear_urgDelay,
      "streamURL": ""
    },
    "status": {
      "current_side": "rear",
      "last_ip": "0.0.0.0"
    },
    "profiles": {
      "normal":    { "res": "$cam_profile_normal_res",    "fps": $cam_profile_normal_fps,    "quality": $cam_profile_normal_quality    },
      "optimized": { "res": "$cam_profile_optimized_res", "fps": $cam_profile_optimized_fps, "quality": $cam_profile_optimized_quality }
    },
    "locks": {
      "zoom_active": false,
      "security_app_active": false
    }
  },
  "appli": {
    "Baby":        { "state": "$appli_Baby_state",        "tasker_task": "$appli_Baby_task",        "package": "",                    "last_change": "" },
    "tasker":      { "state": "$appli_tasker_state",      "tasker_task": "",                        "package": "$appli_tasker_pkg",   "last_change": "" },
    "zoom":        { "state": "$appli_zoom_state",        "tasker_task": "$appli_zoom_task",        "package": "$appli_zoom_pkg",     "last_change": "" },
    "BabyMonitor": { "state": "$appli_BabyMonitor_state", "tasker_task": "$appli_BabyMonitor_task", "package": "",                    "last_change": "" },
    "NavGPS":      { "state": "$appli_NavGPS_state",      "tasker_task": "$appli_NavGPS_task",      "package": "$appli_NavGPS_pkg",   "last_change": "" },
    "bridge_timeout": $appli_bridge_timeout,
    "ExprTasker": { "expr_task": "$appli_expr_task", "info_task": "$appli_info_task" }
  },
  "mic": {
    "modeMicro": "$mic_mode",
    "paraMicro": {
      "freqCont": $mic_freqCont,
      "winSize": $mic_winSize,
      "seuilDelta": $mic_seuilDelta,
      "holdTime": $mic_holdTime,
      "seuilFaible": $mic_seuilFaible,
      "seuilMoyen": $mic_seuilMoyen,
      "seuilFort": $mic_seuilFort,
      "pasBalayage": $mic_pasBalayage,
      "timeoutOrientation": $mic_timeoutOrientation
    }
  },
  "stt": {
    "modeSTT":   "$stt_modeSTT",
    "threshold": $stt_threshold,
    "keyphrase": "$stt_keyphrase",
    "lang":      "$stt_lang",
    "lib_path":  "$stt_lib_path"
  },
  "mtr": {
    "speed_cruise":  $mtr_speed_cruise,
    "inputScale":    $mtr_inputScale,
    "outputScale":   $mtr_outputScale,
    "kturn":         $mtr_kturn,
    "active":        $mtr_active
  }
}
EOF

echo "courant_init.json généré  ✓"
echo "Étape suivante : bash check_config.sh"