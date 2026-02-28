#!/bin/bash
# =============================================================================
# SCRIPT: urgence.sh
# CHEMIN: termux/scripts/sensors/sys/lib/urgence.sh
# AUTEUR: Vincent Philippe
# VERSION: 1.0
# DATE: 2026-02-06
# DESCRIPTION:
#   Gestion des urgences système
# =============================================================================

# Déclenche une urgence
trigger_urgence() {
  local source="$1"
  local niveau="$2"
  local raison="$3"

  # Validation
  if [[ -z "$source" || -z "$niveau" ]]; then
    log_error "Paramètres invalides pour l'urgence"
    return 1
  fi

  # Envoi VPIV
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$E:Urg:source:SE:${source}#" >/dev/null 2>&1

  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$E:Urg:niveau:SE:${niveau}#" >/dev/null 2>&1

  if [[ -n "$raison" ]]; then
    mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                  -m "\$E:Urg:raison:SE:${raison}#" >/dev/null 2>&1
  fi
}

# Vérification des seuils
check_cpu_threshold() {
  local usage="$1"
  local threshold
  threshold=$(jq -r '.thresholds.cpu // 0' "$CONFIG_FILE")

  if (( usage > threshold )); then
    trigger_urgence "URG_CPU_CHARGE" "danger" "CPU: ${usage}%"
  fi
}
