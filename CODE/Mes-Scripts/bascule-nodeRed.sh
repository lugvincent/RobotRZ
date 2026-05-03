#!/data/data/com.termux/files/usr/bin/bash

LOGFILE=$HOME/node-red.log
URL="http://$(ip -4 addr show wlan0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}'):1880/dashboard/accueil"

# Vérifie si Node-RED est déjà lancé
if pgrep -f "node-red" > /dev/null; then
    # Arrêt de Node-RED
    pkill -f node-red
    termux-wake-unlock
    termux-notification-remove node-red
    termux-notification --id node-red --title "Node-RED" --content "Arrêté ✅"
else
    # Démarrage de Node-RED
    nohup node-red > "$LOGFILE" 2>&1 &
    termux-wake-lock
    
    # Notification persistante avec deux boutons
    termux-notification \
      --id node-red \
      --title "Node-RED" \
      --content "En cours d'exécution 🚀" \
      --button1-text "STOP" \
      --button1-action "pkill -f node-red && termux-wake-unlock && termux-notification-remove node-red" \
      --button2-text "Ouvrir" \
      --button2-action "termux-open-url $URL" \
      --priority high \
      --ongoing true

    # Ouvre directement ton dashboard au démarrage
    termux-open-url "$URL"
fi
