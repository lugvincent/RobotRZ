#!/bin/bash
# =============================================================================
# SCRIPT: rz_stt_handler.sh
# CHEMIN: /data/data/com.termux/files/home/scripts_RZ_SE/termux/scripts/sensors/stt/rz_stt_handler.sh
# DESCRIPTION:
#      Cœur décisionnel du traitement vocal.
#      1. Analyse l'Intent reconnu localement via le catalogue JSON.
#      2. Applique les filtres de sécurité (CPU, modeRZ, Moteur).
#      3. Déploie les trames VPIV vers le circuit FIFO.
#      4. Gère le code de retour pour la bascule IA (exit 1 = non trouvé).
#
# VERSION: 2.5 (Intégration capsule_trame pour validation SP)
# DATE: 2026-02-16
# =============================================================================

# --- 1. DÉFINITION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/scripts_RZ_SE/termux"
CONFIG_GLOBAL="$BASE_DIR/config/global.json"
CATALOGUE="$BASE_DIR/config/sensors/stt_catalog.json"
LOG_FILE="$BASE_DIR/logs/stt_handler.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"

# --- 2. RÉCUPÉRATION DES ARGUMENTS ---
TEXT_RAW="$1"      
VAL_NUMERIQUE="$2" 

# Journalisation
log_stt() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [HANDLER] $1" >> "$LOG_FILE"
}

# Expédition FIFO
send_to_system() {
    local trame="$1"
    log_stt "EXPÉDITION VPIV: $trame"
    echo "$trame" > "$FIFO_IN"
}

# --- 3. FONCTION DE TRAITEMENT D'UNE COMMANDE SIMPLE ---
process_simple() {
    local cmd_id="$1"
    local data
    data=$(jq -c --arg id "$cmd_id" '.commandes[] | select(.id == $id)' "$CATALOGUE")
    
    local cat
    cat=$(echo "$data" | jq -r '.cat')
    local var
    var=$(echo "$data" | jq -r '.vpiv_parts.var')
    local prop
    prop=$(echo "$data" | jq -r '.vpiv_parts.prop')
    local inst
    inst=$(echo "$data" | jq -r '.vpiv_parts.inst')
    local val_defaut
    val_defaut=$(echo "$data" | jq -r '.vpiv_parts.val')

    local final_val="${VAL_NUMERIQUE:-$val_defaut}"
    
    # --- MODIFICATION : ENCAPSULATION POUR VALIDATION SP ---
    # On prépare la trame interne
    local inner_vpiv="${cat}:${var}:${prop}:${inst}:${final_val}"
    # On l'enveloppe dans une capsule d'intention CAT=A (Application)
    local capsule_trame="\$A:STT:intent:*:{${inner_vpiv}}#"
    
    send_to_system "$capsule_trame"

    # GESTION TTS
    local tts_list
    tts_list=$(echo "$data" | jq -r '.alias_tts | select(. != null) | join("|")')
    
    if [[ -n "$tts_list" ]]; then
        local tts_msg
        tts_msg=$(echo "$tts_list" | tr '|' '\n' | shuf -n 1)
        termux-tts-speak "$tts_msg"
    fi
}

# =============================================================================
# LOGIQUE PRINCIPALE
# =============================================================================

# --- ÉTAPE A : LECTURE DE L'ÉTAT DU ROBOT ---
JSON_DATA=$(cat "$CONFIG_GLOBAL")

CPU_LOAD=$(echo "$JSON_DATA" | jq -r '.Sys.cpu_load // 0')
CPU_INT=${CPU_LOAD%.*} 
MODE_RZ=$(echo "$JSON_DATA" | jq -r '.CfgS.modeRZ // 1')
TYPE_PTGE=$(echo "$JSON_DATA" | jq -r '.CfgS.typePtge // 0')

# --- ÉTAPE B : VÉRIFICATION SÉCURITÉ CPU ---
if [ "$CPU_INT" -gt 90 ]; then
    log_stt "ALERTE CPU: $CPU_LOAD%. Abandon."
    termux-tts-speak "Je suis trop chargé."
    exit 0 
fi

# --- ÉTAPE C : IDENTIFICATION DE LA COMMANDE ---
CLEAN_TEXT=$(echo "$TEXT_RAW" | sed 's/^rz //g' | xargs)
CMD_DATA=$(jq -c --arg txt "$CLEAN_TEXT" '.commandes[] | select(.synonymes[] == $txt)' "$CATALOGUE")
CMD_ID=$(echo "$CMD_DATA" | jq -r '.id // empty')

# SI INCONNU : ON RENVOIE 1 POUR TENTER L'IA CLOUD
if [[ -z "$CMD_ID" || "$CMD_ID" == "null" ]]; then
    log_stt "INCONNU: '$CLEAN_TEXT' -> Signal bascule IA (exit 1)"
    exit 1 
fi

# --- ÉTAPE D : FILTRAGE MODE RZ (0=ARRET, 5=ERREUR) ---
if [[ "$MODE_RZ" == "0" || "$MODE_RZ" == "5" ]]; then
    if [[ "$CMD_ID" != "URGENCY_STOP" ]]; then
        log_stt "BLOQUÉ: ModeRZ $MODE_RZ actif. Refus de $CMD_ID"
        termux-tts-speak "Système indisponible."
        exit 0
    fi
fi

# --- ÉTAPE E : FILTRAGE LAISSE (TYPE_PTGE 4 = LAISSE+VOCALE) ---
MOTEUR_ACTIF=$(echo "$CMD_DATA" | jq -r '.moteurActif // 0')

if [[ "$TYPE_PTGE" == "4" && "$MOTEUR_ACTIF" == "1" ]]; then
    if [[ "$CMD_ID" != *"STOP"* ]]; then
        log_stt "REJET: Laisse active, mouvement $CMD_ID interdit."
        termux-tts-speak "Mouvement bloqué par la laisse."
        exit 0
    fi
fi

# --- ÉTAPE F : EXÉCUTION (AIGUILLAGE) ---
TRAITEMENT=$(echo "$CMD_DATA" | jq -r '.traitement')

case "$TRAITEMENT" in
    "SIMPLE" | "LOCAL")
        process_simple "$CMD_ID"
        ;;
        
    "COMPLEXE")
        log_stt "MACRO: $CMD_ID"
        FILS=$(echo "$CMD_DATA" | jq -r '.id_fils[]')
        for f in $FILS; do
            process_simple "$f"
            sleep 0.2
        done
        ;;
esac

exit 0