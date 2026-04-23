#!/bin/bash
# =============================================================================
# SCRIPT  : check_config.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/config/check_config.sh
#
# OBJECTIF
# --------
# Script de bootstrap lancé au démarrage du SE, avant les managers de capteurs.
# Lit courant_init.json, vérifie la cohérence de chaque section, puis génère
# les fichiers de configuration opérationnels pour chaque module.
#
# FICHIERS PRODUITS
# -----------------
#   config/gyro_runtime.json          : paramètres Gyro validés
#   config/mag_runtime.json           : paramètres Mag validés
#   config/sys_runtime.json           : paramètres Sys validés
#   config/sensors/cam_config.json    : config caméra (pas de runtime intermédiaire)
#   config/sensors/mic_config.json    : config microphone
#   config/appli_config.json          : config applications Tasker
#   config/sensors/stt_config.json    : config reconnaissance vocale PocketSphinx
#   config/sensors/voice_config.json  : config sortie audio (vol, output, ttsRate)
#
# NON TRAITÉ ICI
# --------------
#   Bloc "mtr" de courant_init.json : lu directement par rz_stt_handler.sh
#   (valeurs de configuration stables, pas de fichier runtime séparé nécessaire)
#   global.json : généré par courant_init.sh (état initial) puis mis à jour
#   au runtime par rz_state-manager.sh et rz_sys_monitor.sh.
#
# ARTICULATION
# ------------
#   courant_init.sh   → courant_init.json
#   courant_init.json → check_config.sh → *_runtime.json / *_config.json
#   *_runtime.json    → managers (rz_gyro_manager.sh, rz_mg_manager.sh,
#                                  rz_sys_monitor.sh, ...)
#   stt_config.json   → rz_stt_manager.py (au démarrage)
#
# TABLE A - SECTIONS TRAITÉES
# ----------------------------
#   Gyro  : modeGyro, angleVSEBase, paraGyro {freqCont(Hz), freqMoy(Hz),
#            nbValPourMoy, seuilMesure{x,y,z}, seuilAlerte{x,y}}
#   Mag   : modeMg, paraMg {freqCont(Hz), freqMoy(Hz), nbValPourMoy,
#            seuilMesure{x,y,z}}
#   Sys   : modeSys, paraSys {freqCont(s), freqMoy(s), nbCycles,
#            thresholds:{cpu,thermal,storage,mem}:{warn,urg} → alerte si > seuil
#                       {batt}:{warn,urg}                    → alerte si < seuil}
#   Cam   : instances {front,rear} {res,fps,quality,urgDelay}
#            profils CPU {normal,optimized}
#   Mic   : modeMicro, paraMicro {freqCont(ms), winSize(ms), seuils, balayage}
#   Appli : états initiaux, tâches Tasker, packages Android, ExprTasker
#   STT   : modeSTT, threshold, keyphrase, lang, lib_path
#            + vérification présence fichiers PocketSphinx (model-fr/, dicts)
#
# NOTE UNITÉS FRÉQUENCES
# -----------------------
# Gyro/Mag : freqCont/freqMoy en Hz       (plus grand = plus fréquent)
# Sys      : freqCont/freqMoy en secondes (plus petit  = plus fréquent)
# Mic      : freqCont/winSize  en ms      (plus petit  = plus fréquent)
#
# NOTE SEUILS BATTERIE
# --------------------
# batt.warn et batt.urg fonctionnent en sens INVERSE des autres métriques :
#   alerte si valeur < seuil (batterie trop basse)
#   Contrainte : batt.urg < batt.warn (ex: warn=30%, urg=15%)
#
# NOTE LIB_POCKETSPHINX
# ----------------------
# model-fr/ et lexique_referent.dict sont des fichiers lourds (~50-200 Mo),
# exclus du dépôt Git. Ce script vérifie leur présence mais ne les crée
# ni ne les écrase JAMAIS. En cas d'absence : avertissement dans le log,
# les autres sections continuent (STT sera inopérant mais le reste fonctionne).
# fr.dict est généré par rz_build_dict.py — son absence est une erreur légère.
#
# PRÉCAUTIONS
# -----------
# - À lancer AVANT tous les managers de capteurs.
# - En cas d'erreur de validation : arrêt immédiat (exit 1).
# - Champs absents dans courant_init.json : valeur par défaut appliquée
#   avec avertissement dans le log.
#
# DÉPENDANCES
# -----------
#   jq : manipulation JSON (pkg install jq)
#
# AUTEUR  : Vincent Philippe
# VERSION : 9.0  (ajout section STT + vérification lib_pocketsphinx)
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

INPUT_JSON="$BASE_DIR/config/courant_init.json"
RUNTIME_GYRO="$BASE_DIR/config/gyro_runtime.json"
RUNTIME_MAG="$BASE_DIR/config/mag_runtime.json"
RUNTIME_SYS="$BASE_DIR/config/sys_runtime.json"
CAM_CONFIG="$BASE_DIR/config/sensors/cam_config.json"
MIC_CONFIG="$BASE_DIR/config/sensors/mic_config.json"
APPLI_CONFIG="$BASE_DIR/config/appli_config.json"
STT_CONFIG="$BASE_DIR/config/sensors/stt_config.json"
VOICE_CONFIG="$BASE_DIR/config/sensors/voice_config.json"
# Chemin vers le dossier du script STT (lib_path est relatif à ce dossier)
STT_SCRIPT_DIR="$BASE_DIR/scripts/sensors/stt"

LOG_FILE="$BASE_DIR/logs/check_config.log"

# =============================================================================
# UTILITAIRES
# =============================================================================

mkdir -p "$(dirname "$LOG_FILE")"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [CHECK] $1" | tee -a "$LOG_FILE"
}

erreur() {
    log "ERREUR : $1"
    exit 1
}

# Lit un champ JSON avec valeur par défaut si absent ou null
lire_json() {
    local val
    val=$(jq -r "${1} // \"${2}\"" "$INPUT_JSON" 2>/dev/null)
    [ "$val" = "null" ] && val="$2"
    echo "$val"
}

# =============================================================================
# VÉRIFICATIONS PRÉALABLES
# =============================================================================

log "=========================================="
log "Démarrage check_config.sh  v9.0"
log "=========================================="

if ! command -v jq &>/dev/null; then
    erreur "jq non installé (pkg install jq)"
fi

if [ ! -f "$INPUT_JSON" ]; then
    erreur "courant_init.json introuvable : $INPUT_JSON"
fi

log "Source : $INPUT_JSON"

# =============================================================================
# SECTION GYRO — Lecture et validation
# =============================================================================

log "--- Validation section Gyro ---"

if ! jq -e '.gyro' "$INPUT_JSON" > /dev/null 2>&1; then
    erreur "Bloc '.gyro' absent de courant_init.json"
fi

modeGyro=$(lire_json '.gyro.modeGyro' 'OFF')
case "$modeGyro" in
    OFF|DATACont|DATAMoy|ALERTE) log "  modeGyro = $modeGyro  ✓" ;;
    *) erreur "modeGyro invalide : '$modeGyro' (attendu : OFF|DATACont|DATAMoy|ALERTE)" ;;
esac

angleVSEBase=$(lire_json '.gyro.angleVSEBase' '0')
if ! [[ "$angleVSEBase" =~ ^-?[0-9]+$ ]]; then erreur "angleVSEBase non entier : '$angleVSEBase'"; fi
log "  angleVSEBase = $angleVSEBase deg×10  ✓"

freqCont=$(lire_json '.gyro.paraGyro.freqCont' '10')
if ! [[ "$freqCont" =~ ^[0-9]+$ ]] || [ "$freqCont" -le 0 ]; then erreur "gyro.freqCont invalide : '$freqCont'"; fi
log "  paraGyro.freqCont = ${freqCont} Hz  ✓"

freqMoy=$(lire_json '.gyro.paraGyro.freqMoy' '2')
if ! [[ "$freqMoy" =~ ^[0-9]+$ ]] || [ "$freqMoy" -le 0 ]; then erreur "gyro.freqMoy invalide : '$freqMoy'"; fi
log "  paraGyro.freqMoy = ${freqMoy} Hz  ✓"

nbValPourMoy=$(lire_json '.gyro.paraGyro.nbValPourMoy' '5')
if ! [[ "$nbValPourMoy" =~ ^[0-9]+$ ]] || [ "$nbValPourMoy" -le 0 ]; then erreur "gyro.nbValPourMoy invalide : '$nbValPourMoy'"; fi
log "  paraGyro.nbValPourMoy = $nbValPourMoy  ✓"

gyro_smX=$(lire_json '.gyro.paraGyro.seuilMesure.x' '10')
gyro_smY=$(lire_json '.gyro.paraGyro.seuilMesure.y' '10')
gyro_smZ=$(lire_json '.gyro.paraGyro.seuilMesure.z' '5')
for val in "$gyro_smX" "$gyro_smY" "$gyro_smZ"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]]; then erreur "gyro.seuilMesure invalide : '$val'"; fi
done
log "  paraGyro.seuilMesure = [${gyro_smX},${gyro_smY},${gyro_smZ}] rad/s×100  ✓"

seuilAlerteX=$(lire_json '.gyro.paraGyro.seuilAlerte.x' '300')
seuilAlerteY=$(lire_json '.gyro.paraGyro.seuilAlerte.y' '300')
for val in "$seuilAlerteX" "$seuilAlerteY"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]] || [ "$val" -le 0 ]; then erreur "gyro.seuilAlerte invalide : '$val'"; fi
done
[ "$seuilAlerteX" -le "$gyro_smX" ] && log "  WARN : seuilAlerte.x <= seuilMesure.x"
[ "$seuilAlerteY" -le "$gyro_smY" ] && log "  WARN : seuilAlerte.y <= seuilMesure.y"
log "  paraGyro.seuilAlerte = [${seuilAlerteX},${seuilAlerteY}] rad/s×100  ✓"

log "--- Section Gyro : VALIDÉE ---"

cat > "$RUNTIME_GYRO" <<EOF
{
  "gyro": {
    "modeGyro": "$modeGyro",
    "angleVSEBase": $angleVSEBase,
    "paraGyro": {
      "freqCont": $freqCont,
      "freqMoy": $freqMoy,
      "nbValPourMoy": $nbValPourMoy,
      "seuilMesure": { "x": $gyro_smX, "y": $gyro_smY, "z": $gyro_smZ },
      "seuilAlerte": { "x": $seuilAlerteX, "y": $seuilAlerteY }
    }
  }
}
EOF
if ! jq '.' "$RUNTIME_GYRO" > /dev/null 2>&1; then erreur "gyro_runtime.json invalide !"; fi
log "gyro_runtime.json généré et validé  ✓"

# =============================================================================
# SECTION MAG — Lecture et validation
# =============================================================================

log "--- Validation section Mag ---"

if ! jq -e '.mag' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.mag' absent"; fi

modeMg=$(lire_json '.mag.modeMg' 'OFF')
case "$modeMg" in
    OFF|DATABrutCont|DATAGeoCont|DATAGeoMoy|DATAMgCont|DATAMgMoy) log "  modeMg = $modeMg  ✓" ;;
    *) erreur "modeMg invalide : '$modeMg'" ;;
esac

mag_freqCont=$(lire_json '.mag.paraMg.freqCont' '5')
if ! [[ "$mag_freqCont" =~ ^[0-9]+$ ]] || [ "$mag_freqCont" -le 0 ]; then erreur "mag.freqCont invalide"; fi
log "  paraMg.freqCont = ${mag_freqCont} Hz  ✓"

mag_freqMoy=$(lire_json '.mag.paraMg.freqMoy' '1')
if ! [[ "$mag_freqMoy" =~ ^[0-9]+$ ]] || [ "$mag_freqMoy" -le 0 ]; then erreur "mag.freqMoy invalide"; fi
log "  paraMg.freqMoy = ${mag_freqMoy} Hz  ✓"

mag_nbVal=$(lire_json '.mag.paraMg.nbValPourMoy' '5')
if ! [[ "$mag_nbVal" =~ ^[0-9]+$ ]] || [ "$mag_nbVal" -le 0 ]; then erreur "mag.nbValPourMoy invalide"; fi
log "  paraMg.nbValPourMoy = $mag_nbVal  ✓"

mag_smX=$(lire_json '.mag.paraMg.seuilMesure.x' '50')
mag_smY=$(lire_json '.mag.paraMg.seuilMesure.y' '50')
mag_smZ=$(lire_json '.mag.paraMg.seuilMesure.z' '50')
for val in "$mag_smX" "$mag_smY" "$mag_smZ"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]]; then erreur "mag.seuilMesure invalide : '$val'"; fi
done
log "  paraMg.seuilMesure = [${mag_smX},${mag_smY},${mag_smZ}] µT×100  ✓"

log "--- Section Mag : VALIDÉE ---"

cat > "$RUNTIME_MAG" <<EOF
{
  "mag": {
    "modeMg": "$modeMg",
    "paraMg": {
      "freqCont": $mag_freqCont,
      "freqMoy": $mag_freqMoy,
      "nbValPourMoy": $mag_nbVal,
      "seuilMesure": { "x": $mag_smX, "y": $mag_smY, "z": $mag_smZ }
    }
  }
}
EOF
if ! jq '.' "$RUNTIME_MAG" > /dev/null 2>&1; then erreur "mag_runtime.json invalide !"; fi
log "mag_runtime.json généré et validé  ✓"

# =============================================================================
# SECTION SYS — Lecture et validation
# =============================================================================

log "--- Validation section Sys ---"

if ! jq -e '.sys' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.sys' absent"; fi

modeSys=$(lire_json '.sys.modeSys' 'FLOW')
case "$modeSys" in
    OFF|FLOW|MOY|ALERTE) log "  modeSys = $modeSys  ✓" ;;
    *) erreur "modeSys invalide : '$modeSys' (attendu : OFF|FLOW|MOY|ALERTE)" ;;
esac

sys_freqCont=$(lire_json '.sys.paraSys.freqCont' '10')
if ! [[ "$sys_freqCont" =~ ^[0-9]+$ ]] || [ "$sys_freqCont" -le 0 ]; then
    erreur "sys.freqCont invalide : '$sys_freqCont'"
fi
log "  paraSys.freqCont = ${sys_freqCont}s  ✓"

sys_freqMoy=$(lire_json '.sys.paraSys.freqMoy' '60')
if ! [[ "$sys_freqMoy" =~ ^[0-9]+$ ]] || [ "$sys_freqMoy" -le 0 ]; then
    erreur "sys.freqMoy invalide : '$sys_freqMoy'"
fi
if [ "$sys_freqMoy" -lt "$sys_freqCont" ]; then
    erreur "sys.freqMoy (${sys_freqMoy}s) < sys.freqCont (${sys_freqCont}s) : période MOY doit être >= CONT"
fi
log "  paraSys.freqMoy = ${sys_freqMoy}s  ✓"

sys_nbCycles=$(lire_json '.sys.paraSys.nbCycles' '3')
if ! [[ "$sys_nbCycles" =~ ^[0-9]+$ ]] || [ "$sys_nbCycles" -le 0 ]; then
    erreur "sys.nbCycles invalide : '$sys_nbCycles'"
fi
log "  paraSys.nbCycles = $sys_nbCycles  ✓"

# Seuils SENS HAUT (alerte si > seuil) — contrainte : urg > warn
sys_warn_cpu=$(lire_json '.sys.paraSys.thresholds.cpu.warn' '80')
sys_urg_cpu=$( lire_json '.sys.paraSys.thresholds.cpu.urg'  '90')
if ! [[ "$sys_warn_cpu" =~ ^[0-9]+$ ]] || [ "$sys_warn_cpu" -le 0 ]; then erreur "sys.thresholds.cpu.warn invalide"; fi
if ! [[ "$sys_urg_cpu"  =~ ^[0-9]+$ ]] || [ "$sys_urg_cpu"  -le 0 ]; then erreur "sys.thresholds.cpu.urg invalide"; fi
if [ "$sys_urg_cpu" -le "$sys_warn_cpu" ]; then erreur "sys.cpu : urg (${sys_urg_cpu}) doit être > warn (${sys_warn_cpu})"; fi
log "  thresholds.cpu = warn:${sys_warn_cpu} urg:${sys_urg_cpu} (sens haut)  ✓"

sys_warn_thermal=$(lire_json '.sys.paraSys.thresholds.thermal.warn' '70')
sys_urg_thermal=$( lire_json '.sys.paraSys.thresholds.thermal.urg'  '85')
if ! [[ "$sys_warn_thermal" =~ ^[0-9]+$ ]] || [ "$sys_warn_thermal" -le 0 ]; then erreur "sys.thresholds.thermal.warn invalide"; fi
if ! [[ "$sys_urg_thermal"  =~ ^[0-9]+$ ]] || [ "$sys_urg_thermal"  -le 0 ]; then erreur "sys.thresholds.thermal.urg invalide"; fi
if [ "$sys_urg_thermal" -le "$sys_warn_thermal" ]; then erreur "sys.thermal : urg (${sys_urg_thermal}) doit être > warn (${sys_warn_thermal})"; fi
log "  thresholds.thermal = warn:${sys_warn_thermal} urg:${sys_urg_thermal} (sens haut)  ✓"

sys_warn_storage=$(lire_json '.sys.paraSys.thresholds.storage.warn' '85')
sys_urg_storage=$( lire_json '.sys.paraSys.thresholds.storage.urg'  '95')
if ! [[ "$sys_warn_storage" =~ ^[0-9]+$ ]] || [ "$sys_warn_storage" -le 0 ]; then erreur "sys.thresholds.storage.warn invalide"; fi
if ! [[ "$sys_urg_storage"  =~ ^[0-9]+$ ]] || [ "$sys_urg_storage"  -le 0 ]; then erreur "sys.thresholds.storage.urg invalide"; fi
if [ "$sys_urg_storage" -le "$sys_warn_storage" ]; then erreur "sys.storage : urg (${sys_urg_storage}) doit être > warn (${sys_warn_storage})"; fi
log "  thresholds.storage = warn:${sys_warn_storage} urg:${sys_urg_storage} (sens haut)  ✓"

sys_warn_mem=$(lire_json '.sys.paraSys.thresholds.mem.warn' '80')
sys_urg_mem=$( lire_json '.sys.paraSys.thresholds.mem.urg'  '90')
if ! [[ "$sys_warn_mem" =~ ^[0-9]+$ ]] || [ "$sys_warn_mem" -le 0 ]; then erreur "sys.thresholds.mem.warn invalide"; fi
if ! [[ "$sys_urg_mem"  =~ ^[0-9]+$ ]] || [ "$sys_urg_mem"  -le 0 ]; then erreur "sys.thresholds.mem.urg invalide"; fi
if [ "$sys_urg_mem" -le "$sys_warn_mem" ]; then erreur "sys.mem : urg (${sys_urg_mem}) doit être > warn (${sys_warn_mem})"; fi
log "  thresholds.mem = warn:${sys_warn_mem} urg:${sys_urg_mem} (sens haut)  ✓"

# Seuils SENS BAS (alerte si < seuil) — ⚠️ contrainte : urg < warn
sys_warn_batt=$(lire_json '.sys.paraSys.thresholds.batt.warn' '30')
sys_urg_batt=$( lire_json '.sys.paraSys.thresholds.batt.urg'  '15')
if ! [[ "$sys_warn_batt" =~ ^[0-9]+$ ]] || [ "$sys_warn_batt" -le 0 ]; then erreur "sys.thresholds.batt.warn invalide"; fi
if ! [[ "$sys_urg_batt"  =~ ^[0-9]+$ ]] || [ "$sys_urg_batt"  -le 0 ]; then erreur "sys.thresholds.batt.urg invalide"; fi
if [ "$sys_urg_batt" -ge "$sys_warn_batt" ]; then
    erreur "sys.batt : urg (${sys_urg_batt}%) doit être < warn (${sys_warn_batt}%) — sens inversé (alerte si batterie trop basse)"
fi
log "  thresholds.batt = warn<${sys_warn_batt}% urg<${sys_urg_batt}% (sens bas)  ✓"

log "--- Section Sys : VALIDÉE ---"

cat > "$RUNTIME_SYS" <<EOF
{
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
  }
}
EOF
if ! jq '.' "$RUNTIME_SYS" > /dev/null 2>&1; then erreur "sys_runtime.json invalide !"; fi
log "sys_runtime.json généré et validé  ✓"

# =============================================================================
# SECTION CAM — Lecture, validation et génération de cam_config.json
# =============================================================================
# Contrairement aux autres modules, le fichier de sortie est cam_config.json
# directement (fichier opérationnel lu par rz_camera_manager.sh).
# Pas de cam_runtime.json intermédiaire.
# =============================================================================

log "--- Validation section Cam ---"

if ! jq -e '.cam' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.cam' absent"; fi

cam_rear_res=$(      lire_json '.cam.rear.res'      '1080p')
cam_rear_fps=$(      lire_json '.cam.rear.fps'      '30')
cam_rear_quality=$(  lire_json '.cam.rear.quality'  '80')
cam_rear_urgDelay=$( lire_json '.cam.rear.urgDelay' '5')
for val in "$cam_rear_fps" "$cam_rear_quality" "$cam_rear_urgDelay"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]] || [ "$val" -le 0 ]; then erreur "cam.rear valeur invalide : '$val'"; fi
done
case "$cam_rear_res" in low|medium|high|720p|1080p) ;; *) erreur "cam.rear.res invalide : '$cam_rear_res'";; esac
log "  rear : res=$cam_rear_res fps=$cam_rear_fps quality=$cam_rear_quality urgDelay=${cam_rear_urgDelay}s  ✓"

cam_front_res=$(      lire_json '.cam.front.res'      '720p')
cam_front_fps=$(      lire_json '.cam.front.fps'      '15')
cam_front_quality=$(  lire_json '.cam.front.quality'  '80')
cam_front_urgDelay=$( lire_json '.cam.front.urgDelay' '5')
for val in "$cam_front_fps" "$cam_front_quality" "$cam_front_urgDelay"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]] || [ "$val" -le 0 ]; then erreur "cam.front valeur invalide : '$val'"; fi
done
case "$cam_front_res" in low|medium|high|720p|1080p) ;; *) erreur "cam.front.res invalide : '$cam_front_res'";; esac
log "  front : res=$cam_front_res fps=$cam_front_fps quality=$cam_front_quality urgDelay=${cam_front_urgDelay}s  ✓"

cam_profile_normal_res=$(       lire_json '.cam.profiles.normal.res'       '720p')
cam_profile_normal_fps=$(       lire_json '.cam.profiles.normal.fps'       '30')
cam_profile_normal_quality=$(   lire_json '.cam.profiles.normal.quality'   '80')
cam_profile_optimized_res=$(    lire_json '.cam.profiles.optimized.res'    'low')
cam_profile_optimized_fps=$(    lire_json '.cam.profiles.optimized.fps'    '10')
cam_profile_optimized_quality=$(lire_json '.cam.profiles.optimized.quality' '50')
for val in "$cam_profile_normal_fps" "$cam_profile_optimized_fps"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]] || [ "$val" -le 0 ]; then erreur "cam.profiles fps invalide : '$val'"; fi
done
case "$cam_profile_normal_res" in    low|medium|high|720p|1080p) ;; *) erreur "cam.profiles.normal.res invalide";; esac
case "$cam_profile_optimized_res" in low|medium|high|720p|1080p) ;; *) erreur "cam.profiles.optimized.res invalide";; esac
if [ "$cam_profile_optimized_fps" -ge "$cam_profile_normal_fps" ]; then
    log "  WARN : profiles.optimized.fps (${cam_profile_optimized_fps}) >= normal.fps (${cam_profile_normal_fps})"
fi
log "  profiles.normal : res=$cam_profile_normal_res fps=$cam_profile_normal_fps  ✓"
log "  profiles.optimized : res=$cam_profile_optimized_res fps=$cam_profile_optimized_fps  ✓"

log "--- Section Cam : VALIDÉE ---"

mkdir -p "$(dirname "$CAM_CONFIG")"
cat > "$CAM_CONFIG" <<EOF
{
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
}
EOF
if ! jq '.' "$CAM_CONFIG" > /dev/null 2>&1; then erreur "cam_config.json invalide !"; fi
log "cam_config.json généré et validé  ✓"

# =============================================================================
# SECTION APPLI — Lecture, validation et génération de appli_config.json
# =============================================================================

log "--- Validation section Appli ---"

if ! jq -e '.appli' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.appli' absent"; fi

appli_Baby_state=$(       lire_json '.appli.Baby.state'        'Off')
appli_tasker_state=$(     lire_json '.appli.tasker.state'      'Off')
appli_zoom_state=$(       lire_json '.appli.zoom.state'        'Off')
appli_BabyMonitor_state=$(lire_json '.appli.BabyMonitor.state' 'Off')
appli_NavGPS_state=$(     lire_json '.appli.NavGPS.state'      'Off')

appli_Baby_task=$(        lire_json '.appli.Baby.tasker_task'        'RZ_Baby')
appli_zoom_task=$(        lire_json '.appli.zoom.tasker_task'        'RZ_Zoom')
appli_BabyMonitor_task=$( lire_json '.appli.BabyMonitor.tasker_task' 'RZ_BabyMonitor')
appli_NavGPS_task=$(      lire_json '.appli.NavGPS.tasker_task'      'RZ_NavGPS')
appli_tasker_pkg=$(       lire_json '.appli.tasker.package'          'net.dinglisch.android.taskerm')
appli_zoom_pkg=$(         lire_json '.appli.zoom.package'            'us.zoom.videomeetings')
appli_NavGPS_pkg=$(       lire_json '.appli.NavGPS.package'          'com.google.android.apps.maps')
appli_bridge_timeout=$(   lire_json '.appli.bridge_timeout'          '5')
appli_expr_task=$(        lire_json '.appli.ExprTasker.expr_task'    'RZ_Expression')
appli_info_task=$(        lire_json '.appli.ExprTasker.info_task'    'RZ_Info')

# Validation états initiaux (On|Off)
for app_state in "$appli_Baby_state" "$appli_tasker_state" "$appli_zoom_state" \
                 "$appli_BabyMonitor_state" "$appli_NavGPS_state"; do
    case "$app_state" in On|Off) ;; *) erreur "appli : état invalide '$app_state' (attendu : On|Off)";; esac
done

# Avertissement si un état initial n'est pas Off (sécurité démarrage)
for app_state in "$appli_Baby_state" "$appli_tasker_state" "$appli_zoom_state" \
                 "$appli_BabyMonitor_state" "$appli_NavGPS_state"; do
    [ "$app_state" != "Off" ] && log "  WARN : état initial '$app_state' non Off — recommandé Off au démarrage"
done

# Noms de tâches Tasker non vides
for task in "$appli_Baby_task" "$appli_zoom_task" "$appli_BabyMonitor_task" "$appli_NavGPS_task"; do
    [ -z "$task" ] && erreur "appli : tasker_task vide pour une app Tasker-dépendante"
done
[ -z "$appli_expr_task" ] && erreur "appli.ExprTasker.expr_task vide"
[ -z "$appli_info_task"  ] && erreur "appli.ExprTasker.info_task vide"

if ! [[ "$appli_bridge_timeout" =~ ^[0-9]+$ ]] || [ "$appli_bridge_timeout" -le 0 ]; then
    erreur "appli.bridge_timeout invalide : '$appli_bridge_timeout'"
fi

log "  ExprTasker : expr_task=$appli_expr_task info_task=$appli_info_task  ✓"
log "--- Section Appli : VALIDÉE ---"

mkdir -p "$(dirname "$APPLI_CONFIG")"
cat > "$APPLI_CONFIG" <<EOF
{
  "appli": {
    "Baby":        { "state": "$appli_Baby_state",        "tasker_task": "$appli_Baby_task",        "package": "",                  "last_change": "" },
    "tasker":      { "state": "$appli_tasker_state",      "tasker_task": "",                        "package": "$appli_tasker_pkg", "last_change": "" },
    "zoom":        { "state": "$appli_zoom_state",        "tasker_task": "$appli_zoom_task",        "package": "$appli_zoom_pkg",   "last_change": "" },
    "BabyMonitor": { "state": "$appli_BabyMonitor_state", "tasker_task": "$appli_BabyMonitor_task", "package": "",                  "last_change": "" },
    "NavGPS":      { "state": "$appli_NavGPS_state",      "tasker_task": "$appli_NavGPS_task",      "package": "$appli_NavGPS_pkg", "last_change": "" },
    "bridge_timeout": $appli_bridge_timeout,
    "ExprTasker": { "expr_task": "$appli_expr_task", "info_task": "$appli_info_task" }
  }
}
EOF
if ! jq '.' "$APPLI_CONFIG" > /dev/null 2>&1; then erreur "appli_config.json invalide !"; fi
log "appli_config.json généré et validé  ✓"

# =============================================================================
# SECTION MIC — Lecture, validation et génération de mic_config.json
# =============================================================================
# Contrainte fenêtre : winSize >= freqCont (sinon NB_FENETRE < 2)
# Contrainte seuils  : seuilFaible < seuilMoyen < seuilFort (0-1000)
# Durée balayage estimée = (180/pasBalayage + 1) × (200 + winSize) ms
# =============================================================================

log "--- Validation section Mic ---"

if ! jq -e '.mic' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.mic' absent"; fi

mic_mode=$(              lire_json '.mic.modeMicro'                    'off')
mic_freqCont=$(          lire_json '.mic.paraMicro.freqCont'           '300')
mic_winSize=$(           lire_json '.mic.paraMicro.winSize'            '300')
mic_seuilDelta=$(        lire_json '.mic.paraMicro.seuilDelta'         '30')
mic_holdTime=$(          lire_json '.mic.paraMicro.holdTime'           '500')
mic_seuilFaible=$(       lire_json '.mic.paraMicro.seuilFaible'        '100')
mic_seuilMoyen=$(        lire_json '.mic.paraMicro.seuilMoyen'         '400')
mic_seuilFort=$(         lire_json '.mic.paraMicro.seuilFort'          '700')
mic_pasBalayage=$(       lire_json '.mic.paraMicro.pasBalayage'        '10')
mic_timeoutOrientation=$(lire_json '.mic.paraMicro.timeoutOrientation' '15')

case "$mic_mode" in
    off|normal|intensite|orientation|surveillance) log "  modeMicro = $mic_mode  ✓" ;;
    *) erreur "mic.modeMicro invalide : '$mic_mode' (attendu : off|normal|intensite|orientation|surveillance)" ;;
esac

for val in "$mic_freqCont" "$mic_winSize" "$mic_holdTime" "$mic_pasBalayage" "$mic_timeoutOrientation"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]] || [ "$val" -le 0 ]; then erreur "mic.paraMicro valeur invalide : '$val'"; fi
done
for val in "$mic_seuilDelta" "$mic_seuilFaible" "$mic_seuilMoyen" "$mic_seuilFort"; do
    if ! [[ "$val" =~ ^[0-9]+$ ]]; then erreur "mic.paraMicro seuil invalide : '$val'"; fi
done

(( mic_winSize < mic_freqCont ))       && erreur "mic : winSize (${mic_winSize}ms) < freqCont (${mic_freqCont}ms)"
(( mic_seuilFaible >= mic_seuilMoyen )) && erreur "mic : seuilFaible (${mic_seuilFaible}) >= seuilMoyen (${mic_seuilMoyen})"
(( mic_seuilMoyen  >= mic_seuilFort  )) && erreur "mic : seuilMoyen (${mic_seuilMoyen}) >= seuilFort (${mic_seuilFort})"
for val in "$mic_seuilFaible" "$mic_seuilMoyen" "$mic_seuilFort"; do
    (( val > 1000 )) && erreur "mic : seuil $val hors plage 0-1000"
done

nb_positions=$(( 180 / mic_pasBalayage + 1 ))
duree_s=$(( nb_positions * (200 + mic_winSize) / 1000 ))
log "  Balayage estimé : $nb_positions positions ≈ ${duree_s}s"
(( duree_s > mic_timeoutOrientation )) && \
    log "  WARN : balayage estimé (${duree_s}s) > timeoutOrientation (${mic_timeoutOrientation}s)"

log "  freqCont=${mic_freqCont}ms winSize=${mic_winSize}ms seuils=[${mic_seuilFaible},${mic_seuilMoyen},${mic_seuilFort}]  ✓"
log "--- Section Mic : VALIDÉE ---"

mkdir -p "$(dirname "$MIC_CONFIG")"
cat > "$MIC_CONFIG" <<EOF
{
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
  }
}
EOF
if ! jq '.' "$MIC_CONFIG" > /dev/null 2>&1; then erreur "mic_config.json invalide !"; fi
log "mic_config.json généré et validé  ✓"

# =============================================================================
# SECTION STT — Lecture, validation et génération de stt_config.json
# =============================================================================
# Valide les paramètres PocketSphinx puis vérifie la présence des fichiers
# modèles dans lib_pocketsphinx/. Ces fichiers ne sont JAMAIS écrits ici.
#
# Fichiers vérifiés (présence uniquement, pas d'écriture) :
#   lib_pocketsphinx/model-fr/             ← modèle acoustique (~50-200 Mo, hors Git)
#   lib_pocketsphinx/lexique_referent.dict ← dictionnaire source (hors Git)
#   lib_pocketsphinx/fr.dict               ← dictionnaire réduit (généré par rz_build_dict.py)
#
# En cas d'absence de fichiers modèles : WARN dans le log mais pas d'exit 1.
# Les autres modules restent opérationnels — seul le STT sera inopérant.
# =============================================================================

log "--- Validation section STT ---"

if ! jq -e '.stt' "$INPUT_JSON" > /dev/null 2>&1; then erreur "Bloc '.stt' absent"; fi

stt_modeSTT=$(  lire_json '.stt.modeSTT'   'KWS')
stt_threshold=$(lire_json '.stt.threshold' '1e-20')
stt_keyphrase=$(lire_json '.stt.keyphrase' 'rz')
stt_lang=$(     lire_json '.stt.lang'      'fr')
stt_lib_path=$( lire_json '.stt.lib_path'  'lib_pocketsphinx')

# modeSTT : enum KWS|OFF
case "$stt_modeSTT" in
    KWS|OFF) log "  modeSTT = $stt_modeSTT  ✓" ;;
    *) erreur "stt.modeSTT invalide : '$stt_modeSTT' (attendu : KWS|OFF)" ;;
esac

# threshold : nombre entier, décimal ou notation scientifique (ex: 1e-20, 0.001)
if ! [[ "$stt_threshold" =~ ^[0-9]+([.][0-9]+)?([eE][+-]?[0-9]+)?$ ]]; then
    erreur "stt.threshold invalide : '$stt_threshold' (ex: 1e-20 ou 0.001)"
fi
log "  threshold = $stt_threshold  ✓"

# keyphrase : non vide (mot de réveil obligatoire)
if [ -z "$stt_keyphrase" ]; then
    erreur "stt.keyphrase vide — le mot de réveil est obligatoire"
fi
log "  keyphrase = '$stt_keyphrase'  ✓"

# lang : uniquement "fr" dans ce projet
if [ "$stt_lang" != "fr" ]; then
    erreur "stt.lang invalide : '$stt_lang' (seule la langue 'fr' est supportée dans ce projet)"
fi
log "  lang = $stt_lang  ✓"

# lib_path : non vide
if [ -z "$stt_lib_path" ]; then
    erreur "stt.lib_path vide — chemin vers les modèles PocketSphinx obligatoire"
fi
log "  lib_path = '$stt_lib_path'  ✓"

# -------------------------------------------------------------------------
# Vérification présence des fichiers modèles PocketSphinx
# ⚠️ JAMAIS écrits ici — vérification uniquement.
# Ces fichiers sont lourds, hors dépôt Git, à installer manuellement une fois.
# -------------------------------------------------------------------------

STT_LIB="$STT_SCRIPT_DIR/$stt_lib_path"
stt_lib_ok=true

log "  Vérification fichiers PocketSphinx : $STT_LIB"

if [ ! -d "$STT_LIB/model-fr" ]; then
    log "  WARN : model-fr/ absent dans $STT_LIB"
    log "         STT inopérant. Installer manuellement le modèle acoustique français."
    log "         (fichiers lourds exclus du dépôt Git)"
    stt_lib_ok=false
else
    log "  model-fr/ présent  ✓  (non modifié)"
fi

if [ ! -f "$STT_LIB/lexique_referent.dict" ]; then
    log "  WARN : lexique_referent.dict absent dans $STT_LIB"
    log "         rz_build_dict.py ne pourra pas générer fr.dict."
    log "         (fichier lourd exclu du dépôt Git)"
    stt_lib_ok=false
else
    log "  lexique_referent.dict présent  ✓  (non modifié)"
fi

if [ ! -f "$STT_LIB/fr.dict" ]; then
    log "  WARN : fr.dict absent — PocketSphinx ne démarrera pas."
    log "         Générer avec : python3 rz_build_dict.py"
    log "         (après génération de rz_words.txt via rz_build_system.py)"
    stt_lib_ok=false
else
    log "  fr.dict présent  ✓"
fi

$stt_lib_ok && log "  Tous les fichiers PocketSphinx présents  ✓" || \
    log "  WARN : STT potentiellement inopérant — voir avertissements ci-dessus"

log "--- Section STT : VALIDÉE ---"

mkdir -p "$(dirname "$STT_CONFIG")"
cat > "$STT_CONFIG" <<EOF
{
  "stt": {
    "modeSTT":   "$stt_modeSTT",
    "threshold": $stt_threshold,
    "keyphrase": "$stt_keyphrase",
    "lang":      "$stt_lang",
    "lib_path":  "$stt_lib_path"
  }
}
EOF
if ! jq '.' "$STT_CONFIG" > /dev/null 2>&1; then erreur "stt_config.json invalide !"; fi
log "stt_config.json généré et validé  ✓"

# =============================================================================
# SECTION VOICE — Lecture, validation et génération de voice_config.json
# =============================================================================
# ⚠ Voice est INDÉPENDANT du modeSTT.
#   rz_voice_manager.sh reste actif même si la reconnaissance vocale est
#   désactivée — il gère toute la sortie audio SE (TTS, musique, alertes).
#
# vol    : volume initial 0-100%
# output : sortie audio (internal|jack|bt)
# ttsRate: vitesse TTS (float positif, 1.0 = normale)
# =============================================================================

log "--- Validation section Voice ---"

if ! jq -e '.voice' "$INPUT_JSON" > /dev/null 2>&1; then
    log "  WARN : Bloc '.voice' absent de courant_init.json"
    log "         voice_config.json non généré — rz_voice_manager.sh utilisera les défauts"
else

    voice_vol=$(    lire_json '.voice.vol'     '80')
    voice_output=$( lire_json '.voice.output'  'internal')
    voice_ttsRate=$(lire_json '.voice.ttsRate' '1.0')

    # Validation vol (entier 0-100)
    if ! [[ "$voice_vol" =~ ^[0-9]+$ ]] || [ "$voice_vol" -lt 0 ] || [ "$voice_vol" -gt 100 ]; then
        erreur "voice.vol invalide : '$voice_vol' (attendu : entier 0-100)"
    fi
    log "  vol = ${voice_vol}%  ✓"

    # Validation output (enum)
    case "$voice_output" in
        internal|jack|bt) log "  output = $voice_output  ✓" ;;
        *) erreur "voice.output invalide : '$voice_output' (attendu : internal|jack|bt)" ;;
    esac

    # Validation ttsRate (float positif)
    if ! [[ "$voice_ttsRate" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
        erreur "voice.ttsRate invalide : '$voice_ttsRate' (attendu : float > 0, ex: 1.0)"
    fi
    log "  ttsRate = $voice_ttsRate  ✓"

    log "--- Section Voice : VALIDÉE ---"

    mkdir -p "$(dirname "$VOICE_CONFIG")"
    cat > "$VOICE_CONFIG" <<EOF
{
  "voice": {
    "vol":     $voice_vol,
    "output":  "$voice_output",
    "ttsRate": $voice_ttsRate
  }
}
EOF
    if ! jq '.' "$VOICE_CONFIG" > /dev/null 2>&1; then
        erreur "voice_config.json généré est invalide"
    fi
    log "voice_config.json généré et validé  ✓"

fi

# =============================================================================
# RÉSUMÉ FINAL
# =============================================================================

log "=========================================="
log "check_config.sh terminé avec succès"
log "  → $RUNTIME_GYRO"
log "  → $RUNTIME_MAG"
log "  → $RUNTIME_SYS"
log "  → $CAM_CONFIG"
log "  → $APPLI_CONFIG"
log "  → $MIC_CONFIG"
log "  → $STT_CONFIG"
log "  → $VOICE_CONFIG"
log ""
log "  Non traité (lecture directe courant_init.json) :"
log "  → bloc 'mtr'  : lu par rz_stt_handler.sh"
log "  → global.json : géré par courant_init.sh + rz_state-manager.sh"
log "=========================================="