#!/bin/bash
# =============================================================================
# SCRIPT: courant_init.sh
# CHEMIN:
#   - PC: PROJETRZ/termux/config/courant_init.sh
#   - Smartphone: ~/scripts_RZ_SE/termux/config/courant_init.sh
#
# DESCRIPTION:
# Fichier source de configuration SP -> SE.
# Génère automatiquement courant_init.json à partir des variables
# définies ci-dessous.
#
# Ce fichier est la SEULE source à modifier pendant le développement.
#
# VERSION: 1.0 (Gyro stabilisé)
# AUTEUR: Vincent Philippe
# =============================================================================

#!/bin/bash
# =============================================================================
# Fichier source SP → SE
# Conforme Table A Gyro
# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
OUTPUT_JSON="$BASE_DIR/config/courant_init.json"

# =======================
# PROPRIETES SP → SE
# =======================

modeGyro="DATACont"

freqCont=20
freqMoy=2
nbValPourMoy=5

seuilMesureX=10
seuilMesureY=10
seuilMesureZ=10

seuilAlerteX=300
seuilAlerteY=300

angleVSEBase=150

cat > "$OUTPUT_JSON" <<EOF
{
  "gyro": {
    "modeGyro": "$modeGyro",
    "angleVSEBase": $angleVSEBase,
    "paraGyro": {
      "freqCont": $freqCont,
      "freqMoy": $freqMoy,
      "nbValPourMoy": $nbValPourMoy,
      "seuilMesure": {
        "x": $seuilMesureX,
        "y": $seuilMesureY,
        "z": $seuilMesureZ
      },
      "seuilAlerte": {
        "x": $seuilAlerteX,
        "y": $seuilAlerteY
      }
    }
  }
}
EOF