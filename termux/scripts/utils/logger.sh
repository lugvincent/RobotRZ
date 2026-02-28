#!/bin/bash
# =============================================================================
# SCRIPT: logger.sh
# CHEMIN: termux/utils/logger.sh
# DESCRIPTION:
#   Fonctions de journalisation avancées pour le projet RZ
# =============================================================================

# Chemins
LOG_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux/logs"

# Niveaux de log
LOG_LEVEL=3  # 1:ERREUR, 2:AVERT, 3:INFO, 4:DEBUG

# Fonction principale
log() {
  local level="$1"
  local message="$2"
  local timestamp
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')  # Déclaration séparée
  local log_file="$LOG_DIR/${3:-main}.log"

  case "$level" in
    "ERREUR") [ $LOG_LEVEL -ge 1 ] && echo "[$timestamp] [ERR] $message" >> "$log_file" ;;
    "AVERT") [ $LOG_LEVEL -ge 2 ] && echo "[$timestamp] [WARN] $message" >> "$log_file" ;;
    "INFO") [ $LOG_LEVEL -ge 3 ] && echo "[$timestamp] [INFO] $message" >> "$log_file" ;;
    "DEBUG") [ $LOG_LEVEL -ge 4 ] && echo "[$timestamp] [DBG] $message" >> "$log_file" ;;
  esac

  echo "$message"
}

# Fonctions spécialisées
log_error() { log "ERREUR" "$1" "error.log"; }
log_warn() { log "AVERT" "$1" "warnings.log"; }
log_info() { log "INFO" "$1" "main.log"; }
log_debug() { log "DEBUG" "$1" "debug.log"; }

# Initialisation
init_logs() {
  mkdir -p "$LOG_DIR"
  touch "$LOG_DIR/main.log" "$LOG_DIR/error.log" "$LOG_DIR/warnings.log" "$LOG_DIR/debug.log"
}

init_logs
