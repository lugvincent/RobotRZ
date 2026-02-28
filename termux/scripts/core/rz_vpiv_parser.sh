#!/bin/bash
# =============================================================================
# SCRIPT: rz_vpiv_parser.sh
# CHEMIN:
#   - PC: PROJETRZ/termux/scripts/core/rz_vpiv_parser.sh
#   - Smartphone: ~/scripts_RZ_SE/termux/scripts/core/rz_vpiv_parser.sh
# DESCRIPTION: 
#   Analyse des messages SP et gestion des ACK/Urgences.
#   Décode les messages VPIV (MQTT) et aiguille vers les FIFOs ou scripts.
#   Gère le déclenchement de la sécurité locale (Point C).
# =============================================================================

# --- DÉTECTION ENVIRONNEMENT & CHEMINS ---
if [ -d "/data/data/com.termux/files" ]; then
  BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
  BASE_DIR="$(dirname "$0")/../.."
fi

export CONFIG_DIR="$BASE_DIR/config"
FIFO_DIR="$BASE_DIR/fifo"
LOG_DIR="$BASE_DIR/logs"
FIFO_TASKER="$FIFO_DIR/fifo_tasker_in"
FIFO_TERMUX="$FIFO_DIR/fifo_termux_in"
FIFO_RETURN="$FIFO_DIR/fifo_return"
LOG_FILE="$LOG_DIR/vpiv_parser.log"

MQTT_BROKER="robotz-vincent.duckdns.org"
MQTT_TOPIC="SE/commande"

# --- FONCTIONS ---
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [VPIV] $1" >> "$LOG_FILE"
  echo "$1"
}

init_fifo() {
  mkdir -p "$FIFO_DIR"
  [ -p "$FIFO_TASKER" ] || mkfifo "$FIFO_TASKER"
  [ -p "$FIFO_TERMUX" ] || mkfifo "$FIFO_TERMUX"
  [ -p "$FIFO_RETURN" ] || mkfifo "$FIFO_RETURN"
}

validate_vpiv() {
  [[ "$1" =~ ^(.):([^:]+):([^:]+):([^:]+):(.*)#$ ]]
}

# --- TRAITEMENT DES COMMANDES ---
process_command() {
  local msg="$1"
  local CAT="${BASH_REMATCH[1]}"
  local VAR="${BASH_REMATCH[2]}"
  local PROP="${BASH_REMATCH[3]}"
  local INST="${BASH_REMATCH[4]}"
  local VAL="${BASH_REMATCH[5]}"

  log "Trame reçue : $CAT:$VAR:$PROP:$INST:$VAL"

  # 1. RÉACTION AUX ÉTATS D'URGENCE (POINT C)
  if [[ "$VAR" == "Urg" ]]; then
    # Si le SP confirme un état de danger
    if [[ "$CAT" == "E" && "$PROP" == "etat" && "$VAL" == "danger" ]]; then
      log "!!! ALERTE DANGER REÇUE : Lancement safety_stop.sh !!!"
      bash "$BASE_DIR/scripts/core/safety_stop.sh"
    fi
    # Envoi systématique de l'ACK vers le flux de retour
    echo "$I:Urg:$PROP:SE:$VAL#" > "$FIFO_TERMUX"
  fi

  # 2. GESTION DU RETOUR VOCAL (SELON TYPE_PTGE)
  local ptge_mode
  ptge_mode=$(jq -r '.CfgS.typePtge // 0' "$CONFIG_DIR/global.json")
  
  if [ "$ptge_mode" -ne 0 ]; then
    # Feedback COM (Info/Erreur)
    if [[ "$VAR" == "COM" ]]; then
      local clean_msg
      clean_msg=$(echo "$VAL" | tr -d '"')
      [[ "$PROP" == "error" ]] && termux-tts-speak "Erreur : $clean_msg"
      [[ "$PROP" == "info" ]] && termux-tts-speak "$clean_msg"
    fi

    # Feedback Urgence Vocal
    if [[ "$CAT" == "E" && "$VAR" == "Urg" ]]; then
      case "$VAL" in
        "URG_LOW_BAT") termux-tts-speak "Batterie critique." ;;
        "URG_CPU_CHARGE") termux-tts-speak "Surcharge processeur." ;;
        *) termux-tts-speak "Alerte sécurité." ;;
      esac
    fi
  fi

  # 3. AIGUILLAGE VERS LES GESTIONNAIRES
  case "$CAT" in
    "V"|"F") 
      case "$VAR" in
        "CamSE") bash "$BASE_DIR/scripts/sensors/cam/rz_camera_manager.sh" "$PROP" "$VAL" ;;
        "Appli"|"Zoom"|"TTS") echo "$msg" > "$FIFO_TASKER" ;;
        "CfgS") 
            # Mise à jour et ACK via le state-manager (via FIFO)
            echo "$msg" > "$FIFO_TERMUX" 
            ;;
        *) echo "$msg" > "$FIFO_TERMUX" ;;
      esac
      ;;
    "A") 
      if [[ "$VAR" == "Voice" && "$PROP" == "tts" ]]; then
        termux-tts-speak "$(echo "$VAL" | tr -d '"')"
      else
        echo "$msg" > "$FIFO_TASKER"
      fi
      ;;
    "I"|"E") 
      echo "$msg" > "$FIFO_TERMUX"
      ;;
  esac
}

# --- BOUCLE PRINCIPALE ---
main() {
  log "Démarrage du parser VPIV"
  init_fifo

  while true; do
    mosquitto_sub -h "$MQTT_BROKER" -p 1883 -t "$MQTT_TOPIC" -C 1 | while read -r msg; do
      if validate_vpiv "$msg"; then
        process_command "$msg"
        # Confirmation de réception au SP (Optionnel, utile pour debug MQTT)
        local short_msg
        short_msg=$(echo "$msg" | cut -d':' -f1-4)
        mosquitto_pub -h "$MQTT_BROKER" -t "SE/statut" -m "\$I:COM:info:SE:ACK_$short_msg#" -q 0 >/dev/null 2>&1
      else
        mosquitto_pub -h "$MQTT_BROKER" -t "SE/statut" -m "\$E:COM:error:SE:Trame_Invalide#" -q 0 >/dev/null 2>&1
      fi
    done
    sleep 0.5
  done
}

main