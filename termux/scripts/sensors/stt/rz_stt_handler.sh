#!/bin/bash
# =============================================================================
# SCRIPT  : rz_stt_handler.sh
# CHEMIN  : /data/data/com.termux/files/home/robotRZ-smartSE/termux/scripts/sensors/stt/rz_stt_handler.sh
#
# OBJECTIF
# --------
#   Coeur decisionnel du traitement vocal.
#   1. Identifie la commande reconnue via le catalogue JSON (Table D).
#   2. Applique les filtres de securite (CPU, modeRZ, laisse).
#   3. Selon la colonne Traitement de la ligne trouvee :
#        - SIMPLE / LocalSE  -> resolu entierement ici (inchange depuis v2.6)
#        - TRADUCTION/CALCUL/MACRO -> transmis BRUT a SP, qui resout via le
#          mecanisme generique ui_Traitement (voir schema_mecanisme_
#          resolution_intents_V1.html)
#   4. Deploie les trames VPIV vers le circuit FIFO.
#   5. Gere le code de retour pour la bascule IA (exit 1 = non trouve).
#
# ARTICULATION
# ------------
#   - Catalogue lu   : stt_catalog.json (Table D operationnelle, copiee a
#     l'init depuis tableD_catalogueV2_corrige.csv)
#   - Appelant       : rz_autovoice_bridge.sh v3.0 (transmet TEXT_RAW et
#     VAL_NUMERIQUE apres arbitrage Cmde/Conv/Mixte)
#   - Cote reception : fn_stt_intent_router (Node-RED, tab RZ_GestionFlux_V3)
#     doit savoir distinguer une capsule a 5 champs (deja resolue) d'une
#     capsule a 6 champs (a resoudre) - voir ETAPE F ci-dessous.
#
# PRECAUTIONS
# -----------
#   - NE PAS ajouter de logique de calcul ou de traduction ici pour
#     TRADUCTION/CALCUL/MACRO : c'est desormais le role exclusif de SP
#     (decision actee le 06/07/2026 - eviter de dupliquer la logique).
#   - Le TTS de confirmation n'est PLUS joue de maniere optimiste pour
#     TRADUCTION/CALCUL/MACRO : on attend l'ACK de SP (wait_for_ack).
#   - id_requete sert UNIQUEMENT a correler la reponse SP - ce n'est pas
#     un identifiant de commande (CMD_ID reste interne a SE, ne voyage
#     jamais jusqu'a SP).
#   - DEPENDANCE CROISEE : wait_for_ack() ne fonctionne que si
#     rz_vpiv_parser.sh a ete mis a jour EN PARALLELE pour ecrire
#     $BASE_DIR/fifo/stt_ack/<id_requete>.ack a la reception d'un ACK
#     $I:STT:intent:<id_requete>:...# - les deux fichiers doivent etre
#     deployes ensemble, jamais l'un sans l'autre.
#
# VERSION : 3.1 (wait_for_ack() implemente - fichier de correlation par
#           requete, polling 200ms - depend de rz_vpiv_parser.sh a jour)
# DATE    : 07/07/2026
# AUTEUR  : Vincent Philippe / Claude
# =============================================================================

# --- 1. DEFINITION DES CHEMINS ---
BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
CONFIG_GLOBAL="$BASE_DIR/config/global.json"
CATALOGUE="$BASE_DIR/scripts/sensors/stt/stt_catalog.json"
LOG_FILE="$BASE_DIR/logs/stt_handler.log"
FIFO_IN="$BASE_DIR/fifo/fifo_termux_in"

# Duree d'attente max (secondes) d'un ACK SP avant abandon.
# ROLE : couvre l'aller-retour MQTT + le temps de parcours complet de la
# chaine de validation SP (Dispatcher->GestionUrgence->ReglesMinimales->
# ValidationIntent->Stabilisation), plus une marge pour la latence reseau
# (4G/DuckDNS notamment, plus lent que le Wifi local).
# VALEUR DE DEPART, non mesuree - a ajuster apres tests reels si besoin.
# Voir vademecum_ack_timeout_V1.html pour la liste des endroits a modifier
# si cette valeur doit changer.
ACK_TIMEOUT_SEC=4

# --- 2. RECUPERATION DES ARGUMENTS ---
TEXT_RAW="$1"
VAL_NUMERIQUE="$2"

# Journalisation
log_stt() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [HANDLER] $1" >> "$LOG_FILE"
}

# Expedition FIFO
send_to_system() {
    local trame="$1"
    log_stt "EXPEDITION VPIV: $trame"
    echo "$trame" > "$FIFO_IN"
}

# Genere un identifiant de correlation court et unique.
# NOTE : on evite `date +%s%N` (nanosecondes) car toybox/Termux ne le
# supporte pas de maniere fiable. epoch-secondes + PID + RANDOM suffit
# a eviter les collisions dans la fenetre d'un appel vocal.
generate_id_requete() {
    echo "req${RANDOM}_$$_$(date +%s)"
}

# --- 3. FONCTION DE TRAITEMENT D'UNE COMMANDE SIMPLE (INCHANGE depuis v2.6) ---
# Utilisee pour Traitement=SIMPLE et Traitement=LocalSE : la ligne Table D
# porte deja tout ce qu'il faut, aucune resolution externe necessaire.
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

    # Capsule a 5 champs (deja resolue) - format inchange depuis v2.6
    local inner_vpiv="${cat}:${var}:${prop}:${inst}:${final_val}"
    local capsule_trame="\$A:STT:intent:*:{${inner_vpiv}}#"

    send_to_system "$capsule_trame"

    # TTS OPTIMISTE CONSERVE ICI : SIMPLE/LocalSE n'ont qu'un seul intent,
    # deja entierement determine - pas de resolution risquee cote SP.
    local tts_list
    tts_list=$(echo "$data" | jq -r '.alias_tts | select(. != null) | join("|")')

    if [[ -n "$tts_list" ]]; then
        local tts_msg
        tts_msg=$(echo "$tts_list" | tr '|' '\n' | shuf -n 1)
        termux-tts-speak "$tts_msg"
    fi
}

# --- 4. NOUVELLE FONCTION : TRANSMISSION BRUTE (TRADUCTION/CALCUL/MACRO) ---
# Contrairement a process_simple(), cette fonction ne calcule RIEN : elle
# transmet la valeur captee telle quelle et laisse SP resoudre via le
# mecanisme ui_Traitement (fonction generique + parametres).
process_raw() {
    local cmd_id="$1"
    local val_brute="$2"    # mot capte (ex: "rouge") ou valeur numerique brute
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
    # Nouvelle colonne Table D : quelle fonction generique SP doit appeler.
    local id_traitement
    id_traitement=$(echo "$data" | jq -r '.id_traitement')

    if [[ -z "$id_traitement" || "$id_traitement" == "null" ]]; then
        log_stt "ERREUR: $cmd_id classe hors SIMPLE/LocalSE mais id_traitement absent de Table D."
        termux-tts-speak "Erreur de configuration."
        return 1
    fi

    local id_requete
    id_requete=$(generate_id_requete)

    # Capsule a 6 champs : id_traitement en PREMIER, pour que
    # fn_stt_intent_router puisse trancher sur la longueur du split
    # avant d'avoir a reinterpreter quoi que ce soit.
    local inner_vpiv="${id_traitement}:${cat}:${var}:${prop}:${inst}:${val_brute}"
    # id_requete porte par le champ INST de l'enveloppe externe (pas *)
    local capsule_trame="\$A:STT:intent:${id_requete}:{${inner_vpiv}}#"

    send_to_system "$capsule_trame"
    log_stt "ATTENTE ACK: id_requete=$id_requete (cmd_id=$cmd_id, id_traitement=$id_traitement)"

    local resultat
    resultat=$(wait_for_ack "$id_requete" "$ACK_TIMEOUT_SEC")

    case "$resultat" in
        OK)
            termux-tts-speak "D'accord."
            ;;
        TIMEOUT)
            log_stt "ACK TIMEOUT: id_requete=$id_requete"
            termux-tts-speak "Je n'ai pas de confirmation, reessaie."
            ;;
        FAIL*)
            log_stt "ACK REJET: id_requete=$id_requete ($resultat)"
            termux-tts-speak "Commande refusee."
            ;;
    esac
}

# Attend l'ACK correspondant à id_requete, publié par SP via SE/commande
# et écrit dans un fichier dédié par rz_vpiv_parser.sh (voir sa version
# à jour — modification requise en parallèle de celle-ci, cf en-tête).
# Contrat : renvoie "OK", "FAIL,<raison>" ou "TIMEOUT" sur stdout.
wait_for_ack() {
    local id_requete="$1"
    local timeout_sec="$2"
    local ack_dir="$BASE_DIR/fifo/stt_ack"
    local ack_file="$ack_dir/${id_requete}.ack"
    local interval="0.2"                     # secondes entre deux vérifications
    local max_iter=$((timeout_sec * 5))       # 5 = 1 / 0.2 (vérifications par seconde)

    mkdir -p "$ack_dir"

    local i=0
    while [ "$i" -lt "$max_iter" ]; do
        if [ -f "$ack_file" ]; then
            local contenu
            contenu=$(cat "$ack_file" 2>/dev/null)
            rm -f "$ack_file"
            echo "$contenu"
            return 0
        fi
        sleep "$interval"
        i=$((i + 1))
    done

    # Timeout atteint : nettoyage préventif si le fichier apparaît juste après
    rm -f "$ack_file" 2>/dev/null
    echo "TIMEOUT"
    return 1
}

# =============================================================================
# LOGIQUE PRINCIPALE
# =============================================================================

# --- ETAPE A : LECTURE DE L'ETAT DU ROBOT (inchange) ---
JSON_DATA=$(cat "$CONFIG_GLOBAL")

CPU_LOAD=$(echo "$JSON_DATA" | jq -r '.Sys.cpu_load // 0')
CPU_INT=${CPU_LOAD%.*}
MODE_RZ=$(echo "$JSON_DATA" | jq -r '.CfgS.modeRZ // 1')
TYPE_PTGE=$(echo "$JSON_DATA" | jq -r '.CfgS.typePtge // 0')

# --- ETAPE B : VERIFICATION SECURITE CPU (inchange) ---
if [ "$CPU_INT" -gt 90 ]; then
    log_stt "ALERTE CPU: $CPU_LOAD%. Abandon."
    termux-tts-speak "Je suis trop charge."
    exit 0
fi

# --- ETAPE C : IDENTIFICATION DE LA COMMANDE (inchange) ---
CLEAN_TEXT=$(echo "$TEXT_RAW" | sed 's/^rz //g' | xargs)
CMD_DATA=$(jq -c --arg txt "$CLEAN_TEXT" '[.commandes[] | select(.synonymes[] == $txt)]| first' "$CATALOGUE")
CMD_ID=$(echo "$CMD_DATA" | jq -r '.id // empty')

# SI INCONNU : ON RENVOIE 1 POUR TENTER L'IA CLOUD
if [[ -z "$CMD_ID" || "$CMD_ID" == "null" ]]; then
    log_stt "INCONNU: '$CLEAN_TEXT' -> Signal bascule IA (exit 1)"
    exit 1
fi

# --- ETAPE D : FILTRAGE MODE RZ (0=ARRET, 5=ERREUR) (inchange) ---
if [[ "$MODE_RZ" == "0" || "$MODE_RZ" == "5" ]]; then
    if [[ "$CMD_ID" != "URGENCY_STOP" ]]; then
        log_stt "BLOQUE: ModeRZ $MODE_RZ actif. Refus de $CMD_ID"
        termux-tts-speak "Systeme indisponible."
        exit 0
    fi
fi

# --- ETAPE E : FILTRAGE LAISSE (TYPE_PTGE 4 = LAISSE+VOCALE) (inchange) ---
MOTEUR_ACTIF=$(echo "$CMD_DATA" | jq -r '.moteurActif // 0')

if [[ "$TYPE_PTGE" == "4" && "$MOTEUR_ACTIF" == "1" ]]; then
    if [[ "$CMD_ID" != *"STOP"* ]]; then
        log_stt "REJET: Laisse active, mouvement $CMD_ID interdit."
        termux-tts-speak "Mouvement bloque par la laisse."
        exit 0
    fi
fi

# --- ETAPE F : EXECUTION (AIGUILLAGE) ---
# Ancien aiguillage (v2.6) : SIMPLE|LOCAL -> process_simple ; COMPLEXE ->
# boucle process_simple sur id_fils (fait ICI, cote SE).
# Nouvel aiguillage (v3.0) : la boucle MACRO disparait de SE - elle migre
# entierement cote SP via le mecanisme generique ui_Traitement.
TRAITEMENT=$(echo "$CMD_DATA" | jq -r '.traitement')

case "$TRAITEMENT" in
    "SIMPLE" | "LocalSE")
        process_simple "$CMD_ID"
        ;;

    "TRADUCTION" | "CALCUL" | "MACRO")
        process_raw "$CMD_ID" "$VAL_NUMERIQUE"
        ;;

    *)
        log_stt "ERREUR: Traitement '$TRAITEMENT' inconnu pour $CMD_ID."
        termux-tts-speak "Erreur de configuration."
        ;;
esac

exit 0