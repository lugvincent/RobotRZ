#!/bin/bash
# =============================================================================
# SCRIPT: test_gyro_simulator_v2.sh
# DESCRIPTION: Simulateur de gyroscope avancé pour tests
#              - Génère des données aléatoires/fixes
#              - Simule des alertes et corrections d'angle
# USAGE:
#   ./test_gyro_simulator_v2.sh [--fixed X,Y,Z] [--freq HZ] [--alert X,Y] [--angleVSE degres]
# EXEMPLES:
#   ./test_gyro_simulator_v2.sh --fixed 100,200,300 --freq 5
#   ./test_gyro_simulator_v2.sh --alert 300,200 --angleVSE 150
# v 2.0 - 2026-01-29 - Adapté pour RZ-SE-Sensors
# =============================================================================

set -euo pipefail

# --- Paramètres par défaut ---
FREQ=10
FIXED_VALUES=""
ALERT_VALUES=""
ANGLE_VSE=0
MODE="normal"  # normal/alert/angle

# --- Parsing des arguments ---
while [[ $# -gt 0 ]]; do
  case "$1" in
    --fixed)
      if [[ -z "$2" || ! "$2" =~ ^[0-9,-]+$ ]]; then
        echo "ERREUR: Format invalide pour --fixed" >&2
        exit 1
      fi
      FIXED_VALUES="$2"
      shift 2
      ;;
    --freq)
      if [[ -z "$2" || ! "$2" =~ ^[0-9]+$ || "$2" -le 0 ]]; then
        echo "ERREUR: --freq doit être un entier positif" >&2
        exit 1
      fi
      FREQ="$2"
      shift 2
      ;;
    --alert)
      if [[ -z "$2" || ! "$2" =~ ^[0-9,-]+$ ]]; then
        echo "ERREUR: Format invalide pour --alert" >&2
        exit 1
      fi
      ALERT_VALUES="$2"
      MODE="alert"
      shift 2
      ;;
    --angleVSE)
      if [[ -z "$2" || ! "$2" =~ ^[0-9]+$ ]]; then
        echo "ERREUR: --angleVSE doit être un entier" >&2
        exit 1
      fi
      ANGLE_VSE="$2"
      MODE="angle"
      shift 2
      ;;
    *)
      echo "Usage: $0 [--fixed X,Y,Z] [--freq HZ] [--alert X,Y] [--angleVSE degres]" >&2
      exit 1
      ;;
  esac
done

# --- Génération des données ---
generate_data() {
  if [ -n "$FIXED_VALUES" ]; then
    IFS=',' read -r X Y Z <<< "$FIXED_VALUES"
  else
    X=$((RANDOM % 2000 - 1000))
    Y=$((RANDOM % 2000 - 1000))
    Z=$((RANDOM % 2000 - 1000))
  fi

  # Ajout des valeurs spécifiques
  case "$MODE" in
    "alert")
      IFS=',' read -r ALERT_X ALERT_Y <<< "$ALERT_VALUES"
      echo "{\"x\": $ALERT_X, \"y\": $ALERT_Y, \"z\": $Z, \"angleVSE\": $ANGLE_VSE}"
      ;;
    "angle")
      echo "{\"x\": $X, \"y\": $Y, \"z\": $Z, \"angleVSE\": $ANGLE_VSE}"
      ;;
    *)
      echo "{\"x\": $X, \"y\": $Y, \"z\": $Z}"
      ;;
  esac
}

# --- Boucle principale ---
echo "Simulateur gyro démarré (freq=${FREQ}Hz, mode=${MODE})" >&2
while true; do
  generate_data
  sleep "$(bc -l <<< "scale=3; 1/$FREQ")"
done
