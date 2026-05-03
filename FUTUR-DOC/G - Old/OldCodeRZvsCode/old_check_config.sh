#!/bin/bash
# =============================================================================
# SCRIPT: check_config.sh
# CHEMIN:
#   - PC: PROJETRZ/termux/config/check_config.sh
#   - Smartphone: ~/scripts_RZ_SE/termux/config/check_config.sh
# DESCRIPTION:
#   Vérifie la cohérence des configurations et charge les valeurs appropriées
#   Gère les fichiers JSON de configuration pour tous les composants
#
# FONCTIONS:
#   - Vérification de la structure des fichiers
#   - Validation des valeurs
#   - Chargement des configurations
#   - Gestion des erreurs
#
# DEPENDANCES:
#   - jq (pour la manipulation JSON)
#
# AUTEUR: Vincent Philippe
# VERSION: 3.1 (Intégration Camera Config)
# DATE: 2026-02-06
# =============================================================================
# Détection de l'environnement
if [ -d "/data/data/com.termux/files" ]; then
  BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
  LOG_DIR="$BASE_DIR/logs"
else
  BASE_DIR="$(dirname "$0")/../.."
  LOG_DIR="$BASE_DIR/logs"
fi

# Chemins des fichiers
ORIGINAL="$BASE_DIR/config/global.json"
CURRENT="$BASE_DIR/config/courant_init.json"
LOG_FILE="$LOG_DIR/init.log"
SENSORS_CONFIG_DIR="$BASE_DIR/config/sensors"

# Fonction de log
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [CHECK] $1" >> "$LOG_FILE"
  echo "$1"
}

# Vérification de la structure
verify_structure() {
  log "Début de la vérification de structure"

  # 1. Vérification de l'existence des fichiers
  if [ ! -f "$ORIGINAL" ]; then
    log "ERREUR: Fichier global.json non trouvé"
    return 1
  fi

  if [ ! -f "$CURRENT" ]; then
    log "ERREUR: Fichier courant_init.json non trouvé"
    return 1
  fi

  # 2. Vérification des clés principales
  if ! diff -q <(jq -r 'keys_unsorted' "$ORIGINAL") <(jq -r 'keys_unsorted' "$CURRENT") >/dev/null; then
    log "ERREUR: Structure des clés différente"
    log "Clés originales: $(jq -r 'keys_unsorted' "$ORIGINAL")"
    log "Clés courantes: $(jq -r 'keys_unsorted' "$CURRENT")"
    return 1
  fi

  log "Structure valide"
  return 0
}

# Chargement de la configuration
load_config() {
  log "Début du chargement de configuration"

  if verify_structure; then
    log "Configuration valide - chargement des valeurs courantes"

    # Distribution vers les fichiers spécifiques (Ajout de cam)
    for sensor in gyro mg sys cam; do
      if [ -f "$SENSORS_CONFIG_DIR/${sensor}_config.json" ]; then
        log "Mise à jour de ${sensor}_config.json"
        jq -r ".$sensor" "$CURRENT" > "$SENSORS_CONFIG_DIR/${sensor}_config.json"
      else
        # Si le fichier n'existe pas, on le crée
        log "Création de ${sensor}_config.json"
        jq -r ".$sensor" "$CURRENT" > "$SENSORS_CONFIG_DIR/${sensor}_config.json"
      fi
    done

    return 0
  else
    log "Configuration invalide - restauration des valeurs par défaut"
    cp "$ORIGINAL" "$CURRENT"
    return 1
  fi
}

# Exécution
log "=========================================="
log "Début de la vérification de configuration"
log "=========================================="

if ! load_config; then
  log "ERREUR: Impossible de charger la configuration"
  exit 1
fi

log "Configuration chargée avec succès"