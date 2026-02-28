#!/bin/bash
# =============================================================================
# SCRIPT: rz_microSe_manager.sh
# CHEMIN:
#   - PC: PROJETRZ/termux/scripts/sensors/mic/rz_microSe_manager.sh
#   - Smartphone: ~/scripts_RZ_SE/termux/scripts/sensors/mic/rz_microSe_manager.sh
# DESCRIPTION:
#   Gestion centrale du microphone (MicroSE) avec machine d'état
#   1. Surveillance du niveau sonore
#   2. Gestion des modes (VEILLE, CONVERSATION, etc.)
#   3. Déclenchement du balayage sonore
#   4. Communication MQTT
#
# DEPENDANCES:
#   - jq
#   - termux-microphone-record
#   - ffmpeg
#
# AUTEUR: Vincent Philippe
# VERSION: 1.1
# DATE: 2026-02-06
# =============================================================================

# Configuration
MQTT_BROKER="robotz-vincent.duckdns.org"
CONFIG_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/config/sensors/microse_config.json"
LOG_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/microse_manager.log"
FIFO_IN="/data/data/com.termux/files/home/scripts_RZ_SE/fifo_microse_in"
LOCK_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/microse.lock"

# Fonctions utilitaires
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [MICRO] $1" >> "$LOG_FILE"
  echo "$1"
}

acquire_lock() {
  while [ -f "$LOCK_FILE" ]; do
    log "Attente du verrou (détenu par $(cat "$LOCK_FILE"))"
    sleep 0.1
  done
  echo $$ > "$LOCK_FILE"
  log "Verrou acquis (PID=$$)"
}

release_lock() {
  rm -f "$LOCK_FILE"
  log "Verrou libéré"
}

read_rms() {
  termux-microphone-record -d 0.1 -f /tmp/temp.wav 2>/dev/null | \
  ffmpeg -i - -af "astats=measure_perchannel=rms:reset=1" -f null - 2>&1 | \
  grep "RMS level" | awk '{print $3*1000}' | awk '{print int($1)}'
}

update_config() {
  local query="$1"
  jq "$query" "$CONFIG_FILE" > "$CONFIG_FILE.tmp" && mv "$CONFIG_FILE.tmp" "$CONFIG_FILE"
  log "Config mise à jour: $query"
}

# Machine d'état corrigée
handle_state() {
  acquire_lock
  local MODE_ACTUEL
  local NIVEAU_ACTUEL
  local PARAMS

  MODE_ACTUEL=$(jq -r '.MicroSE.modeMicro' "$CONFIG_FILE")
  NIVEAU_ACTUEL=$(read_rms)
  PARAMS=$(jq '.MicroSE.paraMicroSE' "$CONFIG_FILE")

  log "État actuel: $MODE_ACTUEL | Niveau sonore: $NIVEAU_ACTUEL"

 case "$MODE_ACTUEL" in
  "VEILLE")
    local SEUIL
    SEUIL=$(jq -r '.seuilLocalisation' <<< "$PARAMS")
    if [ "$NIVEAU_ACTUEL" -gt "$SEUIL" ]; then
      update_config '.MicroSE.modeMicro = "NIVEAU"'
      mosquitto_pub -h "$MQTT_BROKER" -t "SE/statut" -m "\$F:MicroSE:modeMicro:*:NIVEAU#"
      log "Transition VEILLE → NIVEAU (niveau=$NIVEAU_ACTUEL)"
    fi
    ;;
  "NIVEAU")
    local SEUIL
    SEUIL=$(jq -r '.seuilLocalisation' <<< "$PARAMS")
    if [ "$NIVEAU_ACTUEL" -gt "$SEUIL" ]; then
      update_config '.MicroSE.modeMicro = "LOCALISATION"'
      mosquitto_pub -h "$MQTT_BROKER" -t "SE/statut" -m "\$F:MicroSE:modeMicro:*:LOCALISATION#"
      log "Transition NIVEAU → LOCALISATION"
      nohup ./rz_balayage_sonore.sh > /data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/balayage.log 2>&1 &
    fi
    ;;
    "LOCALISATION"|"SURVEILLANCE"|"ALERTE")
      # Géré par les scripts dédiés
      ;;
  esac
  release_lock
}

# Boucle principale
main() {
  log "Démarrage de rz_microSe_manager.sh (PID=$$)"

  # Vérification du fichier de config
  if [ ! -f "$CONFIG_FILE" ]; then
    log "ERREUR: Fichier de configuration manquant"
    exit 1
  fi

  # Initialisation des FIFO
  mkdir -p "/data/data/com.termux/files/home/scripts_RZ_SE"
  [ -p "$FIFO_IN" ] || mkfifo "$FIFO_IN"

  while true; do
    # Écoute MQTT en arrière-plan
    mosquitto_sub -h "$MQTT_BROKER" -t "SE/commande" -q 1 | while read -r msg; do
      if [[ "$msg" =~ ^\$V:MicroSE: ]]; then
        acquire_lock
        echo "$msg" > "$FIFO_IN"
        log "Commande MQTT reçue: $msg"
        release_lock
      fi
    done &

    # Traitement des commandes FIFO
    if read -r cmd < "$FIFO_IN"; then
      acquire_lock
      case "$cmd" in
        *modeMicro*)
          NEW_MODE=$(echo "$cmd" | cut -d':' -f4)
          update_config ".MicroSE.modeMicro = \"$NEW_MODE\""
          mosquitto_pub -h "$MQTT_BROKER" -t "SE/statut" -m "\$I:MicroSE:modeMicro:*:$NEW_MODE#"
          log "Mode mis à jour: $NEW_MODE"
          ;;
        *mesurePonctuelle*)
          nohup ./rz_mesure_ponctuelle.sh > /data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/mesure_ponctuelle.log 2>&1 &
          log "Mesure ponctuelle lancée en arrière-plan"
          ;;
        *demandeBalayage*)
          nohup ./rz_balayage_sonore.sh > /data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/balayage.log 2>&1 &
          log "Balayage sonore lancé en arrière-plan"
          ;;
      esac
      release_lock
    fi

    # Gestion de la machine d'état
    handle_state

    # Calcul du délai adapté au mode
    local FREQ
    FREQ=$(jq -r '.MicroSE.paraMicroSE | if .modeMicro == "VEILLE" then .frequenceVeille else .frequenceSurveillance end' "$CONFIG_FILE")
    local SLEEP_TIME
    SLEEP_TIME=$(echo "1 / $FREQ" | bc)
    sleep "$SLEEP_TIME"
  done
}

main
