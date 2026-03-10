#!/bin/bash
# =============================================================================
# SCRIPT  : alerts_sys.sh
# CHEMIN  : ~/scripts_RZ_SE/termux/scripts/sensors/sys/lib/alerts_sys.sh
#
# OBJECTIF
# --------
# Bibliothèque de vérification des seuils d'alerte système.
# Fournit check_alert() générique pour les 5 métriques :
#   cpu, thermal, storage, mem  → alerte si valeur TROP HAUTE
#   batt                        → alerte si valeur TROP BASSE (sens inversé)
#
# UTILISATION
# -----------
# Ce fichier est SOURCÉ (source alerts_sys.sh), pas exécuté directement.
# Il hérite de l'environnement du script parent :
#   - FIFO_IN          : chemin du pipe nommé (défini dans rz_sys_monitor.sh)
#   - LOG_FILE         : chemin du log       (défini dans rz_sys_monitor.sh)
#   - trigger_urgence(): définie dans urgence.sh (sourcé avant ce fichier)
#
# MÉCANISME À DEUX SEUILS (Table A)
# ----------------------------------
# Chaque métrique dispose de deux seuils dans paraSys.thresholds :
#   seuil_warn : dépassé 10s → $I:COM:warn:SE:"..."#   (informatif, SP affiche)
#   seuil_urg  : dépassé 10s → $E:Urg:source:SE:URG#   (SE arrête émissions)
#
# Sens de déclenchement (paramètre $7) :
#   "haut" (défaut) : alerte si valeur > seuil  (cpu, thermal, storage, mem)
#   "bas"           : alerte si valeur < seuil  (batt)
#
# CODES URG PAR MÉTRIQUE
# ----------------------
#   cpu     → URG_CPU_CHARGE
#   thermal → URG_OVERHEAT
#   storage → URG_SENSOR_FAIL
#   mem     → URG_SENSOR_FAIL
#   batt    → URG_LOW_BAT
#
# FONCTIONS FOURNIES
# ------------------
#   init_alert_trackers              : initialise les tableaux (1 fois au démarrage)
#   check_alert <metrique> <val> <seuil_warn> <seuil_urg> <urg_code> <unite> [sens]
#
# ARTICULATION
# ------------
#   rz_sys_monitor.sh → source urgence.sh → source alerts_sys.sh
#   alerts_sys.sh → FIFO_IN → rz_vpiv_parser.sh → MQTT → SP
#
# PRÉCAUTIONS
# -----------
# - urgence.sh doit être sourcé AVANT ce fichier.
# - Ne jamais appeler mosquitto_pub ici.
# - Les tableaux associatifs sont globaux (persistants entre les cycles).
# - Pour batt : seuil_warn > seuil_urg (ex: warn=30%, urg=15%)
#   car on alerte quand la valeur DESCEND sous le seuil.
#
# AUTEUR  : Vincent Philippe
# VERSION : 4.0  (ajout sens "bas" pour batt, fonction générique à 7 args)
# DATE    : 2026-03-05
# =============================================================================

# Durée de dépassement avant déclenchement — commune warn et urg (secondes)
readonly ALERT_DURATION_S=10

# Tableaux de suivi par métrique (clé = cpu | thermal | storage | mem | batt)
declare -A alert_warn_since    # Timestamp premier dépassement seuil_warn (0 = inactif)
declare -A alert_warn_active   # true si COM:warn déjà envoyé
declare -A alert_urg_since     # Timestamp premier dépassement seuil_urg  (0 = inactif)
declare -A alert_urg_active    # true si urgence déjà déclenchée

# =============================================================================
# FONCTION : init_alert_trackers
# À appeler une seule fois au démarrage de rz_sys_monitor.sh.
# =============================================================================
init_alert_trackers() {
    for metrique in cpu thermal storage mem batt; do
        alert_warn_since[$metrique]=0
        alert_warn_active[$metrique]=false
        alert_urg_since[$metrique]=0
        alert_urg_active[$metrique]=false
    done
    echo "[ALERTS] Trackers initialisés : cpu thermal storage mem batt" >> "${LOG_FILE:-/dev/null}"
}

# =============================================================================
# UTILITAIRE INTERNE : envoi COM:warn via FIFO (non-bloquant, watchdog 2s)
# =============================================================================
_send_warn() {
    local message="$1"
    if [[ -p "$FIFO_IN" ]]; then
        echo "\$I:COM:warn:SE:\"${message}\"#" > "$FIFO_IN" &
        local pid_w=$!
        sleep 2 && kill "$pid_w" 2>/dev/null &
        wait "$pid_w" 2>/dev/null
    fi
    echo "[ALERTS] COM:warn → $message" >> "${LOG_FILE:-/dev/null}"
}

# =============================================================================
# UTILITAIRE INTERNE : teste si un seuil est franchi selon le sens
# $1 = valeur, $2 = seuil, $3 = sens ("haut" ou "bas")
# Retourne 0 (vrai) si dépassement, 1 (faux) sinon
# =============================================================================
_seuil_franchi() {
    local val="$1"
    local seuil="$2"
    local sens="$3"
    if [[ "$sens" == "bas" ]]; then
        (( val < seuil ))   # Alerte si valeur SOUS le seuil (batterie)
    else
        (( val > seuil ))   # Alerte si valeur AU-DESSUS du seuil (défaut)
    fi
}

# =============================================================================
# FONCTION : check_alert
#
# ARGS :
#   $1 = metrique   (cpu | thermal | storage | mem | batt)
#   $2 = valeur     (entier)
#   $3 = seuil_warn (entier)
#   $4 = seuil_urg  (entier)
#   $5 = urg_code   (ex: URG_CPU_CHARGE | URG_LOW_BAT)
#   $6 = unite      (% ou °C — pour le message COM:warn)
#   $7 = sens       (optionnel : "haut" défaut | "bas" pour batt)
#
# LOGIQUE :
#   Dépassement seuil_urg >= 10s  → trigger_urgence() (priorité absolue)
#   Dépassement seuil_warn >= 10s → COM:warn (si seuil_urg non atteint)
#   Retour sous les seuils        → remise à zéro des trackers
# =============================================================================
check_alert() {
    local metrique="$1"
    local val="$2"
    local seuil_warn="$3"
    local seuil_urg="$4"
    local urg_code="$5"
    local unite="$6"
    local sens="${7:-haut}"    # "haut" par défaut, "bas" pour batt
    local now
    now=$(date +%s)

    # -----------------------------------------------------------------------
    # CAS 1 : Dépassement seuil_urg (priorité)
    # -----------------------------------------------------------------------
    if _seuil_franchi "$val" "$seuil_urg" "$sens"; then

        if (( alert_urg_since[$metrique] == 0 )); then
            alert_urg_since[$metrique]=$now
            echo "[ALERTS] $metrique : seuil_urg ${seuil_urg}${unite} franchi (val=${val}, sens=$sens)" >> "${LOG_FILE:-/dev/null}"
        fi

        local duree=$(( now - alert_urg_since[$metrique] ))
        if (( duree >= ALERT_DURATION_S )) && [[ "${alert_urg_active[$metrique]}" == false ]]; then
            alert_urg_active[$metrique]=true
            trigger_urgence "$urg_code"
            echo "[ALERTS] URG $urg_code déclenché : $metrique ${val}${unite} depuis ${duree}s" >> "${LOG_FILE:-/dev/null}"
        fi

        # Tracker warn aussi actif pour cohérence
        (( alert_warn_since[$metrique] == 0 )) && alert_warn_since[$metrique]=$now
        return 0
    fi

    # -----------------------------------------------------------------------
    # CAS 2 : Dépassement seuil_warn uniquement
    # -----------------------------------------------------------------------
    if _seuil_franchi "$val" "$seuil_warn" "$sens"; then

        if (( alert_warn_since[$metrique] == 0 )); then
            alert_warn_since[$metrique]=$now
            echo "[ALERTS] $metrique : seuil_warn ${seuil_warn}${unite} franchi (val=${val})" >> "${LOG_FILE:-/dev/null}"
        fi

        local duree=$(( now - alert_warn_since[$metrique] ))
        if (( duree >= ALERT_DURATION_S )) && [[ "${alert_warn_active[$metrique]}" == false ]]; then
            alert_warn_active[$metrique]=true
            if [[ "$sens" == "bas" ]]; then
                _send_warn "${metrique} : ${val}${unite} (seuil warn : ${seuil_warn}${unite} — trop bas depuis ${duree}s)"
            else
                _send_warn "${metrique} : ${val}${unite} depuis ${duree}s (seuil warn : ${seuil_warn}${unite})"
            fi
        fi

        # Remise à zéro tracker urg (repassé sous seuil_urg)
        alert_urg_since[$metrique]=0
        alert_urg_active[$metrique]=false
        return 0
    fi

    # -----------------------------------------------------------------------
    # CAS 3 : Dans les limites normales → remise à zéro complète
    # -----------------------------------------------------------------------
    if [[ "${alert_warn_active[$metrique]}" == true || "${alert_urg_active[$metrique]}" == true ]]; then
        echo "[ALERTS] $metrique : retour normal (${val}${unite})" >> "${LOG_FILE:-/dev/null}"
    fi
    alert_warn_since[$metrique]=0
    alert_warn_active[$metrique]=false
    alert_urg_since[$metrique]=0
    alert_urg_active[$metrique]=false
}