#!/bin/bash
# =============================================================================
# FICHIER : courant_init.sh
# ROLE :
#   Génère le fichier courant_init.json
#   Source unique de configuration commentée (développement)
#
#   Avantage :
#   - Ajout de commentaires
#   - Historique Git clair
#   - Référence d'architecture

# =============================================================================

BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
OUTPUT_JSON="$BASE_DIR/config/courant_init.json"

# =======================
# PROPRIETES SP → SE GYRO
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