#!/bin/bash
# =============================================================================
# SCRIPT: rz_balayage_sonore.sh
# DESCRIPTION: Balayage sonore pour localisation
# =============================================================================

CONFIG_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/config/sensors/microse_config.json"
LOG_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/balayage.log"
MQTT_BROKER="robotz-vincent.duckdns.org"

# Fonctions
log() {
  echo "[\$(date '+%Y-%m-%d %H:%M:%S')] [BALAYAGE] \$1" >> "\$LOG_FILE"
}

read_rms() {
  termux-microphone-record -d 0.1 -f /tmp/temp.wav 2>/dev/null | \
  ffmpeg -i - -af "astats=measure_perchannel=rms:reset=1" -f null - 2>&1 | \
  grep "RMS level" | awk '{print \$3*1000}' | awk '{print int(\$1)}'
}

# Boucle de balayage
main() {
  log "Début du balayage sonore"

  # Charger paramètres
  local PAS_BALAYAGE=\$(jq -r '.MicroSE.paraMicroSE.pasBalayage' "$CONFIG_FILE")
  local SERVO_TGD=$(jq -r '.MicroSE.paraMicroSE.servoTGD' "$CONFIG_FILE")
  local TIMEOUT=$(jq -r '.MicroSE.paraMicroSE.timeoutLocalisation' "$CONFIG_FILE")

  MAX_RMS=0
  MAX_ANGLE=0

  for ANGLE in $(seq -90 \$PAS_BALAYAGE 90); do
    # Positionner servo (simulé ici)
    log "Position servo: \$ANGLE°"

    # Mesure du niveau sonore
    RMS=\$(read_rms)
    log "RMS à \$ANGLE°: \$RMS"

    if [ \$RMS -gt \$MAX_RMS ]; then
      MAX_RMS=\$RMS
      MAX_ANGLE=\$ANGLE
    fi

    # Petit délai pour stabilisation
    sleep 0.2
  done

  # Envoi du résultat
  mosquitto_pub -h "\$MQTT_BROKER" -t "SE/statut" -m "\\$F:MicroSE:direction:*:\$MAX_ANGLE#"
  log "Direction optimale: \$MAX_ANGLE° (RMS=\$MAX_RMS)"

  # Retour au mode VEILLE
  jq '.MicroSE.modeMicro = "VEILLE"' "\$CONFIG_FILE" > "/tmp/microse_config.json" && \
  mv "/tmp/microse_config.json" "\$CONFIG_FILE"
  log "Retour au mode VEILLE"
}