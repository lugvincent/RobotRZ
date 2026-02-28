#!/bin/bash
# =============================================================================
# SCRIPT: alerts_sys.sh
# CHEMIN: termux/scripts/sensors/sys/lib/alerts_sys.sh
# DESCRIPTION:
#   Gestion des alertes système
#   1. Vérification des seuils
#   2. Déclenchement des alertes
#   3. Intégration avec error_handler.sh
#
# AUTEUR: Vincent Philippe
# VERSION: 1.0
# DATE: 2026-02-06
# =============================================================================

# Chemins
CONFIG_FILE="sys_config.json"
source "/data/data/com.termux/files/home/scripts_RZ_SEscripts_RZ_SE/termux/utils/logger.sh"
source "/data/data/com.termux/files/home/scripts_RZ_SEscripts_RZ_SE/termux/utils/error_handler.sh"

# Vérification des alertes CPU
check_cpu_alert() {
  local usage="$1"
  local config
  config=$(cat "$CONFIG_FILE")
  local alert_threshold
  alert_threshold=$(jq -r '.thresholds.cpu.alert // 0' <<< "$config")
  local alert_enabled
  alert_enabled=$(jq -r '.alerts.cpu // false' <<< "$config")

  if [[ "$alert_enabled" = "true" ]] && (( usage > alert_threshold )); then
    log_warn "Alerte CPU: ${usage}% > ${alert_threshold}%"
    handle_message "warn" "Alerte CPU: ${usage}%"
  fi
}

# Vérification des alertes thermiques
check_thermal_alert() {
  local temp="$1"
  local config
  config=$(cat "$CONFIG_FILE")
  local alert_threshold
  alert_threshold=$(jq -r '.thresholds.thermal.alert // 0' <<< "$config")
  local alert_enabled
  alert_enabled=$(jq -r '.alerts.thermal // false' <<< "$config")

  if [[ "$alert_enabled" = "true" ]] && (( temp > alert_threshold )); then
    log_warn "Alerte thermique: ${temp}°C > ${alert_threshold}°C"
    handle_message "warn" "Alerte thermique: ${temp}°C"
  fi
}

# Vérification des alertes stockage
check_storage_alert() {
  local storage="$1"
  local config
  config=$(cat "$CONFIG_FILE")
  local alert_threshold
  alert_threshold=$(jq -r '.thresholds.storage.alert // 0' <<< "$config")
  local alert_enabled
  alert_enabled=$(jq -r '.alerts.storage // false' <<< "$config")

  if [[ "$alert_enabled" = "true" ]] && (( storage > alert_threshold )); then
    log_warn "Alerte stockage: ${storage}% > ${alert_threshold}%"
    handle_message "warn" "Alerte stockage: ${storage}%"
  fi
}

# Vérification des alertes mémoire
check_mem_alert() {
  local mem="$1"
  local config
  config=$(cat "$CONFIG_FILE")
  local alert_threshold
  alert_threshold=$(jq -r '.thresholds.mem.alert // 0' <<< "$config")
  local alert_enabled
  alert_enabled=$(jq -r '.alerts.mem // false' <<< "$config")

  if [[ "$alert_enabled" = "true" ]] && (( mem > alert_threshold )); then
    log_warn "Alerte mémoire: ${mem}% > ${alert_threshold}%"
    handle_message "warn" "Alerte mémoire: ${mem}%"
  fi
}
