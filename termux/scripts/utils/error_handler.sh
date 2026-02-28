#!/bin/bash
# =============================================================================
# SCRIPT: error_handler.sh
# CHEMIN: termux/utils/error_handler.sh
# DESCRIPTION:
#   Gestion centralisée des erreurs
# version de base à adapter mais déjà utilisable
# et urgences selon la table A avec URG pour cas critiques et COM pour messages d'information
# Exemples d'utilisation :
# handle_urgence "URG_OVERHEAT" "danger" "Température CPU: 85°C"
# handle_message "warn" "Batterie faible (20%)"
# =============================================================================

# Types d'urgences (de la table A)
declare -A URG_TYPES=(
  ["URG_OVERHEAT"]="Surchauffe détectée"
  ["URG_LOW_BAT"]="Niveau batterie faible"
  ["URG_MOTOR_STALL"]="Moteur bloqué"
  ["URG_SENSOR_FAIL"]="Défaillance capteur"
  ["URG_BUFFER_OVERFLOW"]="Dépassement mémoire"
  ["URG_PARSING_ERROR"]="Erreur de parsing"
  ["URG_CPU_CHARGE"]="Charge CPU excessive"
  ["URG_APPLI_FAIL"]="Échec application"
  ["URG_ATTEND-RETOUR"]="Timeout réponse"
  ["URG_LOOP_TOO_SLOW"]="Boucle trop lente"
  ["URG_UNKNOWN"]="Urgence inconnue"
)

# Types d'erreurs (COM)
declare -A ERROR_TYPES=(
  ["debug"]="DEBUG"
  ["error"]="ERREUR"
  ["info"]="INFO"
  ["warn"]="AVERTISSEMENT"
)

# Fonction de gestion des urgences
handle_urgence() {
  local source="$1"
  local niveau="$2"
  local raison="$3"

  # Validation de l'urgence
  if [[ -z "${URG_TYPES[$source]}" ]]; then
    source="URG_UNKNOWN"
  fi

  # Envoi VPIV selon la table A
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$E:Urg:source:SE:${source}#" -q 1 >/dev/null 2>&1

  # Envoi du statut
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$V:Urg:statut:*:active#" -q 1 >/dev/null 2>&1

  # Envoi du niveau
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$E:Urg:niveau:SE:${niveau}#" -q 1 >/dev/null 2>&1

  # Envoi de la raison
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$E:Urg:raison:SE:${raison}#" -q 1 >/dev/null 2>&1
}

# Fonction de gestion des messages COM
handle_message() {
  local type="$1"
  local message="$2"

  if [[ -z "${ERROR_TYPES[$type]}" ]]; then
    type="error"
  fi

  # Envoi VPIV selon la table A
  mosquitto_pub -h "robotz-vincent.duckdns.org" -t "SE/statut" \
                -m "\$I:Com:${type}:SE:${message}#" -q 1 >/dev/null 2>&1
}

# Exemples d'utilisation :
# handle_urgence "URG_OVERHEAT" "danger" "Température CPU: 85°C"
# handle_message "warn" "Batterie faible (20%)"
