#!/bin/bash
# =============================================================================
# SCRIPT: rz_mg_manager.sh
# DESCRIPTION: Gestion complète du magnétomètre avec modes multiples
# CONFIG: /data/data/com.termux/files/home/scripts_RZ_SErz_vars.json
# =============================================================================

CONFIG_FILE="/data/data/com.termux/files/home/scripts_RZ_SErz_vars.json"
LOG_FILE="/data/data/com.termux/files/home/scripts_RZ_SE/termux/logs/mg.log"
MQTT_BROKER="robotz-vincent.duckdns.org"

# --- Fonctions ---
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

send_mqtt() {
  mosquitto_pub -h "$MQTT_BROKER" -t "SEtoP" -m "$1"
}

read_config() {
  jq -r "$1" "$CONFIG_FILE"
}

# --- Initialisation ---
update_config() {
  MODE=$(read_config '.Mag.modeMg')
  FREQ=$(read_config '.Mag.paraMg.frequence')
  log "Config: mode=$MODE, fréquence=$FREQ Hz"
}

# --- Traitement principal ---
process_mg() {
  update_config

  while true; do
    CURRENT_MODE=$(read_config '.Mag.modeMg')

    if [ "$CURRENT_MODE" != "$MODE" ]; then
      log "Changement de mode: $MODE → $CURRENT_MODE"
      update_config
    fi

    case "$CURRENT_MODE" in
      "OFF")
        log "Mode OFF: Attente"
        sleep 5
        continue
        ;;

      "BRUT")
        # Lecture des données brutes
        if command -v termux-sensor &> /dev/null; then
          RAW_DATA=$(termux-sensor -s "MagneticField" -n 1 2>/dev/null)
          if [ $? -eq 0 ]; then
            X=$(echo "$RAW_DATA" | jq -r '.x * 100 | floor')
            Y=$(echo "$RAW_DATA" | jq -r '.y * 100 | floor')
            Z=$(echo "$RAW_DATA" | jq -r '.z * 100 | floor')
            send_mqtt "\$F:Mag:dataBrut:*:[$X,$Y,$Z]#"
          fi
        else
          log "ERREUR: termux-sensor non disponible"
        fi
        sleep $((1000/FREQ))
        ;;

      "CAPMAG")
        # Calcul du cap magnétique
        if command -v termux-sensor &> /dev/null; then
          RAW_DATA=$(termux-sensor -s "MagneticField" -n 1 2>/dev/null)
          if [ $? -eq 0 ]; then
            # Calcul simplifié du cap (0-3600 degrés×10)
            CAP=$(echo "$RAW_DATA" | jq -r '.x * 100 | floor')
            send_mqtt "\$F:Mag:dataMg:*:$CAP#"
          fi
        fi
        sleep $((1000/FREQ))
        ;;

      "CAPGEO")
        # Calcul du cap géographique (nécessite calibration)
        if command -v termux-sensor &> /dev/null; then
          RAW_DATA=$(termux-sensor -s "MagneticField" -n 1 2>/dev/null)
          if [ $? -eq 0 ]; then
            # Calcul simplifié avec calibration
            CAP=$(echo "$RAW_DATA" | jq -r '.x * 100 | floor')
            send_mqtt "\$F:Mag:dataGeo:*:$CAP#"
          fi
        fi
        sleep $((1000/FREQ))
        ;;
    esac
  done
}

# --- Démarrage ---
log "Démarrage du gestionnaire MG"
process_mg
