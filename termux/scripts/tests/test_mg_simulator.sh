#!/bin/bash
# =============================================================================
# SCRIPT: test_mg_simulator.sh
# DESCRIPTION: Simulateur de magnétomètre pour tests
# USAGE:
#   ./test_mg_simulator.sh [--mode BRUT|CAPGEO|CAPMAG] [--freq HZ] [--values X,Y,Z]
# =============================================================================

set -euo pipefail

# --- Paramètres par défaut ---
FREQ=10
MODE="BRUT"
VALUES=""

# --- Parsing des arguments ---
while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      if [[ -z "$2" || ! "$2" =~ ^(BRUT|CAPGEO|CAPMAG)$ ]]; then
        echo "ERREUR: Mode invalide (BRUT|CAPGEO|CAPMAG)" >&2
        exit 1
      fi
      MODE="$2"
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
    --values)
      if [[ -z "$2" || ! "$2" =~ ^[0-9,-]+$ ]]; then
        echo "ERREUR: Format invalide pour --values" >&2
        exit 1
      fi
      VALUES="$2"
      shift 2
      ;;
    *)
      echo "Usage: $0 [--mode MODE] [--freq HZ] [--values X,Y,Z]" >&2
      exit 1
      ;;
  esac
done

# --- Génération des données ---
generate_data() {
  if [ -n "$VALUES" ]; then
    IFS=',' read -r X Y Z <<< "$VALUES"
  else
    X=$((RANDOM % 2000 - 1000))
    Y=$((RANDOM % 2000 - 1000))
    Z=$((RANDOM % 2000 - 1000))
  fi

  case "$MODE" in
    "BRUT")
      echo "{\"x\": $X, \"y\": $Y, \"z\": $Z}"
      ;;
    "CAPGEO")
      # Simule un cap géographique (0-3600 degrés×10)
      echo "{\"capGeo\": $((RANDOM % 3600))}"
      ;;
    "CAPMAG")
      # Simule un cap magnétique (0-3600 degrés×10)
      echo "{\"capMag\": $((RANDOM % 3600))}"
      ;;
  esac
}

# --- Boucle principale ---
echo "Simulateur MG démarré (mode=$MODE, freq=${FREQ}Hz)" >&2
while true; do
  generate_data
  sleep "$(bc -l <<< "scale=3; 1/$FREQ")"
done
