#!/bin/bash
# =============================================================================
# SCRIPT  : rz_torch_manager.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/torch/rz_torch_manager.sh
#
# OBJECTIF
# --------
# Gestionnaire de la torche (flash) du smartphone embarqué (SE).
# Bascule automatiquement entre mode natif (termux-torch) et mode caméra
# (API HTTP IP Webcam) selon l'état courant de la caméra.
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# Ce script est appelé à la demande par rz_vpiv_parser.sh (one-shot, pas daemon).
# Arguments : $1 = propriété VPIV (seul "active" supporté en v1)
#             $2 = valeur ("On" ou "Off")
#
# LOGIQUE DE BASCULE
# ------------------
# 1. Lit cam_config.json pour connaître l'état de la caméra.
# 2. Si caméra active (IP Webcam en cours) :
#      → Commande via API HTTP : enablels (On) ou disablels (Off)
#      → Appel curl asynchrone pour ne pas bloquer le manager.
# 3. Si caméra inactive :
#      → Commande via termux-torch on/off (mode natif Android).
# 4. Dans tous les cas : envoi ACK VPIV $I:TorchSE:active:SE:<état>#
# 5. En cas d'erreur : envoi $I:COM:error:SE:"..."# vers SP (via FIFO).
#
# TABLE A — VPIV PRODUITS
# -----------------------
#   SP → SE :
#     $V:TorchSE:active:*:On#    Demande allumage torche
#     $V:TorchSE:active:*:Off#   Demande extinction torche
#
#   SE → SP :
#     $I:TorchSE:active:SE:On#   ACK allumage effectué
#     $I:TorchSE:active:SE:Off#  ACK extinction effectuée
#     $I:COM:error:SE:"TorchSE : termux-torch indisponible"#
#     $I:COM:error:SE:"TorchSE : API caméra inaccessible (http://IP:8080/)"#
#     $I:COM:error:SE:"TorchSE : cam_config.json illisible ou absent"#
#
# INTERACTIONS
# ------------
#   Appelé par  : rz_vpiv_parser.sh (sur réception $V:TorchSE:active:*:<val>#)
#   Lit         : config/sensors/cam_config.json (état caméra)
#   Écrit dans  : fifo/fifo_termux_in → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# - termux-torch nécessite le package termux-api (pkg install termux-api).
# - L'appel curl est asynchrone (& en arrière-plan) pour ne pas bloquer.
# - Le timeout curl est fixé à 3s pour éviter une attente longue si IP Webcam
#   n'est pas joignable.
# - ⚠️ Les trames VPIV commencent par \$ (dollar échappé, PAS une variable shell).
#
# DÉPENDANCES
# -----------
#   jq              : lecture cam_config.json
#   curl            : commande API IP Webcam
#   termux-torch    : commande torche native (pkg install termux-api)
#   ifconfig        : lecture IP locale (net-tools ou iproute2)
#   fifo_termux_in  : créé par fifo_manager.sh
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.1  (VAR Torch → TorchSE — cohérence Table A v3 et rz_vpiv_parser.sh)
# DATE    : 2026-03-12
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
else
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

CONFIG_CAM="$BASE_DIR/config/sensors/cam_config.json"
FIFO_OUT="$BASE_DIR/fifo/fifo_termux_in"
LOG_FILE="$BASE_DIR/logs/torch.log"

# =============================================================================
# UTILITAIRES
# =============================================================================

log_torch() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [TORCH] $1" >> "$LOG_FILE"
}

# Envoi VPIV via FIFO (non-bloquant, watchdog 2s)
send_vpiv() {
    local trame="$1"
    if [ ! -p "$FIFO_OUT" ]; then
        log_torch "WARN : FIFO absente. Trame perdue : $trame"
        return 1
    fi
    echo "$trame" > "$FIFO_OUT" &
    local pid_write=$!
    sleep 2 && kill "$pid_write" 2>/dev/null &
    wait "$pid_write" 2>/dev/null
}

# Envoi erreur vers SP via COM:error
send_error() {
    local message="$1"
    log_torch "ERREUR : $message"
    send_vpiv "\$I:COM:error:SE:\"${message}\"#"
}

# =============================================================================
# FONCTION : set_torch
# ARGS     : $1 = état cible ("On" ou "Off")
# =============================================================================

set_torch() {
    local state="$1"
    local cam_active
    local ip_local
    local cmd
    local curl_url

    # --- Lecture état caméra dans cam_config.json ---
    if [ ! -f "$CONFIG_CAM" ]; then
        send_error "TorchSE : cam_config.json illisible ou absent ($CONFIG_CAM)"
        exit 1
    fi

    if ! cam_active=$(jq -r '.status.active // "Off"' "$CONFIG_CAM" 2>/dev/null); then
        send_error "TorchSE : cam_config.json illisible ou absent ($CONFIG_CAM)"
        exit 1
    fi

    # -----------------------------------------------------------------------
    # MODE CAMÉRA ACTIVE : commande via API HTTP IP Webcam
    # -----------------------------------------------------------------------
    if [ "$cam_active" == 1 ]; then
        log_torch "Mode Caméra actif : commande via API HTTP."

        ip_local=$(ifconfig wlan0 2>/dev/null | awk '/inet /{print $2}')
        if [ -z "$ip_local" ]; then
            # Fallback : essai avec ip addr si ifconfig absent
            ip_local=$(ip addr show wlan0 2>/dev/null | awk '/inet /{gsub(/\/.*/, "", $2); print $2}')
        fi

        if [ -z "$ip_local" ]; then
            send_error "TorchSE : impossible de lire l'adresse IP wlan0"
            exit 1
        fi

        cmd="disablels"
        [[ "$state" == "On" ]] && cmd="enablels"

        curl_url="http://${ip_local}:8080/${cmd}"
        log_torch "Appel API : $curl_url"

        # Appel asynchrone avec timeout 3s pour ne pas bloquer le manager
        curl -s --max-time 3 "$curl_url" > /dev/null &
        local pid_curl=$!

        # Vérification résultat curl (attente max 4s)
        sleep 4
        if kill -0 "$pid_curl" 2>/dev/null; then
            # curl encore en cours après 4s = timeout dépassé
            kill "$pid_curl" 2>/dev/null
            send_error "TorchSE : API caméra inaccessible (http://${ip_local}:8080/)"
            exit 1
        fi

    # -----------------------------------------------------------------------
    # MODE AUTONOME : commande via termux-torch (natif Android)
    # -----------------------------------------------------------------------
    else
        log_torch "Mode Autonome : commande via termux-torch."

        if ! command -v termux-torch &>/dev/null; then
            send_error "TorchSE : termux-torch indisponible (pkg install termux-api)"
            exit 1
        fi

        if [ "$state" == "On" ]; then
            termux-torch on
        else
            termux-torch off
        fi
    fi

    # -----------------------------------------------------------------------
    # ACK VPIV → SP (Table A : $I:TorchSE:active:SE:<état>#)
    # ⚠️ \$ obligatoire — $I serait une variable shell vide (trame corrompue)
    # -----------------------------------------------------------------------
    send_vpiv "\$I:TorchSE:active:SE:${state}#"
    log_torch "Torche → ${state}  ACK envoyé."
}

# =============================================================================
# AIGUILLAGE PROPRIÉTÉS VPIV
# =============================================================================

case "$1" in
    "active")
        # Validation valeur
        if [[ "$2" != "On" && "$2" != "Off" ]]; then
            log_torch "ERREUR : valeur '$2' invalide pour active (attendu : On|Off)"
            send_error "TorchSE : valeur active invalide : '$2' (attendu : On|Off)"
            exit 1
        fi
        set_torch "$2"
        ;;
    *)
        log_torch "Propriété inconnue ou non supportée en v1 : '$1'"
        send_error "TorchSE : propriété inconnue : '$1'"
        exit 1
        ;;
esac