#!/bin/bash
# =============================================================================
# SCRIPT  : original_init.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/config/original_init.sh
#
# OBJECTIF
# --------
# Script d'initialisation PRODUCTION du SE (Smartphone Embarqué).
# À exécuter UNE SEULE FOIS lors de l'installation sur le smartphone,
# ou après une mise à jour majeure de la configuration ou du catalogue STT.
#
# Ce script est l'équivalent de courant_init.sh pour la production.
# Avantage du format .sh sur le JSON : il permet d'intégrer des commentaires
# détaillés sur chaque paramètre, facilitant les évolutions futures.
#
# CE QUE FAIT CE SCRIPT
# ----------------------
#   1. Vérifie la présence des fichiers PocketSphinx (model-fr/, dicts)
#      → Ne les crée ni ne les écrase JAMAIS.
#   2. Génère courant_init.json (configuration complète SE + Arduino)
#   3. Génère table_d_catalogue.csv (catalogue des commandes vocales STT)
#   4. Lance rz_build_system.py → stt_catalog.json + rz_words.txt
#   5. Lance rz_build_dict.py   → fr.dict (dictionnaire PocketSphinx)
#   6. Lance check_config.sh    → *_runtime.json + *_config.json
#
# DIFFÉRENCES AVEC courant_init.sh
# ----------------------------------
#   courant_init.sh : script de développement PC, valeurs de test.
#   original_init.sh : script de production smartphone, valeurs terrain.
#   original_init.sh génère EN PLUS le catalogue STT (table_d_catalogue.csv).
#   En production, original_init.sh remplace courant_init.sh.
#
# MODULES COUVERTS (courant_init.json)
# --------------------------------------
#   [x] Gyro  : modeGyro, angleVSEBase, paraGyro
#   [x] Mag   : modeMg, paraMg
#   [x] Sys   : modeSys, paraSys (5 métriques dont batt sens inversé)
#   [x] Cam   : instances front/rear, profils CPU normal/optimized
#   [x] Mic   : modeMicro, paraMicro (seuils, balayage orientation)
#   [x] Appli : états initiaux, tâches Tasker, packages Android
#   [x] STT   : modeSTT, keyphrase, threshold, lib_path
#   [x] Voice : vol (volume multimédia global), output (sortie audio), ttsRate
#   [x] Mtr   : speed_cruise, scales, kturn
#
# CATALOGUE STT (table_d_catalogue.csv)
# ----------------------------------------
# Section clairement délimitée, facile à repérer pour les mises à jour.
# ⚠️ CHERCHER LA BALISE : === SECTION CATALOGUE STT ===
# Contient toutes les commandes vocales reconnues par RZ.
# Structure CSV : ID;Traitement;CAT;Destinataire;moteurActif;Alias;ID_fils;
#                 VAR;PROP;INSTANCE;VALEUR;Prepa_Encodage;Alias-TTS
#
# ARTICULATION
# ------------
#   original_init.sh
#     → courant_init.json       → check_config.sh → *_runtime.json / *_config.json
#     → table_d_catalogue.csv   → rz_build_system.py → stt_catalog.json + rz_words.txt
#                                → rz_build_dict.py   → fr.dict
#
# PRÉCAUTIONS
# -----------
# - Vérifier que python3 est installé (pkg install python) avant lancement.
# - Vérifier que model-fr/ et lexique_referent.dict sont installés manuellement
#   dans scripts/sensors/stt/lib_pocketsphinx/ avant lancement.
# - Ce script ÉCRASE courant_init.json et table_d_catalogue.csv.
# - Les fichiers PocketSphinx lourds (model-fr/, lexique_referent.dict) ne sont
#   jamais écrasés — ce script les vérifie seulement.
#
# NOTE UNITÉS FRÉQUENCES
# -----------------------
# Gyro/Mag : freqCont/freqMoy en Hz       (plus grand = plus fréquent)
# Sys      : freqCont/freqMoy en secondes (plus petit  = plus fréquent)
# Mic      : freqCont/winSize  en ms      (plus petit  = plus fréquent)
#
# NOTE SEUILS BATTERIE (sens inversé)
# ------------------------------------
# batt : alerte si valeur TROP BASSE (< seuil)
# Règle : sys_urg_batt < sys_warn_batt  (ex: urg=15% < warn=30%)
#
# DÉPENDANCES
# -----------
#   jq     : manipulation JSON   (pkg install jq)
#   python3: scripts build STT   (pkg install python)
#   check_config.sh : validation et distribution des configs
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0  (intégration courant_init + catalogue STT)
# DATE    : 2026-03-06
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
fi

OUTPUT_JSON="$BASE_DIR/config/courant_init.json"
GLOBAL_JSON="$BASE_DIR/config/global.json"
STT_SCRIPT_DIR="$BASE_DIR/scripts/sensors/stt"
LOG_FILE="$BASE_DIR/logs/original_init.log"

# =============================================================================
# CRÉATION DE L'ARBORESCENCE COMPLÈTE
# =============================================================================
# Tous les répertoires nécessaires sont créés ici une seule fois,
# avant toute génération de fichier. Le script est ainsi totalement
# autonome, même sur une installation vierge.
#
# Structure créée :
#   termux/
#   ├── config/
#   │   └── sensors/          ← cam_config.json, mic_config.json, stt_config.json
#   ├── logs/
#   └── scripts/
#       └── sensors/
#           └── stt/          ← table_d_catalogue.csv, stt_catalog.json, rz_words.txt
#               └── lib_pocketsphinx/   ← model-fr/, lexique_referent.dict (manuels)
# =============================================================================

mkdir -p "$BASE_DIR/config/sensors"
mkdir -p "$BASE_DIR/logs"
mkdir -p "$BASE_DIR/scripts/sensors/stt/lib_pocketsphinx"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INIT] $1" | tee -a "$LOG_FILE"
}

erreur() {
    log "ERREUR FATALE : $1"
    log "Script interrompu."
    exit 1
}

log "=========================================="
log "Démarrage original_init.sh  v1.0"
log "=========================================="

# =============================================================================
# ÉTAPE 0 — VÉRIFICATIONS PRÉALABLES
# =============================================================================

log "--- Vérifications préalables ---"

command -v jq     &>/dev/null || erreur "jq non installé (pkg install jq)"
command -v python3 &>/dev/null || erreur "python3 non installé (pkg install python)"

log "  jq et python3 disponibles  ✓"

# =============================================================================
# ÉTAPE 1 — VÉRIFICATION FICHIERS POCKETSPHINX
# =============================================================================
# ⚠️ Ces fichiers sont LOURDS (~50-200 Mo), exclus du dépôt Git.
#    Ils doivent être installés manuellement UNE SEULE FOIS sur le smartphone.
#    Ce script les vérifie sans jamais les créer ni les écraser.
#
# Structure attendue dans lib_pocketsphinx/ :
#   model-fr/              ← modèle acoustique français (binaires PocketSphinx)
#   lexique_referent.dict  ← dictionnaire source (milliers de mots français)
#   fr.dict                ← dictionnaire réduit (généré par rz_build_dict.py)
# =============================================================================

log "--- Vérification fichiers PocketSphinx ---"

# lib_path est lu depuis les variables STT définies plus bas.
# Pour la vérification préalable, on utilise la valeur par défaut.
STT_LIB_CHECK="$STT_SCRIPT_DIR/lib_pocketsphinx"
stt_prereq_ok=true

if [ ! -d "$STT_LIB_CHECK/model-fr" ]; then
    log "  WARN : model-fr/ absent dans $STT_LIB_CHECK"
    log "         → STT inopérant après installation."
    log "         → Copier manuellement le modèle acoustique français."
    stt_prereq_ok=false
else
    log "  model-fr/ présent  ✓  (non modifié)"
fi

if [ ! -f "$STT_LIB_CHECK/lexique_referent.dict" ]; then
    log "  WARN : lexique_referent.dict absent dans $STT_LIB_CHECK"
    log "         → rz_build_dict.py ne pourra pas générer fr.dict."
    log "         → Copier manuellement le dictionnaire source français."
    stt_prereq_ok=false
else
    log "  lexique_referent.dict présent  ✓  (non modifié)"
fi

$stt_prereq_ok || log "  WARN : Installation STT incomplète — continuer quand même."

# =============================================================================
# ÉTAPE 2 — PARAMÈTRES DE CONFIGURATION
# =============================================================================

# ---------------------------------------------------------------------------
# SECTION GYRO — Propriétés SP → SE  (Table A : module Gyro)
# ---------------------------------------------------------------------------
# OFF      : capteur inactif, aucun flux
# DATACont : flux brut continu à freqCont Hz
# DATAMoy  : flux moyenné à freqMoy Hz
# ALERTE   : surveillance seuils sans flux
modeGyro="DATACont"

# angleVSEBase (deg×10) : angle combiné BASE+THB pour correction tangage
# 150 = 15.0° | 0 = machine parfaitement verticale
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

# ---------------------------------------------------------------------------
# SECTION MAG — Propriétés SP → SE  (Table A : module Mag)
# ---------------------------------------------------------------------------
# OFF / DATABrutCont / DATAGeoCont / DATAGeoMoy / DATAMgCont / DATAMgMoy
modeMg="DATAMgCont"

freqCont_mag=5          # Hz — flux continu (> 0)
freqMoy_mag=1           # Hz — flux moyenné (> 0)
nbValPourMoy_mag=5      # mesures pour moyenne du cap (> 0)

# seuilMesure {x,y,z} (µT×100, >= 0) — filtrage bruit
seuilMesureX_mag=50
seuilMesureY_mag=50
seuilMesureZ_mag=50

# ---------------------------------------------------------------------------
# SECTION SYS — Propriétés SP → SE  (Table A : module Sys)
# ---------------------------------------------------------------------------
# OFF / FLOW / MOY / ALERTE
modeSys="FLOW"

# ⚠️ SECONDES (pas des Hz) — plus petit = plus fréquent
sys_freqCont=10         # secondes — période flux continu (> 0)
sys_freqMoy=60          # secondes — période flux moyenné (>= freqCont)
sys_nbCycles=3          # mesures accumulées avant publication moyenne (> 0)

# Seuils SENS HAUT : alerte si valeur > seuil — contrainte : urg > warn
sys_warn_cpu=80    ;  sys_urg_cpu=90      # CPU (%) — URG_CPU_CHARGE
sys_warn_thermal=70;  sys_urg_thermal=85  # Température (°C) — URG_OVERHEAT
sys_warn_storage=85;  sys_urg_storage=95  # Stockage SDCard (%) — URG_SENSOR_FAIL
sys_warn_mem=80    ;  sys_urg_mem=90      # Mémoire RAM (%) — URG_SENSOR_FAIL

# Seuils SENS BAS : alerte si valeur < seuil — ⚠️ contrainte : urg < warn
sys_warn_batt=30   ;  sys_urg_batt=15    # Batterie SE (%) — URG_LOW_BAT

# ---------------------------------------------------------------------------
# SECTION CAM — Propriétés SP → SE  (Table A : module CamSE)
# ---------------------------------------------------------------------------
# res : low|medium|high|720p|1080p
# urgDelay : délai (s) avant URG si perte cam avec typePtge 0 ou 2

cam_rear_res="1080p"  ;  cam_rear_fps=30  ;  cam_rear_quality=80  ;  cam_rear_urgDelay=5
cam_front_res="720p"  ;  cam_front_fps=15  ;  cam_front_quality=80  ;  cam_front_urgDelay=5

# Profil CPU normal (charge <= 70%)
cam_profile_normal_res="720p"  ;  cam_profile_normal_fps=30  ;  cam_profile_normal_quality=80
# Profil CPU optimized (charge > 70%) — doit être plus lent que normal
cam_profile_optimized_res="low"  ;  cam_profile_optimized_fps=10  ;  cam_profile_optimized_quality=50

# ---------------------------------------------------------------------------
# SECTION MIC — Propriétés SP → SE  (Table A : module MicroSE)
# ---------------------------------------------------------------------------
# MODES : off | normal | intensite | orientation | surveillance
# Fenêtre : winSize >= freqCont (NB_FENETRE = winSize/freqCont, min 2)
# Durée balayage estimée : (180/pasBalayage + 1) × (200ms + winSize)

mic_mode="off"

mic_freqCont=300            # ms — période mesure (> 0)
mic_winSize=300             # ms — fenêtre glissante (>= freqCont)
mic_seuilDelta=30           # RMS×10 — variation min pour envoi dataContRMS
mic_holdTime=500            # ms — durée maintien seuil avant alerte

# Seuils intensité (RMS×10, 0-1000) — contrainte : faible < moyen < fort
mic_seuilFaible=100
mic_seuilMoyen=400
mic_seuilFort=700

mic_pasBalayage=10          # degrés par étape (balayage orientation)
mic_timeoutOrientation=15   # secondes — durée max balayage

# ---------------------------------------------------------------------------
# SECTION APPLI — Propriétés SP → SE  (Table A : module Appli, CAT=A)
# ---------------------------------------------------------------------------
# États toujours Off au démarrage (sécurité)

appli_Baby_state="Off"  ;  appli_tasker_state="Off"  ;  appli_zoom_state="Off"
appli_BabyMonitor_state="Off"  ;  appli_NavGPS_state="Off"

# Noms des tâches Tasker (doivent correspondre exactement dans Tasker)
appli_Baby_task="RZ_Baby"
appli_zoom_task="RZ_Zoom"
appli_BabyMonitor_task="RZ_BabyMonitor"
appli_NavGPS_task="RZ_NavGPS"

# Packages Android (pour fallback am start)
appli_tasker_pkg="net.dinglisch.android.taskerm"
appli_zoom_pkg="us.zoom.videomeetings"
appli_NavGPS_pkg="com.google.android.apps.maps"

appli_bridge_timeout=5

# ExprTasker : tâches expressions et messages info
appli_expr_task="RZ_Expression"
appli_info_task="RZ_Info"

# ---------------------------------------------------------------------------
# SECTION STT — Propriétés SP → SE  (Table A : module STT)
# ---------------------------------------------------------------------------
# modeSTT   : KWS = Keyword Spotting | OFF = inactif
# threshold : seuil PocketSphinx (notation scientifique : 1e-20 recommandé)
#             Plus petit → plus exigeant (moins de faux déclenchements)
# keyphrase : mot de réveil (doit être dans fr.dict)
# lang      : langue de reconnaissance (uniquement "fr" dans ce projet)
# lib_path  : chemin RELATIF au dossier stt/ vers les modèles PocketSphinx

stt_modeSTT="KWS"
stt_threshold="1e-20"
stt_keyphrase="rz"
stt_lang="fr"
stt_lib_path="lib_pocketsphinx"

# ---------------------------------------------------------------------------
# SECTION VOICE — Propriétés SP → SE  (Table A : module Voice)
# ---------------------------------------------------------------------------
# vol    : volume multimédia global SE en % (canal music Android)
#          ⚠ INDÉPENDANT de modeSTT — actif même si STT est coupé.
#          Contrôle toute la sortie audio : TTS, musique, baffe jack/BT.
#          Normalisé 0-100 → plage Android native par rz_voice_manager.sh.
# output : sortie audio active (détectée automatiquement par Android)
#          internal = haut-parleur SE | jack = sortie casque | bt = Bluetooth
#          La commutation BT forcée est en V2 (Tasker requis).
# ttsRate: vitesse de synthèse TTS (1.0 = normale, >1 = plus rapide)

voice_vol=80            # % volume initial (0-100)
voice_output="internal" # sortie par défaut : internal | jack | bt
voice_ttsRate=1.0       # vitesse de synthèse TTS (1.0 = normale, >1 = plus rapide)

# ---------------------------------------------------------------------------
# SECTION MTR — Paramètres de référence moteur  (Table A : module Mtr)
# ---------------------------------------------------------------------------
# Lu directement depuis courant_init.json par rz_stt_handler.sh pour les
# commandes vocales de type PLAN (valeurs de configuration, pas runtime).
#
# kturn : ×10 pour éviter les float Bash → 8 = 0.8 côté Arduino
#   Formule : L = (v - omega×0.8) | R = (v + omega×0.8)
# active : toujours false au démarrage (sécurité)

mtr_speed_cruise=30
mtr_inputScale=100
mtr_outputScale=400
mtr_kturn=8             # ×10 → 0.8 côté Arduino
mtr_active=false

# =============================================================================
# ÉTAPE 3 — GÉNÉRATION DE courant_init.json
# =============================================================================

log "--- Génération courant_init.json ---"
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
    "voice": {
    "vol":     $voice_vol,
    "output":  "$voice_output",
    "ttsRate": $voice_ttsRate
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

if ! jq '.' "$OUTPUT_JSON" > /dev/null 2>&1; then
    erreur "courant_init.json généré est invalide (vérifier les variables)"
fi
log "courant_init.json généré et validé  ✓"

# =============================================================================
# ÉTAPE 4 — GÉNÉRATION DE global.json
# =============================================================================
# global.json contient l'état dynamique du SE, partagé entre tous les managers.
# Il est initialisé ici avec des valeurs de départ sûres, puis mis à jour
# au runtime par rz_sys_monitor.sh (bloc Sys) et rz_state-manager.sh (bloc CfgS/STT).
#
# Les 4 blocs présents :
#
#   CfgS : configuration système (modes de fonctionnement)
#     modeRZ    : mode global robot — 1 = VEILLE au démarrage (sécurité)
#       0=ARRET | 1=VEILLE | 2=FIXE | 3=DEPLACEMENT | 4=AUTONOME | 5=ERREUR
#     typePtge  : type de pilotage — 0 = Écran au démarrage
#       0=Ecran | 1=Vocal | 2=Manette | 3=Laisse | 4=Laisse+Vocal
#     reset     : drapeau de réinitialisation (false = pas de reset demandé)
#
#   Urg : état du système d'urgence
#     source  : code de l'urgence active (URG_NONE = aucune urgence)
#     statut  : cleared = pas d'urgence active
#     niveau  : warn | danger (niveau en cours si urgence active)
#
#   COM : canal de messages informatifs entre modules
#     Les 4 niveaux debug/error/info/warn — vides au démarrage
#     Remplis par les managers puis publiés vers SP via VPIV
#
#   Sys : métriques système temps réel
#     Initialisées à 0 — rz_sys_monitor.sh les met à jour en continu
#     cpu_load (%), temp (°C), storage (%), mem (%), batt (%)
#     uptime (secondes depuis boot), last_update (horodatage dernière mesure)
#
# ⚠️ Ne jamais modifier global.json manuellement pendant l'exécution.
#    Ce fichier est la mémoire partagée du SE — accès concurrent possible.
# =============================================================================

log "--- Génération global.json ---"

cat > "$GLOBAL_JSON" <<EOF
{
  "CfgS": {
    "modeRZ":   1,
    "typePtge": 0,
    "reset":    false
  },
  "Urg": {
    "source": "URG_NONE",
    "statut": "cleared",
    "niveau": "warn"
  },
  "COM": {
    "debug": "",
    "error": "",
    "info":  "",
    "warn":  ""
  },
  "Sys": {
    "cpu_load":   0,
    "temp":       0,
    "storage":    0,
    "mem":        0,
    "batt":       0,
    "uptime":     0,
    "last_update": ""
  }
}
EOF

if ! jq '.' "$GLOBAL_JSON" > /dev/null 2>&1; then
    erreur "global.json généré est invalide"
fi
log "global.json généré et validé  ✓"


# #############################################################################
# #############################################################################
# ##                                                                         ##
# ##           === SECTION CATALOGUE STT — TABLE D ===                      ##
# ##                                                                         ##
# ##  POINT D'ENTRÉE POUR LES MISES À JOUR DU CATALOGUE VOCAL               ##
# ##                                                                         ##
# ##  Format de chaque ligne CSV :                                           ##
# ##  ID ; Traitement ; CAT ; Destinataire ; moteurActif ; Alias ;           ##
# ##  ID_fils ; VAR ; PROP ; INSTANCE ; VALEUR ; Prepa_Encodage ; Alias-TTS ##
# ##                                                                         ##
# ##  Traitement : SIMPLE | COMPLEXE | PLAN | LOCAL                          ##
# ##  CAT        : V (Consigne) | I (Info) | A (Application)                 ##
# ##  Destinataire : A (Arduino) | SE (Smartphone) | SP (Superviseur)        ##
# ##  moteurActif  : 1 = commande moteur (bloquée en mode laisse)            ##
# ##  Alias      : synonymes séparés par virgule (reconnus par PocketSphinx) ##
# ##  ID_fils    : pour COMPLEXE uniquement, IDs séparés par virgule         ##
# ##  Prepa_Encodage : pour PLAN uniquement :                                ##
# ##    NUM       → injecter le nombre extrait de la phrase                  ##
# ##    NUM_DIR   → ±nombre selon gauche/droite/haut/bas                     ##
# ##    angle+speed → nombre + speed_cruise depuis courant_init.json         ##
# ##  Alias-TTS  : réponse vocale du robot, variantes séparées par |         ##
# ##                                                                         ##
# ##  RÈGLES DE MISE À JOUR :                                                ##
# ##  - Ajouter une ligne dans le groupe correspondant                       ##
# ##  - Les lignes de commentaire commencent par ;;;;                        ##
# ##  - Après modification : relancer ce script complet                      ##
# ##    (ou lancer rz_build_system.py + rz_build_dict.py manuellement)       ##
# ##                                                                         ##
# #############################################################################
# #############################################################################

log "--- Génération table_d_catalogue.csv ---"

CATALOGUE_CSV="$STT_SCRIPT_DIR/table_d_catalogue.csv"
mkdir -p "$STT_SCRIPT_DIR"

cat > "$CATALOGUE_CSV" << 'CATALOGUE_EOF'
ID;Traitement;CAT;Destinataire;moteurActif;Alias;ID_fils;VAR;PROP;INSTANCE;VALEUR;Prepa_Encodage;Alias-TTS
;;;;;;;;;;;; URGENCE — priorité absolue (toujours traité même modeRZ=0) ;;;;
URGENCY_STOP;SIMPLE;V;A;0;stop,arrête tout,urgence,freine;;Mtr;cmd;*;0,0,0;;Je m'arrête.|Arrêt d'urgence.
;;;;;;;;;;;; MOTEUR — déplacements simples ;;;;
MTR_FORWARD;SIMPLE;V;A;1;avance,tout droit,vas-y,en avant;;Mtr;cmd;*;50,0,2;;Je fonce.|C'est parti.
MTR_BACKWARD;SIMPLE;V;A;1;recule,en arrière,marche arrière;;Mtr;cmd;*;-50,0,2;;Je recule.
MTR_STOP;SIMPLE;V;A;1;stop,arrête,freine,immobile;;Mtr;cmd;*;0,0,0;;Arrêt.
MTR_SLOW;SIMPLE;V;A;1;lentement,doucement,ralentis;;Mtr;cmd;*;20,0,1;;Je ralentis.
MTR_FAST;SIMPLE;V;A;1;vite,accélère,plus vite,rapide;;Mtr;cmd;*;80,0,3;;À toute vitesse !
;;;;;;;;;;;; MOTEUR — déplacements dynamiques (PLAN) ;;;;
MTR_TURN_RIGHT;PLAN;V;A;1;tourne à droite,tourne droite,virage droite;;Mtr;cmd;*;;NUM_DIR;Je tourne à droite.
MTR_TURN_LEFT;PLAN;V;A;1;tourne à gauche,tourne gauche,virage gauche;;Mtr;cmd;*;;NUM_DIR;Je tourne à gauche.
MTR_TURN_ANGLE;PLAN;V;A;1;tourne de,tourne à;;Mtr;cmd;*;;NUM_DIR;Je tourne.
MTR_SPEED_SET;PLAN;V;A;1;vitesse,à la vitesse de;;Mtr;cmd;*;;NUM;Je règle ma vitesse.
MTR_ADVANCE_DIST;PLAN;V;A;1;avance de,avance jusqu'à;;Mtr;cmd;*;;angle+speed;Je me déplace.
;;;;;;;;;;;; CONFIGURATION — modes globaux (modeRZ) ;;;;
CFG_VEILLE;SIMPLE;V;SE;0;mode veille,veille,repos,mise en veille;;CfgS;modeRZ;*;1;;Mode veille activé.
CFG_FIXE;SIMPLE;V;SE;0;mode fixe,fixe,position fixe;;CfgS;modeRZ;*;2;;Mode fixe activé.
CFG_DEPLACEMENT;SIMPLE;V;SE;0;mode déplacement,déplacement;;CfgS;modeRZ;*;3;;Prêt à me déplacer.
CFG_AUTONOME;SIMPLE;V;SE;0;mode autonome,autonome,auto;;CfgS;modeRZ;*;4;;Mode autonome activé.
;;;;;;;;;;;; CONFIGURATION — type de pilotage (typePtge) ;;;;
CFG_PTGE_VOCAL;SIMPLE;V;SE;0;pilotage vocal,commande vocale;;CfgS;typePtge;*;1;;Pilotage vocal activé.
CFG_PTGE_ECRAN;SIMPLE;V;SE;0;pilotage écran,écran;;CfgS;typePtge;*;0;;Pilotage écran activé.
CFG_PTGE_LAISSE;SIMPLE;V;SE;0;mode laisse,laisse;;CfgS;typePtge;*;3;;Je suis en mode laisse.
;;;;;;;;;;;; CAMÉRA ;;;;
CAM_ON;SIMPLE;A;SE;0;démarre la caméra,allume la caméra,caméra on;;CamSE;active;rear;On;;Caméra démarrée.
CAM_OFF;SIMPLE;A;SE;0;arrête la caméra,éteins la caméra,caméra off;;CamSE;active;*;Off;;Caméra arrêtée.
CAM_FRONT;SIMPLE;A;SE;0;caméra avant,vue de face;;CamSE;mode;*;front;;Je bascule sur la caméra avant.
CAM_REAR;SIMPLE;A;SE;0;caméra arrière,vue arrière;;CamSE;mode;*;rear;;Je bascule sur la caméra arrière.
CAM_SNAP;SIMPLE;A;SE;0;prends une photo,snapshot,capture;;CamSE;(snap);*;1;;Photo prise.
;;;;;;;;;;;; LEDS — anneau (LRing) ;;;;
LRING_ON;SIMPLE;V;A;0;allume les leds,allume l'anneau;;LRing;act;*;1;;Leds allumées.
LRING_OFF;SIMPLE;V;A;0;éteins les leds,éteins l'anneau;;LRing;act;*;0;;Leds éteintes.
LRING_SMILE;SIMPLE;V;A;0;souris,fais un sourire,expression sourire;;LRing;expr;*;sourire;;Voilà mon plus beau sourire.
LRING_SAD;SIMPLE;V;A;0;triste,sois triste,expression triste;;LRing;expr;*;triste;;Je suis triste.
LRING_NEUTRAL;SIMPLE;V;A;0;neutre,expression neutre,visage neutre;;LRing;expr;*;neutre;;Expression neutre.
LRING_SURPRISED;SIMPLE;V;A;0;étonné,surpris,expression étonnement;;LRing;expr;*;étonnement;;Oh !
LRING_ANGRY;SIMPLE;V;A;0;en colère,fâché,expression colère;;LRing;expr;*;colère;;Grrr !
LRING_BRIGHTNESS;PLAN;V;A;0;intensité,luminosité;;LRing;int;*;;NUM;Je règle la luminosité.
;;;;;;;;;;;; SERVOS — orientation tête (TGD = gauche-droite, THB = haut-bas) ;;;;
SRV_HEAD_UP;SIMPLE;V;A;0;lève la tête,regarde en haut,haut;;Srv;angle;THB;120;;Je lève la tête.
SRV_HEAD_DOWN;SIMPLE;V;A;0;baisse la tête,regarde en bas,bas;;Srv;angle;THB;30;;Je baisse la tête.
SRV_HEAD_RIGHT;SIMPLE;V;A;0;regarde à droite,tourne la tête à droite;;Srv;angle;TGD;60;;Je regarde à droite.
SRV_HEAD_LEFT;SIMPLE;V;A;0;regarde à gauche,tourne la tête à gauche;;Srv;angle;TGD;0;;Je regarde à gauche.
SRV_HEAD_CENTER;SIMPLE;V;A;0;regarde devant,tout droit,centre;;Srv;angle;TGD;30;;Je regarde devant moi.
SRV_HEAD_GD_ANGLE;PLAN;V;A;0;tourne la tête de,oriente la tête de;;Srv;angle;TGD;;NUM_DIR;Je tourne la tête.
SRV_HEAD_HB_ANGLE;PLAN;V;A;0;incline la tête de,relève la tête de;;Srv;angle;THB;;NUM_DIR;J'incline la tête.
;;;;;;;;;;;; APPLICATIONS ;;;;
APP_TASKER_ON;SIMPLE;A;SE;0;lance tasker,démarre tasker;;Appli;tasker;*;On;;Tasker démarré.
APP_TASKER_OFF;SIMPLE;A;SE;0;arrête tasker,ferme tasker;;Appli;tasker;*;Off;;Tasker fermé.
APP_ZOOM_ON;SIMPLE;A;SE;0;lance zoom,démarre zoom,réunion zoom;;Appli;zoom;*;On;;Je lance Zoom.
APP_ZOOM_OFF;SIMPLE;A;SE;0;arrête zoom,ferme zoom;;Appli;zoom;*;Off;;Zoom fermé.
APP_BABY_ON;SIMPLE;A;SE;0;lance baby,surveillance bébé;;Appli;Baby;*;On;;Surveillance bébé démarrée.
APP_BABY_OFF;SIMPLE;A;SE;0;arrête baby,stop bébé;;Appli;Baby;*;Off;;Surveillance bébé arrêtée.
APP_GPS_ON;SIMPLE;A;SE;0;lance la navigation,gps,navigation;;Appli;NavGPS;*;On;;Navigation démarrée.
APP_GPS_OFF;SIMPLE;A;SE;0;arrête la navigation,stop gps;;Appli;NavGPS;*;Off;;Navigation arrêtée.
;;;;;;;;;;;; EXPRESSIONS TASKER ;;;;
EXPR_SMILE;SIMPLE;A;SE;0;fais un sourire,souris tasker;;Appli;ExprTasker;Expression;sourire;;Voilà !
EXPR_NEUTRAL;SIMPLE;A;SE;0;retour neutre,expression normale;;Appli;ExprTasker;Off;*;;D'accord.
;;;;;;;;;;;; MICRO (smartphone) ;;;;
MIC_ON;SIMPLE;V;SE;0;active le micro,allume le micro;;MicroSE;modeMicro;*;intensite;;Micro activé.
MIC_OFF;SIMPLE;V;SE;0;coupe le micro,désactive le micro;;MicroSE;modeMicro;*;off;;Micro coupé.
MIC_ORIENT;SIMPLE;V;SE;0;écoute d'où vient le son,localise le son;;MicroSE;modeMicro;*;orientation;;J'écoute d'où tu viens.
MIC_SURV;SIMPLE;V;SE;0;mode surveillance micro,surveillance sonore;;MicroSE;modeMicro;*;surveillance;;Surveillance sonore activée.
;;;;;;;;;;;; SYSTÈME — état et info ;;;;
SYS_STATUS;LOCAL;I;SE;0;quel est ton état,comment tu vas,état système;;COM;info;SE;status;;Je vais bien.|Je vérifie mon état.
SYS_BATTERY;LOCAL;I;SE;0;batterie,niveau de batterie,autonomie;;COM;info;SE;battery;;Je vérifie ma batterie.
SYS_RESET;SIMPLE;V;SE;0;réinitialise,reset;;CfgS;(reset);*;1;;Je me réinitialise.
SYS_URG_CLEAR;SIMPLE;V;SE;0;efface l'urgence,annule l'alerte;;Urg;(clear);*;1;;Urgence effacée.
;;;;;;;;;;;; MACROS — enchaînements de commandes ;;;;
MACRO_PAUSE;COMPLEXE;V;A;0;pause,fais une pause;MTR_STOP,LRING_NEUTRAL;;;;;;;Pause.
MACRO_HELLO;COMPLEXE;V;A;0;bonjour,salut,coucou;LRING_SMILE,EXPR_SMILE;;;;;;;Bonjour ! Je suis RZ.
MACRO_SLEEP;COMPLEXE;V;SE;0;dors,va dormir,mode nuit;MTR_STOP,LRING_OFF,CFG_VEILLE;;;;;;;Bonne nuit.
MACRO_WAKE;COMPLEXE;V;SE;0;réveille-toi,debout,active-toi;CFG_DEPLACEMENT,LRING_ON,LRING_SMILE;;;;;;;Me revoilà !
CATALOGUE_EOF

log "table_d_catalogue.csv généré  ✓  ($(grep -c '^[A-Z]' "$CATALOGUE_CSV") commandes)"

# #############################################################################
# ##           === FIN SECTION CATALOGUE STT ===                            ##
# #############################################################################


# =============================================================================
# ÉTAPE 5 — BUILD STT : stt_catalog.json + rz_words.txt
# =============================================================================

log "--- Build STT : rz_build_system.py ---"

cd "$STT_SCRIPT_DIR" || erreur "Impossible d'accéder à $STT_SCRIPT_DIR"

if [ ! -f "rz_build_system.py" ]; then
    erreur "rz_build_system.py introuvable dans $STT_SCRIPT_DIR"
fi

if python3 rz_build_system.py; then
    log "stt_catalog.json et rz_words.txt générés  ✓"
else
    log "WARN : rz_build_system.py a échoué — STT inopérant."
fi

# =============================================================================
# ÉTAPE 6 — BUILD DICTIONNAIRE : fr.dict
# =============================================================================

log "--- Build dictionnaire : rz_build_dict.py ---"

if [ ! -f "rz_build_dict.py" ]; then
    log "WARN : rz_build_dict.py introuvable — fr.dict non généré."
elif [ ! -f "$STT_SCRIPT_DIR/lib_pocketsphinx/lexique_referent.dict" ]; then
    log "WARN : lexique_referent.dict absent — fr.dict non généré."
else
    if python3 rz_build_dict.py; then
        log "fr.dict généré  ✓"
    else
        log "WARN : rz_build_dict.py a échoué — vérifier lexique_referent.dict"
    fi
fi

# =============================================================================
# ÉTAPE 7 — VALIDATION ET DISTRIBUTION : check_config.sh
# =============================================================================

log "--- Validation et distribution : check_config.sh ---"

CHECK_SCRIPT="$BASE_DIR/config/check_config.sh"

if [ ! -f "$CHECK_SCRIPT" ]; then
    log "WARN : check_config.sh introuvable — distributions non effectuées."
    log "       Lancer manuellement : bash config/check_config.sh"
else
    if bash "$CHECK_SCRIPT"; then
        log "check_config.sh : distributions effectuées  ✓"
    else
        log "WARN : check_config.sh a signalé des erreurs — vérifier le log."
    fi
fi

# =============================================================================
# RÉSUMÉ FINAL
# =============================================================================

log "=========================================="
log "original_init.sh terminé"
log ""
log "Arborescence créée :"
log "  → $BASE_DIR/config/sensors/"
log "  → $BASE_DIR/logs/"
log "  → $BASE_DIR/scripts/sensors/stt/lib_pocketsphinx/"
log ""
log "Fichiers générés directement :"
log "  → $OUTPUT_JSON"
log "  → $GLOBAL_JSON"
log "  → $CATALOGUE_CSV"
log "  → $STT_SCRIPT_DIR/stt_catalog.json"
log "  → $STT_SCRIPT_DIR/rz_words.txt"
log "  → $STT_SCRIPT_DIR/lib_pocketsphinx/fr.dict"
log ""
log "Distributions via check_config.sh :"
log "  → config/gyro_runtime.json"
log "  → config/mag_runtime.json"
log "  → config/sys_runtime.json"
log "  → config/sensors/cam_config.json"
log "  → config/sensors/mic_config.json"
log "  → config/sensors/stt_config.json"
log "  → config/sensors/voice_config.json"
log "  → config/appli_config.json"
log "=========================================="