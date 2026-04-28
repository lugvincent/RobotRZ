#!/bin/bash
MQTT_BROKER="robotz-vincent.duckdns.org"
TOPIC="SE/commande"

# Écoute les messages MQTT et agit en conséquence
mosquitto_sub -h "$MQTT_BROKER" -t "$TOPIC" | while read -r msg; do
  if [[ $msg =~ ^\$A:TTS:speak:\*\:(.*)#$ ]]; then
    text="${BASH_REMATCH[1]}"
    termux-tts-speak "$text"
    mosquitto_pub -h "$MQTT_BROKER" -t "SE/reponse" -m "\$I:TTS:status:*:spoken#"
  elif [[ $msg =~ ^\$A:Zoom:open:\*\:1#$ ]]; then
    am start -n com.android.zoom/.MainActivity
    mosquitto_pub -h "$MQTT_BROKER" -t "SE/reponse" -m "\$I:Zoom:status:*:opened#"
  fi
done

