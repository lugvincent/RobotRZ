#!/bin/bash
# =============================================================================
# SCRIPT: rz_network_init.sh (version finale)
# CHEMIN: ~/scripts_RZ_SE/termux/scripts/core/rz_network_init.sh
# =============================================================================

CONFIG_DIR="/sdcard/scripts_RZ_SE/termux/config"
LOG_FILE="/sdcard/rz_logs/network.log"

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [NET] $1" >> "$LOG_FILE"
  echo "$1"
}

test_broker_connection() {
  local ip="$1"
 local test_msg
 test_msg="ping_$(date +%s)"

  echo "$test_msg" | mosquitto_pub -h "$ip" -p 1883 \
                 -t "SE/ping" -I "SE_Test" -q 1 -W 3 >/dev/null 2>&1
  return $?
}

receive_large_config() {
  local ip="$1"
  local config_json=""
  local tmp_file
  tmp_file=$(mktemp)

  mosquitto_pub -h "$ip" -p 1883 \
                -t "SE/config/request" \
                -m "\$A:CfgS:request:*:init#" >/dev/null 2>&1

  # Lire dans un fichier temporaire pour éviter le sous-shell pipeline
  mosquitto_sub -h "$ip" -p 1883 \
                -t "SE/config/chunk" \
                -W 10 > "$tmp_file"

  while read -r line; do
    if [[ $line =~ ^\$F:CfgS:chunk:([0-9]+):(.*)#$ ]]; then
      config_json+="${BASH_REMATCH[2]}"
    fi
  done < "$tmp_file"

  rm -f "$tmp_file"
  echo "$config_json" | jq .
}

find_broker() {
  local local_ips=("192.168.1.37" "192.168.1.38")
  local public_ip="robotz-vincent.duckdns.org"

  for ip in "${local_ips[@]}"; do
    if test_broker_connection "$ip"; then
      BROKER_IP="$ip"
      return 0
    fi
  done

  if test_broker_connection "$public_ip"; then
    BROKER_IP="$public_ip"
    return 0
  fi

  return 1
}

sync_with_sp() {
  log "Envoi de message d'initialisation à SP"

  mosquitto_pub -h "$BROKER_IP" -p 1883 \
                -t "SE/statut" \
                -m "\$I:COM:warn:SE:En attente de configuration#" \
                -q 1 >/dev/null 2>&1

  CONFIG_JSON=$(receive_large_config "$BROKER_IP")

  if [ -n "$CONFIG_JSON" ]; then
    if jq -e . <<< "$CONFIG_JSON" >/dev/null && \
       diff -q <(jq -r 'keys' <<< "$CONFIG_JSON") \
              <(jq -r 'keys' "$CONFIG_DIR/original_init.json") >/dev/null; then

      echo "$CONFIG_JSON" > "$CONFIG_DIR/courant_init.json"

      mosquitto_pub -h "$BROKER_IP" -p 1883 \
                    -t "SE/statut" \
                    -m "\$I:COM:info:SE:Configuration chargée#" \
                    -q 1 >/dev/null 2>&1
      return 0
    fi
  fi

  return 1
}

main() {
  while true; do
    if find_broker; then
      if sync_with_sp; then
        break
      fi
    fi
    sleep 15
  done
}

main

