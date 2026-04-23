#!/bin/bash
# =============================================================================
# SCRIPT  : fifo_manager.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/utils/fifo_manager.sh
#
# OBJECTIF
# --------
# Gestion centralisée des files FIFO inter-processus du projet RZ.
# Crée, nettoie ou vérifie les 4 pipes nommées nécessaires au fonctionnement.
#
# DESCRIPTION DES FIFO
# --------------------
#   fifo_tasker_in  : commandes Tasker → Termux (apps Android)
#   fifo_termux_in  : trames VPIV → rz_vpiv_parser.sh (point d'entrée principal)
#   fifo_return     : retours Termux → Tasker (ACK, bascule IA exit 0/1)
#   fifo_appli_in   : commandes CAT=A → rz_appli_manager.sh
#
# UTILISATION
# -----------
#   bash fifo_manager.sh create   → crée les FIFO manquantes (idempotent)
#   bash fifo_manager.sh clean    → supprime toutes les FIFO
#   bash fifo_manager.sh check    → vérifie que toutes les FIFO existent
#
# ARTICULATION
# ------------
#   Appelé par  : original_init.sh au premier démarrage
#   Utilisé par : rz_vpiv_parser.sh, rz_stt_handler.sh, rz_appli_manager.sh,
#                 rz_state-manager.sh, rz_sys_monitor.sh
#                 (tous écrivent dans fifo_termux_in ou fifo_appli_in)
#
# PRÉCAUTIONS
# -----------
#   - Lancer AVANT tous les managers (les FIFO doivent exister avant toute écriture).
#   - Si un fichier ordinaire porte le même nom qu'une FIFO attendue,
#     il est automatiquement supprimé et remplacé par une pipe nommée.
#   - chmod 666 : nécessaire pour que Tasker (processus Android distinct) puisse écrire.
#
# DÉPENDANCES
# -----------
#   mkfifo  : disponible dans Termux par défaut
#
# AUTEUR  : Vincent Philippe
# VERSION : 3.0  (fusion : détection chemin dynamique + fifo_appli_in + correction pipe)
# DATE    : 2026-03-11
# =============================================================================

# =============================================================================
# DÉTECTION DYNAMIQUE DU CHEMIN
# =============================================================================

if [ -d "/data/data/com.termux/files" ]; then
    # Chemin smartphone (Termux)
    BASE_DIR="/data/data/com.termux/files/home/robotRZ-smartSE/termux"
else
    # Chemin PC — calculé depuis l'emplacement du script
    # Le script est dans termux/scripts/utils/ → remonter de 3 niveaux
    BASE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

FIFO_DIR="$BASE_DIR/fifo"

# Liste de toutes les FIFO du projet
FIFOS=(
    "fifo_tasker_in"
    "fifo_termux_in"
    "fifo_return"
    "fifo_appli_in"
)

# =============================================================================
# FONCTIONS
# =============================================================================

# --- Création des FIFO (idempotent) ---
create_fifos() {
    echo "Initialisation des FIFOs dans : $FIFO_DIR"
    mkdir -p "$FIFO_DIR"

    for f in "${FIFOS[@]}"; do
        local cible="$FIFO_DIR/$f"

        if [ -e "$cible" ] && [ ! -p "$cible" ]; then
            # Existe mais n'est pas une pipe — correction automatique
            echo "[!] $f : fichier ordinaire détecté, remplacement par une pipe..."
            rm -f "$cible"
            mkfifo "$cible"
            echo "[ok] $f recréé comme pipe nommée."
        elif [ ! -e "$cible" ]; then
            mkfifo "$cible"
            echo "[ok] $f créé."
        else
            echo "[=]  $f déjà présent."
        fi

        chmod 666 "$cible"
    done

    echo "Toutes les FIFOs sont prêtes."
}

# --- Nettoyage des FIFO ---
clean_fifos() {
    echo "Nettoyage des FIFOs dans : $FIFO_DIR"
    rm -f "$FIFO_DIR"/fifo_*
    echo "FIFOs supprimées."
}

# --- Vérification des FIFO ---
check_fifos() {
    local ok=true
    for f in "${FIFOS[@]}"; do
        if [ ! -p "$FIFO_DIR/$f" ]; then
            echo "[x] FIFO manquante ou invalide : $f"
            ok=false
        else
            echo "[ok] $f"
        fi
    done

    if [ "$ok" = true ]; then
        echo "Toutes les FIFOs sont OK."
        return 0
    else
        echo "Des FIFOs manquent — lancer : bash fifo_manager.sh create"
        return 1
    fi
}

# =============================================================================
# POINT D'ENTRÉE
# =============================================================================

case "$1" in
    create) create_fifos ;;
    clean)  clean_fifos  ;;
    check)  check_fifos  ;;
    *)
        echo "Usage : fifo_manager.sh {create|clean|check}"
        echo ""
        echo "  create  : crée les FIFOs manquantes (sans effacer les existantes)"
        echo "  clean   : supprime toutes les FIFOs"
        echo "  check   : vérifie que toutes les FIFOs existent et sont des pipes"
        ;;
esac