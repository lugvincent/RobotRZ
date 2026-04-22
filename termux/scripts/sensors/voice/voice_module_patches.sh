#!/bin/bash
# =============================================================================
# FICHIER  : voice_module_patches.sh
# CHEMIN   : (documentation — à lire, non exécutable directement)
# OBJECTIF : Modifications à appliquer aux scripts existants pour intégrer
#            le module Voice (rz_voice_manager.sh).
#
# MODIFICATIONS À FAIRE (4 scripts) :
#   1. original_init.sh    → section Voice dans courant_init.json
#   2. check_config.sh     → validation + génération voice_config.json
#   3. rz_vpiv_parser.sh   → FIFO dédié fifo_voice_in + routage Voice
#   4. rz_stt_manager.sh   → suppression run_tts_listener() (conflit FIFO)
#
# AUTEUR  : Vincent Philippe
# DATE    : 2026-04-22
# =============================================================================


# =============================================================================
# PATCH 1 — original_init.sh
# =============================================================================
#
# A) Dans l'en-tête, section "MODULES COUVERTS", ajouter :
#   [x] Voice : vol (volume multimédia global), output (sortie audio), ttsRate
#
# B) Dans "ÉTAPE 2 — PARAMÈTRES DE CONFIGURATION",
#    AJOUTER après la section "# SECTION STT" et avant "# SECTION MTR" :

cat << 'SECTION_VOICE_PARAMS'
# ---------------------------------------------------------------------------
# SECTION VOICE — Propriétés SP → SE  (Table A : module Voice)
# ---------------------------------------------------------------------------
# vol    : volume multimédia global SE en % (canal music Android)
#          ⚠ INDÉPENDANT de modeSTT — actif même si STT est coupé.
#          Contrôle toute la sortie audio : TTS, musique, baffe jack/BT.
#          Normalisé 0-100 → plage Android native par rz_voice_manager.sh.
# output : sortie audio active (détectée automatiquement par Android)
#          internal = haut-parleur SE | jack = sortie casque | bt = Bluetooth
#          La commutation BT forcée est en V2 (Tasker requis).
# ttsRate: vitesse de synthèse TTS (1.0 = normale, >1 = plus rapide)

voice_vol=80            # % volume initial (0-100)
voice_output="internal" # sortie par défaut : internal | jack | bt
voice_ttsRate="1.0"     # vitesse TTS (float, 1.0 recommandé)

SECTION_VOICE_PARAMS

# C) Dans "ÉTAPE 3 — GÉNÉRATION DE courant_init.json" (heredoc JSON),
#    AJOUTER le bloc "voice" AVANT le bloc "mtr" (et après "stt") :
#
#   CHERCHER la ligne contenant :   "mtr": {
#   INSÉRER AVANT :

cat << 'SECTION_VOICE_JSON'
  "voice": {
    "vol":     $voice_vol,
    "output":  "$voice_output",
    "ttsRate": $voice_ttsRate
  },
SECTION_VOICE_JSON

# D) Dans le résumé final (section "Distributions via check_config.sh"),
#    AJOUTER :
#   log "  → config/sensors/voice_config.json"


# =============================================================================
# PATCH 2 — check_config.sh
# =============================================================================
#
# A) Dans l'en-tête, section "FICHIERS PRODUITS", AJOUTER :
#   config/sensors/voice_config.json  : config sortie audio (vol, output, ttsRate)
#
# B) Dans la déclaration des chemins, AJOUTER après STT_CONFIG= :

cat << 'VOICE_CONFIG_PATH'
VOICE_CONFIG="$BASE_DIR/config/sensors/voice_config.json"
VOICE_CONFIG_PATH

# C) AJOUTER la section Voice complète AVANT le bloc "RÉSUMÉ FINAL".
#    (Après la section STT — chercher "# RÉSUMÉ FINAL")

cat << 'SECTION_VOICE_CHECK'
# =============================================================================
# SECTION VOICE — Lecture, validation et génération de voice_config.json
# =============================================================================
# ⚠ Voice est INDÉPENDANT du modeSTT.
#   rz_voice_manager.sh reste actif même si la reconnaissance vocale est
#   désactivée — il gère toute la sortie audio SE (TTS, musique, alertes).
#
# vol    : volume initial 0-100%
# output : sortie audio (internal|jack|bt)
# ttsRate: vitesse TTS (float positif, 1.0 = normale)
# =============================================================================

log "--- Validation section Voice ---"

if ! jq -e '.voice' "$INPUT_JSON" > /dev/null 2>&1; then
    log "  WARN : Bloc '.voice' absent de courant_init.json"
    log "         voice_config.json non généré — rz_voice_manager.sh utilisera les défauts"
else

    voice_vol=$(    lire_json '.voice.vol'     '80')
    voice_output=$( lire_json '.voice.output'  'internal')
    voice_ttsRate=$(lire_json '.voice.ttsRate' '1.0')

    # Validation vol (entier 0-100)
    if ! [[ "$voice_vol" =~ ^[0-9]+$ ]] || [ "$voice_vol" -lt 0 ] || [ "$voice_vol" -gt 100 ]; then
        erreur "voice.vol invalide : '$voice_vol' (attendu : entier 0-100)"
    fi
    log "  vol = ${voice_vol}%  ✓"

    # Validation output (enum)
    case "$voice_output" in
        internal|jack|bt) log "  output = $voice_output  ✓" ;;
        *) erreur "voice.output invalide : '$voice_output' (attendu : internal|jack|bt)" ;;
    esac

    # Validation ttsRate (float positif)
    if ! [[ "$voice_ttsRate" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
        erreur "voice.ttsRate invalide : '$voice_ttsRate' (attendu : float > 0, ex: 1.0)"
    fi
    log "  ttsRate = $voice_ttsRate  ✓"

    log "--- Section Voice : VALIDÉE ---"

    mkdir -p "$(dirname "$VOICE_CONFIG")"
    cat > "$VOICE_CONFIG" <<EOF
{
  "voice": {
    "vol":     $voice_vol,
    "output":  "$voice_output",
    "ttsRate": $voice_ttsRate
  }
}
EOF
    if ! jq '.' "$VOICE_CONFIG" > /dev/null 2>&1; then
        erreur "voice_config.json généré est invalide"
    fi
    log "voice_config.json généré et validé  ✓"

fi

SECTION_VOICE_CHECK

# D) Dans le résumé final de check_config.sh, AJOUTER :
#   log "  → $VOICE_CONFIG"


# =============================================================================
# PATCH 3 — rz_vpiv_parser.sh
# =============================================================================
#
# CINQ POINTS D'INTERVENTION :
#
# ── 3A : Déclaration FIFO ────────────────────────────────────────────────────
# CHERCHER la ligne :   FIFO_APPLI="$FIFO_DIR/fifo_appli_in"
# AJOUTER APRÈS :

cat << 'PATCH3A'
FIFO_VOICE="$FIFO_DIR/fifo_voice_in"   # module Voice (TTS, vol, output)
PATCH3A

# ── 3B : Fonction init_fifo() ────────────────────────────────────────────────
# CHERCHER la ligne :   [ -p "$FIFO_APPLI" ]   || mkfifo "$FIFO_APPLI"
# AJOUTER APRÈS :

cat << 'PATCH3B'
    [ -p "$FIFO_VOICE"  ] || mkfifo "$FIFO_VOICE"
PATCH3B

# ── 3C : Bloc CAT=A — cas "Voice" ───────────────────────────────────────────
# CHERCHER ce bloc dans le case "A") :
#   elif [[ "$MODULE" == "Voice" && "$PROP" == "tts" ]]; then
#       # TTS direct depuis SP
#       termux-tts-speak "$(echo "$VAL" | tr -d '"')"
#
# REMPLACER PAR :

cat << 'PATCH3C'
            elif [[ "$MODULE" == "Voice" ]]; then
                # Route toutes les trames Voice vers rz_voice_manager.sh
                # (TTS, vol, output) — supprime le conflit fifo_termux_in/stt_manager
                log "CAT=A Voice → fifo_voice_in : $msg"
                write_fifo "$FIFO_VOICE" "$msg"
PATCH3C

# ── 3D : Bloc CAT=V|F — cas "Voice" ─────────────────────────────────────────
# CHERCHER dans le case "$MODULE" du bloc V|F, la ligne :
#   # Tous les autres modules → fifo_termux_in (managers capteurs)
# AJOUTER AVANT ce commentaire :

cat << 'PATCH3D'
                # Voice (vol, output) → manager Voice dédié
                "Voice")
                    write_fifo "$FIFO_VOICE" "$msg"
                    ;;

PATCH3D

# ── 3E : Bloc CAT=I|E — propagation urgences vers Voice ─────────────────────
# CHERCHER dans le bloc I|E :
#   if [[ "$MODULE" == "Urg" ]]; then
#       write_fifo "$FIFO_TERMUX" "\$I:Urg:..."
#   else
#       write_fifo "$FIFO_TERMUX" "$msg"
#   fi
#
# AJOUTER APRÈS le bloc if/else (avant la fermeture du case I|E) :

cat << 'PATCH3E'
            # Propagation urgences vers Voice (TTS urgence interruptif)
            if [[ "$CAT" == "E" && "$MODULE" == "Urg" ]]; then
                write_fifo "$FIFO_VOICE" "$msg"
            fi
PATCH3E

# ── 3F : En-tête INTERACTIONS ────────────────────────────────────────────────
# Dans le bloc commentaire INTERACTIONS, AJOUTER :
#   Écrit   : fifo/fifo_voice_in    (rz_voice_manager.sh — TTS, vol, output)


# =============================================================================
# PATCH 4 — rz_stt_manager.sh  (version 2.0 → 2.1)
# =============================================================================
#
# OBJECTIF : Retirer run_tts_listener() qui lisait fifo_termux_in pour le TTS.
# Ce rôle est désormais tenu par rz_voice_manager.sh via fifo_voice_in.
# Cela résout le conflit de FIFO documenté dans la fiche module STT.
#
# ── 4A : Supprimer la fonction run_tts_listener() entière ───────────────────
# Localisation : bloc de commentaires "BOUCLE TTS LOCAL" + fonction
# SUPPRIMER tout ce bloc (environ 60 lignes) :
#   # =============================================================================
#   # BOUCLE TTS LOCAL
#   # ...
#   run_tts_listener() {
#       ...
#   }
#
# ── 4B : Dans main(), supprimer le lancement du listener ────────────────────
# SUPPRIMER ces 3 lignes :
#   # Lancement TTS listener en arrière-plan
#   run_tts_listener &
#   local tts_listener_pid=$!
#   log_stt "TTS listener lancé (PID=$tts_listener_pid)"
#
# SUPPRIMER aussi dans le nettoyage final :
#   kill "$tts_listener_pid" 2>/dev/null
#
# ── 4C : Mettre à jour l'en-tête ────────────────────────────────────────────
# REMPLACER la section "FLUX TTS" par :

cat << 'PATCH4C'
# FLUX TTS
# --------
# Désormais délégué à rz_voice_manager.sh via fifo_voice_in.
# SP → MQTT → rz_vpiv_parser.sh → fifo_voice_in → rz_voice_manager.sh
# rz_stt_manager.sh ne lit plus fifo_termux_in pour le TTS.
PATCH4C

# MODIFIER la VERSION :
#   VERSION : 2.1  (délégation TTS à rz_voice_manager.sh, suppression run_tts_listener)
#
# ── 4D : Supprimer la variable FIFO_IN si elle n'est plus utilisée ──────────
# Vérifier si FIFO_IN est encore utilisé ailleurs dans le script.
# Si seul run_tts_listener() l'utilisait → supprimer la déclaration.


# =============================================================================
# NOUVEAUX FICHIERS À CRÉER SUR LE SMARTPHONE
# =============================================================================
echo ""
echo "Arborescence à créer :"
echo "  mkdir -p scripts/sensors/voice/"
echo "  cp rz_voice_manager.sh scripts/sensors/voice/"
echo "  chmod +x scripts/sensors/voice/rz_voice_manager.sh"
echo ""
echo "voice_config.json généré automatiquement par check_config.sh"
echo "après ajout de la section Voice dans courant_init.json."
echo ""

# =============================================================================
# ORDRE DE DÉMARRAGE RECOMMANDÉ
# (à intégrer dans rz_launch.sh ou équivalent)
# =============================================================================
echo "Ordre de démarrage :"
echo "  1. rz_vpiv_parser.sh       (crée les FIFOs + écoute MQTT)"
echo "  2. rz_voice_manager.sh     (TTS boot + volume initial)"
echo "  3. rz_stt_manager.sh       (PocketSphinx)"
echo "  4. rz_state-manager.sh     (état global)"
echo "  5. autres managers capteurs"
