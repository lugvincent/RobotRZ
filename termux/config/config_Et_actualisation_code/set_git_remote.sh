#!/bin/bash
# =============================================================================
# SCRIPT  : set_git_remote.sh
# CHEMIN  : ~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/
#
# OBJECTIF
# --------
# Met à jour l'URL remote Git du dépôt RobotRZ en lisant le token GitHub
# depuis keys.json. À relancer après chaque renouvellement de token GitHub
# ou après un git clone sur une nouvelle machine/instance Termux.
#
# POURQUOI CE SCRIPT EXISTE
# -------------------------
# Le token GitHub est embarqué dans l'URL remote Git (méthode HTTPS Termux).
# La commande "git remote -v" affiche cette URL en clair, token inclus.
# ⚠️ RÈGLE ABSOLUE : ne JAMAIS coller le résultat de "git remote -v"
#    dans une conversation, un mail, ou tout autre canal externe.
#    Utiliser ce script pour toute mise à jour — jamais manuellement.
#
# UTILISATION
# -----------
#   cd ~/robotRZ-smartSE
#   bash termux/config/config_Et_actualisation_code/set_git_remote.sh
#
# Ce script peut aussi être lancé depuis n'importe où :
#   bash ~/robotRZ-smartSE/termux/config/config_Et_actualisation_code/set_git_remote.sh
#
# PRÉCAUTIONS
# -----------
# - keys.json doit exister et contenir GITHUB_USER et GITHUB_TOKEN
# - Ne jamais versionner keys.json (il est dans .gitignore)
# - Ne jamais afficher la valeur du token (vérification par longueur uniquement)
#
# ARTICULATION
# ------------
#   Lit    : termux/config/keys.json  (GITHUB_USER, GITHUB_TOKEN)
#   Modifie: remote "origin" du dépôt ~/robotRZ-smartSE/
#   Vérifie: git fetch (test authentification silencieux)
#
# DÉPENDANCES
# -----------
#   jq, git
#
# AUTEUR  : Vincent Philippe
# VERSION : 1.0
# DATE    : 2026-07-01
# =============================================================================

# =============================================================================
# CONFIGURATION DES CHEMINS
# =============================================================================

REPO_DIR="/data/data/com.termux/files/home/robotRZ-smartSE"
KEYS_FILE="$REPO_DIR/termux/config/keys.json"
GITHUB_REPO="lugvincent/RobotRZ"

# =============================================================================
# VÉRIFICATIONS PRÉALABLES
# =============================================================================

echo "[set_git_remote] Vérification de l'environnement..."

# Vérifier que jq est disponible
if ! command -v jq &>/dev/null; then
    echo "[ERREUR] jq n'est pas installé. Lancer : pkg install jq"
    exit 1
fi

# Vérifier que le dossier repo existe
if [ ! -d "$REPO_DIR/.git" ]; then
    echo "[ERREUR] Dépôt Git non trouvé dans : $REPO_DIR"
    echo "         Vérifier que git clone a bien été effectué."
    exit 1
fi

# Vérifier que keys.json existe
if [ ! -f "$KEYS_FILE" ]; then
    echo "[ERREUR] Fichier keys.json introuvable : $KEYS_FILE"
    echo "         Créer manuellement ce fichier (jamais dans Git)."
    echo "         Voir README.md dans ce dossier pour le format."
    exit 1
fi

# =============================================================================
# LECTURE DES CREDENTIALS (jamais affiché en clair)
# =============================================================================

GITHUB_USER=$(jq -r '.GITHUB_USER // empty' "$KEYS_FILE" 2>/dev/null)
GITHUB_TOKEN=$(jq -r '.GITHUB_TOKEN // empty' "$KEYS_FILE" 2>/dev/null)

# Vérification par longueur uniquement — jamais par valeur affichée
if [ -z "$GITHUB_USER" ]; then
    echo "[ERREUR] GITHUB_USER absent de keys.json"
    exit 1
fi

if [ ${#GITHUB_TOKEN} -lt 20 ]; then
    echo "[ERREUR] GITHUB_TOKEN absent ou trop court dans keys.json"
    echo "         Longueur attendue : 40+ caractères. Longueur lue : ${#GITHUB_TOKEN}"
    exit 1
fi

echo "[set_git_remote] Utilisateur GitHub : $GITHUB_USER"
echo "[set_git_remote] Token GitHub       : présent (longueur ${#GITHUB_TOKEN} car.) ✓"

# =============================================================================
# MISE À JOUR DU REMOTE
# =============================================================================

NEW_URL="https://${GITHUB_USER}:${GITHUB_TOKEN}@github.com/${GITHUB_REPO}"

echo "[set_git_remote] Mise à jour du remote 'origin'..."

# On évite d'afficher l'URL avec le token — on construit la commande
# et on vérifie uniquement par git remote get-url ensuite (tronqué)
git -C "$REPO_DIR" remote set-url origin "$NEW_URL"
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo "[ERREUR] Échec de git remote set-url (code $EXIT_CODE)"
    exit 1
fi

echo "[set_git_remote] Remote mis à jour. ✓"

# =============================================================================
# TEST D'AUTHENTIFICATION
# =============================================================================

echo "[set_git_remote] Test de connexion GitHub (git fetch)..."
git -C "$REPO_DIR" fetch --quiet 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "[set_git_remote] Authentification réussie. ✓"
    echo "[set_git_remote] Dépôt RobotRZ connecté et opérationnel."
else
    echo "[ERREUR] git fetch a échoué (code $EXIT_CODE)"
    echo "         Causes possibles :"
    echo "         - Token GitHub expiré ou révoqué → en régénérer un sur github.com/settings/tokens"
    echo "         - Scope du token insuffisant (vérifier permission 'Contents: Read/Write')"
    echo "         - Pas de connexion réseau"
    exit 1
fi

echo ""
echo "=== Mise à jour terminée avec succès ==="
echo "    ⚠️  RAPPEL : ne jamais lancer 'git remote -v' et coller le résultat"
echo "    dans une conversation, email ou autre canal. Le token apparaît en clair."
