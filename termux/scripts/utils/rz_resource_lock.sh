#!/bin/bash
# =============================================================================
# SCRIPT  : rz_resource_lock.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/utils/rz_resource_lock.sh
#
# OBJECTIF
# --------
# Gestion générique des verrous sur les ressources matérielles partagées du SE
# (caméra, microphone, ...). Empêche que deux applications/managers mobilisent
# simultanément la même ressource physique (ex : Zoom + IP Webcam Pro sur la
# caméra ; Zoom + MicroSE/STT sur le micro).
#
# Ce fichier n'est PAS exécuté directement : il est SOURCÉ (`source`) par les
# managers qui ont besoin de poser/lever/consulter un verrou.
#
# PRINCIPE
# --------
# Un seul fichier centralisé (resource_locks.json) répertorie l'état de
# chaque ressource physique connue. Format :
#   {
#     "camera":     { "locked_by": null, "since": null },
#     "microphone": { "locked_by": null, "since": null }
#   }
# - locked_by = null  → ressource libre
# - locked_by = "zoom"|"ipwebcam"|"stt"|...  → nom de l'acteur détenteur
#
# RÈGLE D'OR
# ----------
# Seul l'acteur qui détient un verrou peut le lever (unlock_resource vérifie
# locked_by avant de libérer). Évite qu'un module libère par erreur le verrou
# posé par un autre.
#
# FONCTIONS EXPORTÉES
# --------------------
#   lock_resource   <resource> <owner>   → pose le verrou si libre
#                                           retour 0 = succès
#                                           retour 1 = déjà verrouillé (echo du détenteur sur stdout)
#   unlock_resource <resource> <owner>   → libère si <owner> est bien le détenteur
#                                           retour 0 = succès ou déjà libre
#                                           retour 1 = détenu par un autre acteur (refus)
#   check_resource  <resource>           → affiche le détenteur actuel ("null" si libre)
#                                           ne modifie jamais l'état
#
# TABLE A — VPIV PRODUIT (par les appelants, pas par ce script)
# -----------------------------------------------------------------
#   Ce script ne publie AUCUNE trame VPIV lui-même. C'est aux managers
#   appelants (rz_camera_manager.sh, rz_appli_manager.sh, ...) de publier
#   $I:LockSE:<resource>:SE:<locked_by|libre>#  après un changement d'état,
#   et de gérer l'information utilisateur (COM:error) en cas de refus.
#
# ARTICULATION
# ------------
#   Sourcé par  : rz_camera_manager.sh, rz_appli_manager.sh, (futur) rz_microSe_manager.sh
#   Lit/écrit   : config/resource_locks.json
#
# PRÉCAUTIONS
# -----------
# - Toujours initialiser resource_locks.json avant le premier appel (init_locks_file
#   le fait automatiquement si le fichier est absent — ressources connues = libres).
# - lock_resource ÉCHOUE silencieusement côté state (n'écrit rien) si déjà verrouillé
#   par un AUTRE acteur — c'est à l'appelant de réagir (handle_error, COM:error, etc.)
# - Pas de verrou inter-process (pas de flock) : les managers SE sont des scripts
#   one-shot courts, le risque de race condition réelle est jugé négligeable à ce
#   stade. À revoir si un jour plusieurs managers tournent réellement en parallèle
#   sur la même ressource au même instant.
#
# DÉPENDANCES
# -----------
#   jq
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0
# DATE    : 2026-06-22
# =============================================================================

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================
# Respecte le même pattern que les autres scripts SE : si sourcé depuis un
# script qui a déjà défini BASE_DIR, on le réutilise ; sinon on le calcule.

if [ -z "$BASE_DIR" ]; then
    if [ -d "/data/data/com.termux/files" ]; then
        BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
    else
        BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
    fi
fi

RESOURCE_LOCKS_FILE="$BASE_DIR/config/resource_locks.json"

# Liste des ressources connues — utilisée uniquement pour l'initialisation
# du fichier s'il est absent. Ajouter ici toute nouvelle ressource physique
# partagée (ex: "gps" plus tard).
RZ_RESOURCE_LOCK_KNOWN_RESOURCES=("camera" "microphone")

# =============================================================================
# INITIALISATION — crée resource_locks.json s'il est absent
# Toutes les ressources connues démarrent "libres" (locked_by=null).
# =============================================================================

init_locks_file() {
    if [ -f "$RESOURCE_LOCKS_FILE" ]; then
        return 0
    fi

    mkdir -p "$(dirname "$RESOURCE_LOCKS_FILE")"

    local json="{"
    local first=true
    for res in "${RZ_RESOURCE_LOCK_KNOWN_RESOURCES[@]}"; do
        if [ "$first" = true ]; then
            first=false
        else
            json+=","
        fi
        json+="\"${res}\":{\"locked_by\":null,\"since\":null}"
    done
    json+="}"

    echo "$json" | jq '.' > "$RESOURCE_LOCKS_FILE" 2>/dev/null
}

# =============================================================================
# lock_resource <resource> <owner>
#
# Pose le verrou sur <resource> au nom de <owner>, SAUF si déjà détenu par
# un autre acteur.
#
# Retour 0 : verrou posé (ou déjà détenu par ce même <owner> — idempotent)
# Retour 1 : refus — stdout contient le nom du détenteur actuel
# =============================================================================

lock_resource() {
    local resource="$1"
    local owner="$2"

    if [ -z "$resource" ] || [ -z "$owner" ]; then
        echo "ERREUR lock_resource : resource et owner requis" >&2
        return 2
    fi

    init_locks_file

    local current_owner
    current_owner=$(jq -r --arg r "$resource" '.[$r].locked_by // "null"' "$RESOURCE_LOCKS_FILE" 2>/dev/null)

    # Déjà verrouillé par un AUTRE acteur → refus
    if [ "$current_owner" != "null" ] && [ "$current_owner" != "$owner" ]; then
        echo "$current_owner"
        return 1
    fi

    # Libre, ou déjà détenu par ce même owner (idempotent) → on pose/réaffirme le verrou
    local ts
    ts=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local tmp
    tmp=$(jq --arg r "$resource" --arg o "$owner" --arg ts "$ts" \
        '.[$r] = {"locked_by": $o, "since": $ts}' \
        "$RESOURCE_LOCKS_FILE")
    echo "$tmp" > "$RESOURCE_LOCKS_FILE"

    return 0
}

# =============================================================================
# unlock_resource <resource> <owner>
#
# Libère <resource> UNIQUEMENT si <owner> en est bien le détenteur actuel.
# Idempotent : libérer une ressource déjà libre ne fait rien (succès).
#
# Retour 0 : libéré (ou déjà libre)
# Retour 1 : refus — la ressource est détenue par un AUTRE acteur (stdout = ce détenteur)
# =============================================================================

unlock_resource() {
    local resource="$1"
    local owner="$2"

    if [ -z "$resource" ] || [ -z "$owner" ]; then
        echo "ERREUR unlock_resource : resource et owner requis" >&2
        return 2
    fi

    init_locks_file

    local current_owner
    current_owner=$(jq -r --arg r "$resource" '.[$r].locked_by // "null"' "$RESOURCE_LOCKS_FILE" 2>/dev/null)

    # Déjà libre → rien à faire
    if [ "$current_owner" == "null" ]; then
        return 0
    fi

    # Détenu par un AUTRE acteur → refus (protège contre une libération par erreur)
    if [ "$current_owner" != "$owner" ]; then
        echo "$current_owner"
        return 1
    fi

    # C'est bien notre verrou → on le lève
    local tmp
    tmp=$(jq --arg r "$resource" \
        '.[$r] = {"locked_by": null, "since": null}' \
        "$RESOURCE_LOCKS_FILE")
    echo "$tmp" > "$RESOURCE_LOCKS_FILE"

    return 0
}

# =============================================================================
# check_resource <resource>
#
# Affiche le détenteur actuel sur stdout ("null" si libre). Ne modifie rien.
# =============================================================================

check_resource() {
    local resource="$1"

    if [ -z "$resource" ]; then
        echo "ERREUR check_resource : resource requise" >&2
        return 2
    fi

    init_locks_file

    jq -r --arg r "$resource" '.[$r].locked_by // "null"' "$RESOURCE_LOCKS_FILE" 2>/dev/null
}