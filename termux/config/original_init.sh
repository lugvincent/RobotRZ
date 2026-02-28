#!/bin/bash
# =============================================================================
# SCRIPT: original_init.sh
# CHEMIN:
#   - PC: PROJETRZ/termux/config/original_init.sh
#   - Smartphone: ~/scripts_RZ_SE/termux/config/original_init.sh
# DESCRIPTION: Génère la structure de base et les valeurs par défaut (État Zéro)
#   - Est exécuté une seule fois sur le smartphone lors de l'installation initiale
#
# FONCTIONS:
#   - Génération des fichiers de configuration
#   - Création des liens symboliques : Lie les configs centrales aux dossiers scripts/sensors
#   - Validation de la structure
#   - Permet de créer un fichier courant_init.json
#     pour stocker les valeurs modifiables par les scripts utile 
#     si on est en mode-autonome sans accès à SP, trace si des nom modifier ds SP
#
# DEPENDANCES:
#   - jq (pour la manipulation JSON)
#
# AUTEUR: Vincent Philippe
# VERSION: 4.0 (Exhaustif: Tous modules intégrés - Fin de Dev SE)
# DATE: 2026-02-17
# =============================================================================

#
  # Smartphone (Termux)
 #!/bin/bash
# =============================================================================
# SCRIPT: original_init.sh
# CHEMIN: ~/scripts_RZ_SE/termux/config/original_init.sh
# DESCRIPTION: État "Zéro" du robot. Définit la cohérence absolue du SE.
# VERSION: 4.0 (Exhaustif : Cam, Torch, Mic, STT, Gyro, MG, Sys)
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_DIR="$BASE_DIR/config"
SENSORS_CONFIG_DIR="$CONFIG_DIR/sensors"
LOG_FILE="$BASE_DIR/logs/init.log"

# --- UTILITAIRES ---
log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INIT] $1" | tee -a "$LOG_FILE"; }

# --- SÉCURITÉ ANTI-RÉINITIALISATION ---
if [ -f "$CONFIG_DIR/courant_init.json" ] && [ "$1" != "--force" ]; then
    log "ALERTE : Système déjà initialisé. Utilisez --force pour écraser."
    exit 0
fi

# --- 1. CRÉATION DE L'ARBORESCENCE ---
create_directories() {
    log "Vérification des répertoires..."
    mkdir -p "$CONFIG_DIR" "$SENSORS_CONFIG_DIR" "$BASE_DIR/logs"
    mkdir -p "$BASE_DIR/scripts/sensors/"{cam,mg,gyro,mic,stt,sys,torch}
}

# --- 2. GÉNÉRATION DE L'ÉTAT GLOBAL (DYNAMIQUE) ---
generate_global_config() {
    log "Génération de global.json..."
    cat > "$CONFIG_DIR/global.json" <<EOF
{
  "Sys": { "cpu_load": 0, "temp": 0, "storage": 0, "uptime": 0, "last_update": "" },
  "CfgS": { "modeRZ": 1, "typePtge": 0, "reset": false },
  "Urg": { "etat": "clear", "source": "NONE" }, #oldUrg: { "statut": "cleared", "raison": "URG_NONE" },
  "COM": { "info": "", "warn": "", "error": "", "debug": "" } #oldCOM: { "debug": "", "error": "", "info": "" }
}
EOF
}

# --- 3. GÉNÉRATION DES CONFIGURATIONS MODULES (STATIQUE) ---
generate_sensor_configs() {
    log "Génération des fichiers de configuration modules..."

    # Système (Alertes et Monitoring)
    cat > "$SENSORS_CONFIG_DIR/sys_config.json" <<EOF
{ "mode": "FLOW", "frequence": 10, "thresholds": { "cpu": {"alert": 90}, "thermal": {"alert": 85} }, "alerts": { "cpu": true, "thermal": true } }
EOF

    # Vision (Profils Caméra)
    cat > "$SENSORS_CONFIG_DIR/cam_config.json" <<EOF
{ "profiles": { "normal": {"res": "1920x1080", "fps": 30}, "optimized": {"res": "1280x720", "fps": 15} }, "status": { "active": "Off", "current_side": "rear" } }
EOF

    # Audio & STT
    cat > "$SENSORS_CONFIG_DIR/mic_config.json" <<EOF
{ "sensibilite": 80, "filtre_bruit": true, "volume": 100 }
EOF
    cat > "$SENSORS_CONFIG_DIR/stt_config.json" <<EOF
{ "modeSTT": "KWS", "threshold": 1e-20, "keyphrase": "rz", "lang": "fr", "lib_path": "lib_pocketsphinx" }
EOF

    # Navigation (Gyro ok & MG)
cat > "$SENSORS_CONFIG_DIR/gyro_config.json" <<EOF
{
  "modeGyro": "OFF",
  "paraM": {
    "frequence_Hz": 20,
    "freqOdo_Hz": 40,
    "seuilMesure_xyz": [10, 10, 5],
    "seuilAlerte_xy": [30, 35],
    "nbValPourMoy": 5,
    "ajustMagnetic_deg10": 0
  }
}
EOF
    cat > "$SENSORS_CONFIG_DIR/mg_config.json" <<EOF
{ "modeMG": "BRUT", "frequence": 5, "modeRZ": [1, 2, 3], "calibration": { "declinaison": 0, "scaleFactor": 1.0 } }
EOF
}

# --- 4. CRÉATION DES LIENS SYMBOLIQUES ---
create_symlinks() {
    log "Création des liens symboliques vers les scripts..."
    # On itère sur la liste exacte de tes modules
    for sensor in sys cam mic stt gyro mg; do
        ln -sf "$SENSORS_CONFIG_DIR/${sensor}_config.json" "$BASE_DIR/scripts/sensors/$sensor/${sensor}_config.json"
        log "Lien OK : $sensor"
    done
}

# --- 5. INITIALISATION DE LA MATRICE COURANTE ---
generate_initial_state() {
    log "Génération de courant_init.json (Matrice Usine)..."
    # On compile les fichiers individuels dans un seul JSON via jq
    jq -n \
      --slurpfile sys "$SENSORS_CONFIG_DIR/sys_config.json" \
      --slurpfile cam "$SENSORS_CONFIG_DIR/cam_config.json" \
      --slurpfile mic "$SENSORS_CONFIG_DIR/mic_config.json" \
      --slurpfile stt "$SENSORS_CONFIG_DIR/stt_config.json" \
      --slurpfile gyro "$SENSORS_CONFIG_DIR/gyro_config.json" \
      --slurpfile mg "$SENSORS_CONFIG_DIR/mg_config.json" \
      '{sys: $sys[0], cam: $cam[0], mic: $mic[0], stt: $stt[0], gyro: $gyro[0], mg: $mg[0], global_overrides: {modeRZ: 1}}' \
      > "$CONFIG_DIR/courant_init.json"
}

# --- 6. VÉRIFICATION DE COHÉRENCE ---
verify_structure() {
    log "Vérification finale de la structure..."
    local errors=0
    [ -f "$CONFIG_DIR/global.json" ] || ((errors++))
    [ -f "$CONFIG_DIR/courant_init.json" ] || ((errors++))
    
    if [ $errors -eq 0 ]; then
        log ">>> STRUCTURE VALIDÉE : SE PRÊT À L'EMPLOI <<<"
        return 0
    else
        log "!!! ERREUR CRITIQUE : Structure incomplète !!!"
        return 1
    fi
}

# --- EXÉCUTION ---
main() {
    log "=========================================="
    log "DÉBUT INITIALISATION SYSTÈME ROBOTZ (SE)"
    log "=========================================="
    create_directories
    generate_global_config
    generate_sensor_configs
    create_symlinks
    generate_initial_state
    verify_structure
}

main