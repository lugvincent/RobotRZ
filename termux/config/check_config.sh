#!/bin/bash
# =============================================================================
# SCRIPT: check_config.sh
# ROLE:
#   - Lecture courant_init.json (SP intention)
#   - Vérification cohérence Table A
#   - Génération fichier runtime spécifique module
#   - Bootstrap des modules (lancement scripts capteurs)
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
INPUT_JSON="$BASE_DIR/config/courant_init.json"
RUNTIME_JSON="$BASE_DIR/config/gyro_runtime.json"

echo "=== Vérification configuration Gyro ==="

# --------------------------------------------------
# Vérification présence bloc gyro
# --------------------------------------------------

if ! jq -e '.gyro' "$INPUT_JSON" > /dev/null; then
    echo "ERREUR: bloc gyro absent"
    exit 1
fi

modeGyro=$(jq -r '.gyro.modeGyro' "$INPUT_JSON")

# --------------------------------------------------
# Vérification enum modeGyro
# --------------------------------------------------

case "$modeGyro" in
    OFF|DATACont|DATAMoy|ALERTE)
        ;;
    *)
        echo "ERREUR: modeGyro invalide"
        exit 1
        ;;
esac

freqCont=$(jq -r '.gyro.paraGyro.freqCont' "$INPUT_JSON")
freqMoy=$(jq -r '.gyro.paraGyro.freqMoy' "$INPUT_JSON")
nbValPourMoy=$(jq -r '.gyro.paraGyro.nbValPourMoy' "$INPUT_JSON")

# --------------------------------------------------
# Vérifications cohérence numérique
# --------------------------------------------------

if [ "$freqCont" -le 0 ]; then
    echo "ERREUR: freqCont <= 0"
    exit 1
fi

if [ "$freqMoy" -le 0 ]; then
    echo "ERREUR: freqMoy <= 0"
    exit 1
fi

if [ "$nbValPourMoy" -le 0 ]; then
    echo "ERREUR: nbValPourMoy <= 0"
    exit 1
fi

echo "Structure OK"

# --------------------------------------------------
# Génération JSON runtime spécifique module
# --------------------------------------------------

cat > "$RUNTIME_JSON" <<EOF
{
  "modeGyro": "$modeGyro",
  "freqCont": $freqCont,
  "freqMoy": $freqMoy,
  "nbValPourMoy": $nbValPourMoy
}
EOF

echo "gyro_runtime.json généré"
echo "Configuration Gyro VALIDEE"